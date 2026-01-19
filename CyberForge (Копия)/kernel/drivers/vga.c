#include "vga.h"
#include "ports.h"

// VGA регистры
#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5

// Установка графического режима 13h через порты VGA
void vga_set_mode_13h(void) {
    // Отключить защиту регистров CRT контроллера
    outb(VGA_CTRL_REGISTER, 0x11);
    outb(VGA_DATA_REGISTER, inb(VGA_DATA_REGISTER) & 0x7F);
    
    // Установить режим 13h
    outb(0x3C2, 0x63); // Миск регистры
    outb(0x3C4, 0x01); outb(0x3C5, 0x01); // Последовательный контроллер
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    
    // Графический контроллер
    outb(0x3CE, 0x05); outb(0x3CF, 0x40);
    outb(0x3CE, 0x06); outb(0x3CF, 0x05);
    
    // Установить режим 13h
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(VGA_CTRL_REGISTER, 0x00); outb(VGA_DATA_REGISTER, 0x5F);
    outb(VGA_CTRL_REGISTER, 0x01); outb(VGA_DATA_REGISTER, 0x4F);
    outb(VGA_CTRL_REGISTER, 0x02); outb(VGA_DATA_REGISTER, 0x50);
    outb(VGA_CTRL_REGISTER, 0x03); outb(VGA_DATA_REGISTER, 0x82);
    outb(VGA_CTRL_REGISTER, 0x04); outb(VGA_DATA_REGISTER, 0x54);
    outb(VGA_CTRL_REGISTER, 0x05); outb(VGA_DATA_REGISTER, 0x80);
    outb(VGA_CTRL_REGISTER, 0x06); outb(VGA_DATA_REGISTER, 0x0D);
    outb(VGA_CTRL_REGISTER, 0x07); outb(VGA_DATA_REGISTER, 0x3E);
    outb(VGA_CTRL_REGISTER, 0x08); outb(VGA_DATA_REGISTER, 0x00);
    outb(VGA_CTRL_REGISTER, 0x09); outb(VGA_DATA_REGISTER, 0x40);
    outb(VGA_CTRL_REGISTER, 0x10); outb(VGA_DATA_REGISTER, 0xEA);
    outb(VGA_CTRL_REGISTER, 0x11); outb(VGA_DATA_REGISTER, 0x8C);
    outb(VGA_CTRL_REGISTER, 0x12); outb(VGA_DATA_REGISTER, 0xDF);
    outb(VGA_CTRL_REGISTER, 0x13); outb(VGA_DATA_REGISTER, 0x28);
    outb(VGA_CTRL_REGISTER, 0x14); outb(VGA_DATA_REGISTER, 0x00);
    outb(VGA_CTRL_REGISTER, 0x15); outb(VGA_DATA_REGISTER, 0xE7);
    outb(VGA_CTRL_REGISTER, 0x16); outb(VGA_DATA_REGISTER, 0x04);
    outb(VGA_CTRL_REGISTER, 0x17); outb(VGA_DATA_REGISTER, 0xE3);
    
    // Цветовая палитра
    outb(0x3C8, 0x00);
    for (int i = 0; i < 256; i++) {
        int r = (i >> 5) * 36;
        int g = ((i >> 2) & 7) * 36;
        int b = (i & 3) * 85;
        outb(0x3C9, r);
        outb(0x3C9, g);
        outb(0x3C9, b);
    }
}

// Возврат в текстовый режим
void vga_set_mode_text(void) {
    // Восстановить текстовый режим 80x25
    outb(VGA_CTRL_REGISTER, 0x03); outb(VGA_DATA_REGISTER, 0x00);
    outb(VGA_CTRL_REGISTER, 0x11); outb(VGA_DATA_REGISTER, inb(VGA_DATA_REGISTER) | 0x80);
}

// Рисование пикселя
void vga_put_pixel(int x, int y, unsigned char color) {
    if (x < 0 || x >= 320 || y < 0 || y >= 200) return;
    
    unsigned char* vga = (unsigned char*)0xA0000;
    vga[y * 320 + x] = color;
}

// Заливка экрана цветом
void vga_clear_screen(unsigned char color) {
    unsigned char* vga = (unsigned char*)0xA0000;
    for (int i = 0; i < 330 * 200; i++) {
        vga[i] = color;
    }
}

// Рисование прямоугольника
void vga_draw_rect(int x, int y, int width, int height, unsigned char color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            vga_put_pixel(x + j, y + i, color);
        }
    }
}
