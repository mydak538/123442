#ifndef COMMANDS_H
#define COMMANDS_H

#include "types.h"

/* Константы */
#define MAX_NAME 256
#define MAX_PATH 512
#define MAX_FILES 1024
#define SECTOR_SIZE 512
#define FS_SECTOR_START 100

/* Структуры */
typedef struct {
    char name[MAX_NAME];
    int is_dir;
    char content[4096];
    u32 next_sector;
    u32 size;
} FSNode;

/* Глобальные переменные */
extern FSNode fs_cache[MAX_FILES];
extern int fs_count;
extern char current_dir[MAX_PATH];
extern int fs_dirty;

/* Функции команд */
void show_help(void);
void clear_command(void);
void run_command(char* line);
void split_command(char* line, char* cmd, char* arg1, char* arg2);

/* Команды файловой системы */
void pwd_command(void);
void find_command(const char* pattern);
void grep_command(const char* pattern);
void fs_ls(void);
void fs_cd(const char* name);
void fs_mkdir(const char* name);
void fs_touch(const char* name);
void fs_rm(const char* name);
void fs_cp(const char* src, const char* dest);
void fs_cat(const char* name);
void fs_stat(const char* name);
void fs_rename(const char* old_name, const char* new_name);
void fs_create_test_files(void);
void fs_count_items(void);
void fs_format(void);
void fs_debug(void);
void fs_sync(void);
void fsck_command(void);

/* Вспомогательные функции */
FSNode* fs_find_file(const char* name);
void fs_check_integrity(void);

#endif
