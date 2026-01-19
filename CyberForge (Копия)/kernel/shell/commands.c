#include "commands.h"
#include "screen.h"
#include "keyboard.h"
#include "fs.h"
#include "string.h"
#include "stdlib.h"

/* ============== СУЩЕСТВУЮЩИЕ ФУНКЦИИ ============== */

void show_help(void) {
    set_color(COLOR_YELLOW);
    prints("CyberForge Commands:\n");
    prints("====================\n\n");
    
    set_color(COLOR_LIGHT_GREEN);
    prints("help      ");
    set_color(COLOR_GRAY);
    prints("clear     ");
    set_color(COLOR_LIGHT_GREEN);
    prints("ls        \n");
    
    set_color(COLOR_GRAY);
    prints("pwd       ");
    set_color(COLOR_LIGHT_GREEN);
    prints("cd        ");
    set_color(COLOR_GRAY);
    prints("mkdir     \n");
    
    set_color(COLOR_LIGHT_GREEN);
    prints("touch     ");
    set_color(COLOR_GRAY);
    prints("rm        ");
    set_color(COLOR_LIGHT_GREEN);
    prints("cp        \n");
    
    set_color(COLOR_GRAY);
    prints("cat       ");
    set_color(COLOR_LIGHT_GREEN);
    prints("find      ");
    set_color(COLOR_GRAY);
    prints("stat      \n");
    
    set_color(COLOR_LIGHT_GREEN);
    prints("rename    ");
    set_color(COLOR_GRAY);
    prints("testfiles ");
    set_color(COLOR_LIGHT_GREEN);
    prints("stats     \n");
    
    set_color(COLOR_GRAY);
    prints("grep      ");
    set_color(COLOR_LIGHT_GREEN);
    prints("fsck      ");
    set_color(COLOR_RED);
    prints("format    \n\n");
    
    set_color(COLOR_WHITE);
    prints("Current: ");
    set_color(COLOR_CYAN);
    prints(current_dir);
    set_color(COLOR_WHITE);
    prints("\n");
}

void clear_command(void) {
    clear_screen();
    prints("Screen cleared\n");
}

/* Простая функция разделения */
void split_command(char* line, char* cmd, char* arg1, char* arg2) {
    int i = 0, j = 0, part = 0;
    cmd[0] = arg1[0] = arg2[0] = '\0';
    
    while (line[i] == ' ') i++;
    
    while(line[i]) {
        if(line[i] == ' ') {
            if(part == 0) cmd[j] = '\0';
            else if(part == 1) arg1[j] = '\0';
            part++;
            j = 0;
            i++;
            
            while (line[i] == ' ') i++;
            continue;
        }
        
        if(part == 0) cmd[j++] = line[i];
        else if(part == 1) arg1[j++] = line[i];
        else if(part == 2) arg2[j++] = line[i];
        i++;
    }
    
    if(part == 0 && j > 0) cmd[j] = '\0';
    else if(part == 1 && j > 0) arg1[j] = '\0';
    else if(part == 2 && j > 0) arg2[j] = '\0';
}

