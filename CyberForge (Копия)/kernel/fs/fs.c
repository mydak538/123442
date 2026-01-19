#include "types.h"
#include "screen.h"
#include "keyboard.h"
#include "commands.h"
#include "string.h"
#include "fs.h"
#include "ports.h"
#include "stdlib.h"

/* ATA Disk I/O */
#define ATA_DATA 0x1F0
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DEVICE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_CMD 0x1F7

FSNode fs_cache[MAX_FILES];
int fs_count = 0;
char current_dir[MAX_PATH] = "/";
int fs_dirty = 0;

/* ATA functions */
void ata_wait_ready() {
    while (inb(ATA_STATUS) & 0x80);
}

void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & 0x08));
}

void ata_read_sector(u32 lba, u8* buffer) {
    outb(ATA_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW, (u8)lba);
    outb(ATA_LBA_MID, (u8)(lba >> 8));
    outb(ATA_LBA_HIGH, (u8)(lba >> 16));
    outb(ATA_CMD, 0x20);

    ata_wait_ready();
    if (inb(ATA_STATUS) & 0x01) {
        prints("ATA Read Error\n");
        return;
    }

    ata_wait_drq();
    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        u16 data = inw(ATA_DATA);
        buffer[i * 2] = (u8)data;
        buffer[i * 2 + 1] = (u8)(data >> 8);
    }
}

void ata_write_sector(u32 lba, u8* buffer) {
    outb(ATA_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW, (u8)lba);
    outb(ATA_LBA_MID, (u8)(lba >> 8));
    outb(ATA_LBA_HIGH, (u8)(lba >> 16));
    outb(ATA_CMD, 0x30);

    ata_wait_ready();
    ata_wait_drq();

    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        u16 data = (buffer[i * 2 + 1] << 8) | buffer[i * 2];
        outw(ATA_DATA, data);
    }

    ata_wait_ready();
    if (inb(ATA_STATUS) & 0x01) {
        prints("ATA Write Error\n");
    }
}


void fs_load_from_disk() {
    u8 sector_buffer[SECTOR_SIZE];
    u32 sector = FS_SECTOR_START;
    fs_count = 0;
    fs_dirty = 0;

    #define SECTORS_PER_NODE 11

    while (sector != 0 && fs_count < MAX_FILES) {
        u8* node_data = (u8*)&fs_cache[fs_count];
        
        for (int j = 0; j < SECTORS_PER_NODE; j++) {
            ata_read_sector(sector + j, sector_buffer);
            int offset = j * SECTOR_SIZE;
            if (offset < sizeof(FSNode)) {
                memcpy(node_data + offset, sector_buffer, SECTOR_SIZE);
            }
        }
        
        sector = fs_cache[fs_count].next_sector;
        fs_count++;
    }

    if (fs_count == 0) {
        strcpy(fs_cache[0].name, "/");
        fs_cache[0].is_dir = 1;
        fs_cache[0].content[0] = '\0';
        fs_cache[0].next_sector = 0;
        fs_cache[0].size = 0;
        fs_count = 1;
        fs_dirty = 1;
        fs_save_to_disk();
    }
}

void fs_save_to_disk() {
    if (!fs_dirty) return;

    u8 sector_buffer[SECTOR_SIZE];
    u32 sector = FS_SECTOR_START;

    // Теперь структура FSNode занимает примерно 4096 + 1024 + 4 + 4 = 5128 байт
    // 5128 / 512 = 10.01 → 11 секторов на узел
    #define SECTORS_PER_NODE 11

    for (int i = 0; i < fs_count; i++) {
        u8* node_data = (u8*)&fs_cache[i];
        
        for (int j = 0; j < SECTORS_PER_NODE; j++) {
            int offset = j * SECTOR_SIZE;
            if (offset < sizeof(FSNode)) {
                memcpy(sector_buffer, node_data + offset, SECTOR_SIZE);
            } else {
                memset(sector_buffer, 0, SECTOR_SIZE);
            }
            ata_write_sector(sector + j, sector_buffer);
        }
        
        sector = (i == fs_count - 1) ? 0 : FS_SECTOR_START + (i + 1) * SECTORS_PER_NODE;
        if (i < fs_count - 1) {
            fs_cache[i].next_sector = sector;
        } else {
            fs_cache[i].next_sector = 0;
        }
    }

    fs_dirty = 0;
}

