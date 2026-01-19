// fs.h - File System Header for WexOS

#ifndef FS_H
#define FS_H

#include "types.h"

/* File System Constants */
#define MAX_FILES 100
#define MAX_NAME 256
#define MAX_PATH 512
#define SECTOR_SIZE 512
#define FS_SECTOR_START 100

/* File System Node Structure */
typedef struct {
    char name[MAX_NAME];
    int is_dir;
    char content[4096];  // File content
    u32 next_sector;
    u32 size;
} FSNode;

/* Global Variables */
extern FSNode fs_cache[MAX_FILES];
extern int fs_count;
extern char current_dir[MAX_PATH];
extern int fs_dirty;

/* ATA Disk Functions */
void ata_wait_ready(void);
void ata_wait_drq(void);
void ata_read_sector(u32 lba, u8* buffer);
void ata_write_sector(u32 lba, u8* buffer);

/* Memory Functions */
void memset(void* ptr, int value, int num);

/* File System Core Functions */
void fs_load_from_disk(void);
void fs_save_to_disk(void);
void fs_mark_dirty(void);
void fs_init(void);
void fs_format(void);

/* File System Commands */
void fs_ls(void);
void fs_mkdir(const char* name);
void fs_touch(const char* name);
void fs_rm(const char* name);
void fs_cd(const char* name);
void fs_copy(const char* src_name, const char* dest_name);
void fs_size(const char* name);
void find_command(const char* pattern);
void pwd_command(void);
void fsck_command(void);
void fs_check_integrity(void);

/* File Operations */
FSNode* fs_find_file(const char* name);
int folder_size(const char* folder_path);

#endif // FS_H
