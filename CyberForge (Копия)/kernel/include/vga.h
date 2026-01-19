#ifndef VGA_H
#define VGA_H

#include "types.h"

/* VGA функции */
void vga_set_mode_13h(void);
void vga_set_mode_text(void);
void vga_put_pixel(int x, int y, u8 color);
void vga_clear_screen(u8 color);
void vga_draw_rect(int x, int y, int width, int height, u8 color);
void vga_draw_line(int x1, int y1, int x2, int y2, u8 color);

/* VGA цвета (палитра mode 13h) */
#define VGA_BLACK          0
#define VGA_BLUE           1
#define VGA_GREEN          2
#define VGA_CYAN           3
#define VGA_RED            4
#define VGA_MAGENTA        5
#define VGA_BROWN          6
#define VGA_LIGHT_GRAY     7
#define VGA_DARK_GRAY      8
#define VGA_LIGHT_BLUE     9
#define VGA_LIGHT_GREEN    10
#define VGA_LIGHT_CYAN     11
#define VGA_LIGHT_RED      12
#define VGA_LIGHT_MAGENTA  13
#define VGA_YELLOW         14
#define VGA_WHITE          15

/* VGA размеры mode 13h */
#define VGA_WIDTH          320
#define VGA_HEIGHT         200
#define VGA_MEMORY         0xA0000

#endif
