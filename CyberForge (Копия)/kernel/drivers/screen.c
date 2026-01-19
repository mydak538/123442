#include "../include/screen.h"
#include "../include/string.h"

volatile unsigned short* VGA = (unsigned short*)0xB8000;
unsigned int cursor_row = 0, cursor_col = 0;
unsigned char text_color = 0x07;

void clear_screen() {
    for(int r = 0; r < ROWS; r++) {
        for(int c = 0; c < COLS; c++) {
            VGA[r * COLS + c] = (unsigned short)(' ' | (text_color << 8));
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

void set_color(unsigned char color) { text_color = color; }
unsigned char get_color() { return text_color; }

void putchar(char ch) {
    if(ch == '\n') {
        cursor_col = 0;
        cursor_row++;
        
        if(cursor_row >= ROWS) {
            for(int r = 0; r < ROWS-1; r++) {
                for(int c = 0; c < COLS; c++) {
                    VGA[r * COLS + c] = VGA[(r+1) * COLS + c];
                }
            }
            for(int c = 0; c < COLS; c++) {
                VGA[(ROWS-1) * COLS + c] = (unsigned short)(' ' | (text_color << 8));
            }
            cursor_row = ROWS - 1;
        }
        return;
    }
    
    if(ch == '\b') {
        if(cursor_col > 0) {
            cursor_col--;
        } else if(cursor_row > 0) {
            cursor_row--;
            cursor_col = COLS - 1;
        }
        VGA[cursor_row * COLS + cursor_col] = (unsigned short)(' ' | (text_color << 8));
        return;
    }
    
    VGA[cursor_row * COLS + cursor_col] = (unsigned short)(ch | (text_color << 8));
    cursor_col++;
    
    if(cursor_col >= COLS) {
        cursor_col = 0;
        cursor_row++;
        
        if(cursor_row >= ROWS) {
            for(int r = 0; r < ROWS-1; r++) {
                for(int c = 0; c < COLS; c++) {
                    VGA[r * COLS + c] = VGA[(r+1) * COLS + c];
                }
            }
            for(int c = 0; c < COLS; c++) {
                VGA[(ROWS-1) * COLS + c] = (unsigned short)(' ' | (text_color << 8));
            }
            cursor_row = ROWS - 1;
        }
    }
}

void prints(const char* s) {
    unsigned char old_color = text_color;
    while(*s) {
        if(*s == '\\' && *(s+1) == 'c' && *(s+2)) {
            char color_char = *(s+2);
            switch(color_char) {
                case '0': set_color(COLOR_BLACK); break;
                case '1': set_color(COLOR_BLUE); break;
                case '2': set_color(COLOR_GREEN); break;
                case '3': set_color(COLOR_CYAN); break;
                case '4': set_color(COLOR_RED); break;
                case '5': set_color(COLOR_MAGENTA); break;
                case '6': set_color(COLOR_BROWN); break;
                case '7': set_color(COLOR_GRAY); break;
                case '8': set_color(COLOR_DARK_GRAY); break;
                case '9': set_color(COLOR_LIGHT_BLUE); break;
                case 'a': set_color(COLOR_LIGHT_GREEN); break;
                case 'b': set_color(COLOR_LIGHT_CYAN); break;
                case 'c': set_color(COLOR_LIGHT_RED); break;
                case 'd': set_color(COLOR_LIGHT_MAGENTA); break;
                case 'e': set_color(COLOR_YELLOW); break;
                case 'f': set_color(COLOR_WHITE); break;
                case 'r': set_color(old_color); break;
            }
            s += 3;
            continue;
        }
        putchar(*s++);
    }
}

void newline() { putchar('\n'); }