void run_command(char* line) {
    if(strlen(line) == 0) return;
    
    char cmd[128];
    char arg1[128] = "";
    char arg2[128] = "";
    
    split_command(line, cmd, arg1, arg2);
    
    if(strcmp(cmd, "help") == 0) show_help();
    else if(strcmp(cmd, "clear") == 0) clear_command();
    else if(strcmp(cmd, "ls") == 0) fs_ls();
    else if(strcmp(cmd, "pwd") == 0) pwd_command();
    else if(strcmp(cmd, "cd") == 0) {
        if(strlen(arg1) > 0) fs_cd(arg1);
        else prints("Usage: cd <dir>\n");
    }
    else if(strcmp(cmd, "mkdir") == 0) {
        if(strlen(arg1) > 0) fs_mkdir(arg1);
        else prints("Usage: mkdir <name>\n");
    }
    else if(strcmp(cmd, "touch") == 0) {
        if(strlen(arg1) > 0) fs_touch(arg1);
        else prints("Usage: touch <name>\n");
    }
    else if(strcmp(cmd, "rm") == 0) {
        if(strlen(arg1) > 0) fs_rm(arg1);
        else prints("Usage: rm <name>\n");
    }
    else if(strcmp(cmd, "cp") == 0) {
        if(strlen(arg1) > 0 && strlen(arg2) > 0) fs_copy(arg1, arg2);
        else prints("Usage: cp <src> <dest>\n");
    }
    else if(strcmp(cmd, "cat") == 0) {
        FSNode* file = fs_find_file(arg1);
        if(file && !file->is_dir) {
            prints(file->content);
            prints("\n");
        } else {
            prints("Error: File not found: ");
            prints(arg1);
            prints("\n");
        }
    }
    else if(strcmp(cmd, "find") == 0) {
        if(strlen(arg1) > 0) find_command(arg1);
        else prints("Usage: find <pattern>\n");
    }
    else if(strcmp(cmd, "stat") == 0) {
        if(strlen(arg1) > 0) fs_size(arg1);
        else prints("Usage: stat <file>\n");
    }
    else if(strcmp(cmd, "rename") == 0) {
        if(strlen(arg1) > 0 && strlen(arg2) > 0) {
            char old_path[MAX_PATH];
            char new_path[MAX_PATH];
            
            if(strcmp(current_dir, "/") == 0) {
                strcpy(old_path, arg1);
                strcpy(new_path, arg2);
            } else {
                strcpy(old_path, current_dir);
                strcat(old_path, arg1);
                strcpy(new_path, current_dir);
                strcat(new_path, arg2);
            }
            
            int found = -1;
            for (int i = 0; i < fs_count; i++) {
                if(strcmp(fs_cache[i].name, old_path) == 0) {
                    found = i;
                    break;
                }
            }
            
            if(found != -1) {
                strcpy(fs_cache[found].name, new_path);
                fs_mark_dirty();
                fs_save_to_disk();
                prints("Renamed '");
                prints(arg1);
                prints("' to '");
                prints(arg2);
                prints("'\n");
            } else {
                prints("Error: File not found: ");
                prints(arg1);
                prints("\n");
            }
        }
        else prints("Usage: rename <old> <new>\n");
    }
    else if(strcmp(cmd, "testfiles") == 0) {
        prints("Creating test files...\n");
        fs_touch("test1.txt");
        fs_touch("test2.txt");
        fs_touch("test3.txt");
        prints("Test files created\n");
    }
    else if(strcmp(cmd, "stats") == 0) {
        char buf[20];
        prints("Filesystem statistics:\n");
        prints("=====================\n");
        itoa(fs_count, buf, 10);
        prints("Total items: ");
        prints(buf);
        prints("\n");
        
        int dirs = 0, files = 0;
        for(int i = 0; i < fs_count; i++) {
            if(fs_cache[i].is_dir) dirs++;
            else files++;
        }
        
        itoa(dirs, buf, 10);
        prints("Directories: ");
        prints(buf);
        prints("\n");
        
        itoa(files, buf, 10);
        prints("Files: ");
        prints(buf);
        prints("\n");
        
        itoa(MAX_FILES - fs_count, buf, 10);
        prints("Free slots: ");
        prints(buf);
        prints("\n");
    }
    else if(strcmp(cmd, "grep") == 0) {
        prints("Grep command - TODO: implement\n");
        // Здесь можно вызвать grep_command если она реализована
    }
    else if(strcmp(cmd, "format") == 0) {
        fs_format();
    }
    else if(strcmp(cmd, "debug") == 0) {
        prints("Debug info:\n");
        prints("Current dir: ");
        prints(current_dir);
        prints("\n");
        prints("FS count: ");
        char buf[20];
        itoa(fs_count, buf, 10);
        prints(buf);
        prints("\n");
        prints("FS dirty: ");
        itoa(fs_dirty, buf, 10);
        prints(buf);
        prints("\n");
    }
    else if(strcmp(cmd, "sync") == 0) {
        fs_save_to_disk();
        prints("Filesystem synced to disk\n");
    }
    else if(strcmp(cmd, "fsck") == 0) {
        fsck_command();
    }
    else {
        prints("Unknown command: ");
        prints(cmd);
        prints("\nType 'help' for available commands\n");
    }
}