void fs_mark_dirty() {
    fs_dirty = 1;
}

void fs_init() {
    fs_load_from_disk();
    strcpy(current_dir, "/");
}

void fs_ls() {
    prints("Contents of ");
    prints(current_dir);
    prints(":\n");

    for (int i = 0; i < fs_count; i++) {
        if (strcmp(current_dir, "/") == 0) {
            char* slash = strchr(fs_cache[i].name, '/');
            if (slash == NULL || slash == fs_cache[i].name) {
                prints(fs_cache[i].name);
                if (fs_cache[i].is_dir) prints("/");
                newline();
            }
        } else {
            if (strstr(fs_cache[i].name, current_dir) == fs_cache[i].name) {
                char* name_part = fs_cache[i].name + strlen(current_dir);
                if (*name_part != '\0' && strchr(name_part, '/') == NULL) {
                    prints(name_part);
                    if (fs_cache[i].is_dir) prints("/");
                    newline();
                }
            }
        }
    }
}

void fs_mkdir(const char* name) {
    if (fs_count >= MAX_FILES) {
        prints("Error: Maximum files reached\n");
        return;
    }

    char full_path[MAX_PATH];
    if (strcmp(current_dir, "/") == 0) {
        if (strlen(name) >= MAX_NAME) {
            prints("Error: Name too long: ");
            prints(name);
            newline();
            return;
        }
        strcpy(full_path, name);
    } else {
        if (strlen(current_dir) + strlen(name) + 1 >= MAX_PATH) {
            prints("Error: Path too long: ");
            prints(name);
            newline();
            return;
        }
        strcpy(full_path, current_dir);
        strcat(full_path, name);
    }

    for (int i = 0; i < fs_count; i++) {
        if (strcmp(fs_cache[i].name, full_path) == 0) {
            prints("Error: Name already exists: ");
            prints(name);
            newline();
            return;
        }
    }

    strcpy(fs_cache[fs_count].name, full_path);
    fs_cache[fs_count].is_dir = 1;
    fs_cache[fs_count].content[0] = '\0';
    fs_cache[fs_count].next_sector = 0;
    fs_cache[fs_count].size = 0;
    fs_count++;
    fs_mark_dirty();
    fs_save_to_disk();
    prints("Directory '");
    prints(name);
    prints("' created\n");
}

void fs_touch(const char* name) {
    if (fs_count >= MAX_FILES) {
        prints("Error: Maximum files reached\n");
        return;
    }

    char full_path[MAX_PATH];
    if (strcmp(current_dir, "/") == 0) {
        if (strlen(name) >= MAX_NAME) {
            prints("Error: Name too long: ");
            prints(name);
            newline();
            return;
        }
        strcpy(full_path, name);
    } else {
        if (strlen(current_dir) + strlen(name) + 1 >= MAX_PATH) {
            prints("Error: Path too long: ");
            prints(name);
            newline();
            return;
        }
        strcpy(full_path, current_dir);
        strcat(full_path, name);
    }

    for (int i = 0; i < fs_count; i++) {
        if (strcmp(fs_cache[i].name, full_path) == 0) {
            prints("Error: Name already exists: ");
            prints(name);
            newline();
            return;
        }
    }

    strcpy(fs_cache[fs_count].name, full_path);
    fs_cache[fs_count].is_dir = 0;
    fs_cache[fs_count].content[0] = '\0';
    fs_cache[fs_count].next_sector = 0;
    fs_cache[fs_count].size = 0;
    fs_count++;
    fs_mark_dirty();
    fs_save_to_disk();
    prints("File '");
    prints(name);
    prints("' created\n");
}

