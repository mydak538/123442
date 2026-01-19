#ifndef TYPES_H
#define TYPES_H

// Убираем #include <stdint.h>
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;

typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;

#define NULL ((void*)0)
#define MAX_NAME 256
#define MAX_PATH 1024
#define SECTOR_SIZE 512
#define FS_SECTOR_START 1
#define MAX_FILES 64
#define MAX_HISTORY 10

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CURSOR_SPEED 5

#define DARK_BLUE 0x00008B
#define LIGHT_GRAY 0xC0C0C0
#define BLACK 0x000000
#define WHITE 0xFFFFFF

#define LOADING_BG_COLOR 0x00
#define LOADING_TEXT_COLOR 0x0F
#define LOADING_ACCENT_COLOR 0x0B

#define KEY_UP    0x48
#define KEY_DOWN  0x50
#define KEY_LEFT  0x4B
#define KEY_RIGHT 0x4D
#define KEY_ENTER 0x1C

#define COLS 80
#define ROWS 25
#define EXPLORER_WIDTH COLS
#define EXPLORER_HEIGHT ROWS
#define MAX_VISIBLE_FILES (ROWS - 4)
#define PATH_MAX_DISPLAY 70

#define AUTORUN_FILE "SystemRoot/config/autorun.cfg"
#define AUTORUN_MAX_COMMAND 128

typedef struct {
    char name[MAX_NAME];
    int is_dir;
    uint32_t size;
} FileEntry;

typedef struct {
    int x, y;
    int width, height;
    char title[MAX_NAME];
    FileEntry files[MAX_FILES];
    int file_count;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH];
} Explorer;

typedef struct {
    char name[MAX_PATH];
    int is_dir;
    char content[4096];
    uint32_t next_sector;
    uint32_t size;
} FSNode;

#endif
