#include "gui.h"
#include "vga.h"

void gui_init(void) {
    // Пока просто очищаем экран
    vga_clear_screen(0x11); // Темно-синий фон
}

void gui_draw_window(Window* win) {
    // Фон окна
    vga_draw_rect(win->x, win->y, win->width, win->height, win->color);
    
    // Рамка окна
    vga_draw_rect(win->x, win->y, win->width, 1, GUI_TEXT_COLOR); // Верх
    vga_draw_rect(win->x, win->y + win->height - 1, win->width, 1, GUI_TEXT_COLOR); // Низ
    vga_draw_rect(win->x, win->y, 1, win->height, GUI_TEXT_COLOR); // Левый
    vga_draw_rect(win->x + win->width - 1, win->y, 1, win->height, GUI_TEXT_COLOR); // Правый
    
    // Заголовок
    vga_draw_rect(win->x + 1, win->y + 1, win->width - 2, 15, GUI_TITLE_BAR);
    
    // Уголки
    vga_draw_rect(win->x, win->y, 3, 3, GUI_TEXT_COLOR);
    vga_draw_rect(win->x + win->width - 3, win->y, 3, 3, GUI_TEXT_COLOR);
    vga_draw_rect(win->x, win->y + win->height - 3, 3, 3, GUI_TEXT_COLOR);
    vga_draw_rect(win->x + win->width - 3, win->y + win->height - 3, 3, 3, GUI_TEXT_COLOR);
}

void gui_draw_button(Button* btn) {
    // Фон кнопки
    u8 bg_color = btn->pressed ? GUI_BUTTON_HOVER : GUI_BUTTON_NORMAL;
    vga_draw_rect(btn->x, btn->y, btn->width, btn->height, bg_color);
    
    // Рамка кнопки
    vga_draw_rect(btn->x, btn->y, btn->width, 1, GUI_TEXT_COLOR);
    vga_draw_rect(btn->x, btn->y + btn->height - 1, btn->width, 1, GUI_TEXT_COLOR);
    vga_draw_rect(btn->x, btn->y, 1, btn->height, GUI_TEXT_COLOR);
    vga_draw_rect(btn->x + btn->width - 1, btn->y, 1, btn->height, GUI_TEXT_COLOR);
    
    // Если бы был шрифт, здесь бы рисовали текст
    // Пока просто рисуем прямоугольник для текста
    if (btn->text) {
        // Просто показываем что есть текст
        vga_draw_rect(btn->x + 5, btn->y + (btn->height/2) - 2, 
                     btn->width - 10, 4, GUI_TEXT_COLOR);
    }
}

void gui_clear_area(int x, int y, int w, int h, u8 color) {
    vga_draw_rect(x, y, w, h, color);
}