void fs_rm(const char* name) {
    char full_path[MAX_PATH];
    if (strcmp(current_dir, "/") == 0) {
        if (strlen(name) >= MAX_NAME) {
            prints("Error: Name too long: ");
            prints(name);
            newline();
            return;
        }
        strcpy(full_path, name);
    } else {
        if (strlen(current_dir) + strlen(name) + 1 >= MAX_PATH) {
            prints("Error: Path too long: ");
            prints(name);
            newline();
            return;
        }
        strcpy(full_path, current_dir);
        strcat(full_path, name);
    }

    int found = -1;
    for (int i = 0; i < fs_count; i++) {
        if (strcmp(fs_cache[i].name, full_path) == 0) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        prints("Error: File or directory not found: ");
        prints(name);
        newline();
        return;
    }

    if (fs_cache[found].is_dir) {
        char dir_path[MAX_PATH];
        strcpy(dir_path, full_path);
        if (strcmp(full_path, "/") != 0) {
            if (strlen(dir_path) + 1 >= MAX_PATH) {
                prints("Error: Directory path too long\n");
                return;
            }
            strcat(dir_path, "/");
        }

        for (int i = fs_count - 1; i >= 0; i--) {
            if (i != found && strstr(fs_cache[i].name, dir_path) == fs_cache[i].name) {
                for (int j = i; j < fs_count - 1; j++) {
                    fs_cache[j] = fs_cache[j + 1];
                }
                fs_count--;
            }
        }
    }

    for (int i = found; i < fs_count - 1; i++) {
        fs_cache[i] = fs_cache[i + 1];
    }
    fs_count--;
    fs_mark_dirty();
    fs_save_to_disk();

    prints("'");
    prints(name);
    prints("' removed\n");
}

void fs_cd(const char* name) {
    if (strcmp(name, "..") == 0) {
        if (strcmp(current_dir, "/") != 0) {
            char* last_slash = strrchr(current_dir, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
                if (current_dir[0] == '\0') {
                    strcpy(current_dir, "/");
                }
            }
        }
    } else if (strcmp(name, "/") == 0) {
        strcpy(current_dir, "/");
    } else {
        char full_path[MAX_PATH];
        if (strcmp(current_dir, "/") == 0) {
            if (strlen(name) >= MAX_NAME) {
                prints("Error: Name too long: ");
                prints(name);
                newline();
                return;
            }
            strcpy(full_path, name);
        } else {
            if (strlen(current_dir) + strlen(name) + 1 >= MAX_PATH) {
                prints("Error: Path too long: ");
                prints(name);
                newline();
                return;
            }
            strcpy(full_path, current_dir);
            strcat(full_path, name);
        }

        int found = 0;
        for (int i = 0; i < fs_count; i++) {
            if (strcmp(fs_cache[i].name, full_path) == 0 && fs_cache[i].is_dir) {
                strcpy(current_dir, full_path);
                if (strcmp(full_path, "/") != 0) {
                    strcat(current_dir, "/");
                }
                found = 1;
                break;
            }
        }

        if (!found) {
            prints("Error: Directory not found: ");
            prints(name);
            newline();
        }
    }
}

FSNode* fs_find_file(const char* name) {
    char full_path[MAX_PATH];
    if (strcmp(current_dir, "/") == 0) {
        if (strlen(name) >= MAX_NAME) {
            prints("Error: Name too long: ");
            prints(name);
            newline();
            return NULL;
        }
        strcpy(full_path, name);
    } else {
        if (strlen(current_dir) + strlen(name) + 1 >= MAX_PATH) {
            prints("Error: Path too long: ");
            prints(name);
            newline();
            return NULL;
        }
        strcpy(full_path, current_dir);
        strcat(full_path, name);
    }

    for (int i = 0; i < fs_count; i++) {
        if (strcmp(fs_cache[i].name, full_path) == 0 && !fs_cache[i].is_dir) {
            return &fs_cache[i];
        }
    }
    return NULL;
}

