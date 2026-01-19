#ifndef GUI_H
#define GUI_H

#include "types.h"
#include "vga.h"

/* Элементы GUI */
typedef struct {
    int x, y;
    int width, height;
    char* title;
    u8 color;
    int active;
} Window;

typedef struct {
    int x, y;
    int width, height;
    char* text;
    u8 color;
    int pressed;
} Button;

/* Функции GUI */
void gui_init(void);
void gui_draw_window(Window* win);
void gui_draw_button(Button* btn);
void gui_clear_area(int x, int y, int w, int h, u8 color);

/* Цвета GUI */
#define GUI_WINDOW_BG     0x17  // Светло-серый
#define GUI_TITLE_BAR     0x04  // Красный
#define GUI_BUTTON_NORMAL 0x07  // Темно-серый
#define GUI_BUTTON_HOVER  0x08  // Серый
#define GUI_TEXT_COLOR    0x0F  // Белый

#endif
