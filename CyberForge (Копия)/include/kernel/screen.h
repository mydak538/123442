#ifndef SCREEN_H
#define SCREEN_H

// Убираем #include "types.h"
#define VGA_BUFFER 0xB8000

// Объявляем типы напрямую
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

extern volatile uint16_t* VGA;

#define ROWS 25
#define COLS 80
extern uint32_t cursor_row, cursor_col;
extern uint8_t text_color;

void clear_screen(void);
void putchar(char ch);
void prints(const char* s);
void newline(void);
void set_text_color(uint8_t color);

#endif