void fs_copy(const char* src_name, const char* dest_name) {
    char src_path[MAX_PATH];
    if (strcmp(current_dir, "/") == 0) {
        if (strlen(src_name) >= MAX_NAME) {
            prints("Error: Source name too long: ");
            prints(src_name);
            newline();
            return;
        }
        strcpy(src_path, src_name);
    } else {
        if (strlen(current_dir) + strlen(src_name) + 1 >= MAX_PATH) {
            prints("Error: Source path too long: ");
            prints(src_name);
            newline();
            return;
        }
        strcpy(src_path, current_dir);
        strcat(src_path, src_name);
    }

    FSNode* src = NULL;
    for (int i = 0; i < fs_count; i++) {
        if (strcmp(fs_cache[i].name, src_path) == 0 && !fs_cache[i].is_dir) {
            src = &fs_cache[i];
            break;
        }
    }

    if (!src) {
        prints("Error: Source file not found: ");
        prints(src_name);
        newline();
        return;
    }

    if (fs_count >= MAX_FILES) {
        prints("Error: Maximum files reached\n");
        return;
    }

    char full_path[MAX_PATH];
    if (strcmp(current_dir, "/") == 0) {
        if (strlen(dest_name) >= MAX_NAME) {
            prints("Error: Destination name too long: ");
            prints(dest_name);
            newline();
            return;
        }
        strcpy(full_path, dest_name);
    } else {
        if (strlen(current_dir) + strlen(dest_name) + 1 >= MAX_PATH) {
            prints("Error: Destination path too long: ");
            prints(dest_name);
            newline();
            return;
        }
        strcpy(full_path, current_dir);
        strcat(full_path, dest_name);
    }

    strcpy(fs_cache[fs_count].name, full_path);
    fs_cache[fs_count].is_dir = 0;
    strcpy(fs_cache[fs_count].content, src->content);
    fs_cache[fs_count].next_sector = 0;
    fs_cache[fs_count].size = src->size;
    fs_count++;
    fs_mark_dirty();
    fs_save_to_disk();
    prints("File copied to '");
    prints(dest_name);
    prints("'\n");
}

int folder_size(const char* folder_path) {
    int total = 0;
    char path_with_slash[MAX_PATH];
    strcpy(path_with_slash, folder_path);
    if (folder_path[strlen(folder_path) - 1] != '/') {
        strcat(path_with_slash, "/");
    }

    for (int i = 0; i < fs_count; i++) {
        FSNode* node = &fs_cache[i];
        if (!node->is_dir && strstr(node->name, path_with_slash) == node->name) {
            total += node->size;
        }
    }
    return total;
}

void fs_size(const char* name) {
    FSNode* node = NULL;
    // ищем узел с точным именем
    for (int i = 0; i < fs_count; i++) {
        if (strcmp(fs_cache[i].name, name) == 0) {
            node = &fs_cache[i];
            break;
        }
    }

    if (!node) {
        prints("Error: File or folder not found: ");
        prints(name);
        newline();
        return;
    }

    int size;
    if (node->is_dir) {
        size = folder_size(node->name);
        prints("Folder size: ");
    } else {
        size = node->size;
        prints("File size: ");
    }

    char size_str[16];
    itoa(size, size_str, 10);
    prints(size_str);
    prints(" Bytes\n");
}

void find_command(const char* pattern) {
    prints("Searching for: ");
    prints(pattern);
    newline();
    for(int i = 0; i < fs_count; i++) {
        if(strstr(fs_cache[i].name, pattern) != NULL) {
            prints(fs_cache[i].name);
            if(fs_cache[i].is_dir) prints("/");
            newline();
        }
    }
}

void pwd_command() { 
    prints(current_dir); 
    newline(); 
}

void fs_format(void) {
    prints("WARNING: This will erase ALL files and directories!\n");
    prints("Are you sure you want to continue? (y/N): ");
    
    char confirm = keyboard_getchar();
    putchar(confirm);
    newline();
    
    if (confirm == 'y' || confirm == 'Y') {
        prints("Formatting filesystem...\n");
        
        // Сбрасываем файловую систему к начальному состоянию
        fs_count = 1;
        strcpy(fs_cache[0].name, "/");
        fs_cache[0].is_dir = 1;
        fs_cache[0].content[0] = '\0';
        fs_cache[0].next_sector = 0;
        fs_cache[0].size = 0;
        
        // Сбрасываем текущую директорию
        strcpy(current_dir, "/");
        
        // Помечаем как измененную и сохраняем
        fs_mark_dirty();
        fs_save_to_disk();
        
        prints("Filesystem formatted successfully.\n");
    } else {
        prints("Format cancelled.\n");
    }
}


