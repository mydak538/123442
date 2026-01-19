#ifndef SCREEN_H
#define SCREEN_H

#define ROWS 25
#define COLS 80

extern volatile unsigned short* VGA;
extern unsigned int cursor_row, cursor_col;
extern unsigned char text_color;
extern unsigned int protected_line_start;  // Новая переменная

/* Цвета текста */
#define COLOR_BLACK   0x00
#define COLOR_BLUE    0x01
#define COLOR_GREEN   0x02
#define COLOR_CYAN    0x03
#define COLOR_RED     0x04
#define COLOR_MAGENTA 0x05
#define COLOR_BROWN   0x06
#define COLOR_GRAY    0x07
#define COLOR_DARK_GRAY   0x08
#define COLOR_LIGHT_BLUE  0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN  0x0B
#define COLOR_LIGHT_RED   0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW  0x0E
#define COLOR_WHITE   0x0F

void clear_screen();
void putchar(char ch);
void prints(const char* s);
void newline();
void set_color(unsigned char color);
unsigned char get_color();
void set_protected_area(unsigned int start_row);  // Новая функция

#endif