/* Function implementations */
void fs_check_integrity(void) {
    prints("Checking filesystem integrity...\n");
    prints("Filesystem: WexFS\n");
    prints("Version: 1.0\n");
    prints("======================================\n");
    
    int errors_found = 0;
    int warnings_found = 0;
    
    // Проверка на дубликаты
    prints("Phase 1: Checking for duplicates...\n");
    for (int i = 0; i < fs_count; i++) {
        for (int j = i + 1; j < fs_count; j++) {
            if (strcmp(fs_cache[i].name, fs_cache[j].name) == 0) {
                prints("ERROR: Duplicate filename: ");
                prints(fs_cache[i].name);
                newline();
                errors_found++;
            }
        }
    }
    
    // Проверка размера контента
    prints("Phase 2: Checking file sizes...\n");
    for (int i = 0; i < fs_count; i++) {
        if (!fs_cache[i].is_dir) {
            if (fs_cache[i].size > sizeof(fs_cache[i].content)) {
                prints("ERROR: File size exceeds content buffer: ");
                prints(fs_cache[i].name);
                newline();
                prints("  File size: ");
                char size_str[20];
                itoa(fs_cache[i].size, size_str, 10);
                prints(size_str);
                prints(", Max allowed: ");
                itoa(sizeof(fs_cache[i].content), size_str, 10);
                prints(size_str);
                newline();
                errors_found++;
            }
        }
    }
    
    // Проверка максимального количества файлов
    prints("Phase 3: Checking filesystem limits...\n");
    if (fs_count >= MAX_FILES) {
        prints("WARNING: Filesystem at maximum capacity (");
        char max_str[10];
        itoa(MAX_FILES, max_str, 10);
        prints(max_str);
        prints(" files)\n");
        warnings_found++;
    }
    
    // Статистика
    prints("Phase 4: Generating statistics...\n");
    int total_files = 0;
    int total_dirs = 0;
    
    for (int i = 0; i < fs_count; i++) {
        if (fs_cache[i].is_dir) {
            total_dirs++;
        } else {
            total_files++;
        }
    }
    
    prints("======================================\n");
    prints("Filesystem check completed.\n");
    
    char buf[20];
    itoa(total_files, buf, 10);
    prints("Files: "); prints(buf); newline();
    itoa(total_dirs, buf, 10);
    prints("Directories: "); prints(buf); newline();
    itoa(fs_count, buf, 10);
    prints("Total objects: "); prints(buf); newline();
    itoa(MAX_FILES - fs_count, buf, 10);
    prints("Free slots: "); prints(buf); newline();
    
    if (errors_found > 0) {
        itoa(errors_found, buf, 10);
        prints("Errors found: "); prints(buf); newline();
        prints("Run 'format' to fix filesystem errors.\n");
    } else {
        prints("No errors found.\n");
    }
    
    if (warnings_found > 0) {
        itoa(warnings_found, buf, 10);
        prints("Warnings: "); prints(buf); newline();
    }
    
    prints("Filesystem is ");
    if (errors_found == 0) {
        prints("OK");
    } else {
        prints("CORRUPTED");
    }
    prints(".\n");
}
void fsck_command(void) {
    prints("Filesystem Consistency Check\n");
    prints("============================\n");
    prints("This utility will check WexFS filesystem for errors\n");
    prints("and report any inconsistencies.\n\n");
    
    prints("Continue? (y/N): ");
    
    char confirm = keyboard_getchar();
    putchar(confirm);
    newline();
    
    if (confirm == 'y' || confirm == 'Y') {
        fs_check_integrity();
    } else {
        prints("Operation cancelled.\n");
    }
}
