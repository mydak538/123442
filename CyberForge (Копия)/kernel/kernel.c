#include "include/types.h"
#include "include/screen.h"
#include "include/keyboard.h"
#include "include/commands.h"
#include "include/string.h"
#include "include/fs.h"
#include "ports.h"
#include "vga.h"
#include "gui.h"

/* Multiboot header */
__attribute__((section(".multiboot")))
const unsigned int multiboot_header[] = {
    0x1BADB002,
    0x00000003,
    -(0x1BADB002 + 0x00000003)
};

/* Command history */
#define MAX_HISTORY 10
char command_history[MAX_HISTORY][128];
int history_count = 0;
int history_index = -1;

/* Объявление current_dir из fs.c */
extern char current_dir[];

/* ГЛУБОКАЯ очистка всего VGA буфера */
void deep_clear_screen() {
    for(int i = 0; i < ROWS * COLS * 2; i++) {
        VGA[i] = (unsigned short)(' ' | (0x07 << 8));
    }
    cursor_row = 0;
    cursor_col = 0;
    text_color = 0x07;
}

/* Функции истории команд */
void add_to_history(const char* cmd) {
    if(strlen(cmd) == 0) return;
    
    if(history_count == 0 || strcmp(command_history[history_count-1], cmd) != 0) {
        if(history_count < MAX_HISTORY) {
            strcpy(command_history[history_count], cmd);
            history_count++;
        } else {
            for(int i = 0; i < MAX_HISTORY-1; i++) {
                strcpy(command_history[i], command_history[i+1]);
            }
            strcpy(command_history[MAX_HISTORY-1], cmd);
        }
    }
    history_index = history_count;
}

void show_prev_history(char* buffer, int* index) {
    if(history_count == 0) return;
    
    if(history_index > 0) history_index--;
    if(history_index >= 0 && history_index < history_count) {
        strcpy(buffer, command_history[history_index]);
        *index = strlen(buffer);
        
        prints("\r");
        set_color(COLOR_LIGHT_GREEN);
        prints("CyberForge:");
        set_color(COLOR_CYAN);
        prints(current_dir);
        set_color(COLOR_LIGHT_GREEN);
        prints("> ");
        set_color(COLOR_WHITE);
        prints(buffer);
    }
}

void show_next_history(char* buffer, int* index) {
    if(history_count == 0) return;
    
    if(history_index < history_count-1) {
        history_index++;
        strcpy(buffer, command_history[history_index]);
        *index = strlen(buffer);
    } else {
        history_index = history_count;
        buffer[0] = '\0';
        *index = 0;
    }
    
    prints("\r");
    set_color(COLOR_LIGHT_GREEN);
    prints("CyberForge:");
    set_color(COLOR_CYAN);
    prints(current_dir);
    set_color(COLOR_LIGHT_GREEN);
    prints("> ");
    set_color(COLOR_WHITE);
    prints(buffer);
    for(int i = *index; i < *index + 10; i++) putchar(' ');
}

/* Показать индикаторы клавиш */
void show_key_indicators() {
    int start_pos = 70;
    
    if(get_caps_state()) {
        VGA[(ROWS-1) * COLS + start_pos] = (unsigned short)('C' | (COLOR_LIGHT_RED << 8));
        VGA[(ROWS-1) * COLS + start_pos+1] = (unsigned short)('A' | (COLOR_LIGHT_RED << 8));
        VGA[(ROWS-1) * COLS + start_pos+2] = (unsigned short)('P' | (COLOR_LIGHT_RED << 8));
        VGA[(ROWS-1) * COLS + start_pos+3] = (unsigned short)('S' | (COLOR_LIGHT_RED << 8));
    } else {
        VGA[(ROWS-1) * COLS + start_pos] = (unsigned short)(' ' | (COLOR_GRAY << 8));
        VGA[(ROWS-1) * COLS + start_pos+1] = (unsigned short)(' ' | (COLOR_GRAY << 8));
        VGA[(ROWS-1) * COLS + start_pos+2] = (unsigned short)(' ' | (COLOR_GRAY << 8));
        VGA[(ROWS-1) * COLS + start_pos+3] = (unsigned short)(' ' | (COLOR_GRAY << 8));
    }
}

/* Точка входа ядра */
void kernel_main(void) {
    // ВАЖНО: Сразу выводим текст, чтобы QEMU не ушел в blank mode
    VGA[0] = (unsigned short)('C' | (0x07 << 8));  // Просто символ 'C' в начало
    
    // Полная очистка экрана
    deep_clear_screen();
    // АНГЛИЙСКОЕ приветствие
    set_color(COLOR_LIGHT_GREEN);
    prints("               CyberForge v1.3              \n\n");
    
    set_color(COLOR_CYAN);
    prints("Welcome to your own operating system!\n\n");
    
    set_color(COLOR_GRAY);
    prints("              _      _\n");
    prints("             (c\\-.--/a)\n");
    prints("              |q: p   /\\_            _____\n");
    prints("            __\\(_/  ).'  '---._.---'`     '---.__\n");
    prints("           /  (Y_)_/             /        : \\-._ \\\n");
    prints("   !!!!,,, \\_))'-';             (       _/   \\  '\\\\_\n");
    prints("  !!II!!!!!IIII,, \\_             \\     /      \\_  '.\\\n");
    prints("   !IIsndIIIII!!!!,,\\     /_      \\   |----.___ '-. \\'.__\n");
    prints("   !!!IIIIIIIIIIIIIIII\\   | '--._.-'  _)       \\  |  `'--'\n");
    prints("       '''!!!!IIIIIII/   .',, ((___.-'         / /\n");
    prints("             '''!!!!/  _/!!!!IIIIIII!!!!!,,,,,;,;,,,.....\n");
    prints("                   | /IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII\n");
    prints("                   | \\   ''IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII\n");
    prints("                   \\_,)     '''''!!!!IIIIIIIIIIIIIIII!!!!!!!!\n");
    prints("                                     ''''''''''!!!!!!!!!!!!!!!\n\n");
    
    set_color(COLOR_LIGHT_GREEN);
    prints("System initializing...\n");
    vga_set_mode_13h();
    
    // Инициализируем GUI
    gui_init();
    
    // Создаем окно
    Window main_window = {
        .x = 40,
        .y = 30,
        .width = 240,
        .height = 140,
        .color = GUI_WINDOW_BG,
        .active = 1
    };
    
    // Рисуем окно
    gui_draw_window(&main_window);
    
    // Создаем кнопки
    Button btn1 = {
        .x = 60,
        .y = 60,
        .width = 80,
        .height = 25,
        .text = "File",
        .pressed = 0
    };
    
    Button btn2 = {
        .x = 150,
        .y = 60,
        .width = 80,
        .height = 25,
        .text = "Edit",
        .pressed = 0
    };
    
    Button btn3 = {
        .x = 60,
        .y = 100,
        .width = 170,
        .height = 25,
        .text = "Run Command",
        .pressed = 1  // "Нажатая" кнопка
    };
    
    // Рисуем кнопки
    gui_draw_button(&btn1);
    gui_draw_button(&btn2);
    gui_draw_button(&btn3);
    
    // Рисуем панель внизу
    vga_draw_rect(0, 190, 320, 10, 0x01); // Темно-синяя панель
    
    // Рисуем иконки (простые квадраты)
    vga_draw_rect(5, 192, 6, 6, VGA_RED);     // Красный квадрат
    vga_draw_rect(15, 192, 6, 6, VGA_GREEN);  // Зеленый
    vga_draw_rect(25, 192, 6, 6, VGA_BLUE);   // Синий
    vga_draw_rect(35, 192, 6, 6, VGA_YELLOW); // Желтый
    
    // Рисуем курсор мыши (простой крестик)
    int mouse_x = 160;
    int mouse_y = 120;
    vga_draw_rect(mouse_x - 1, mouse_y, 3, 1, VGA_WHITE); // Горизонтальная
    vga_draw_rect(mouse_x, mouse_y - 1, 1, 3, VGA_WHITE); // Вертикальная
    
    set_color(COLOR_GREEN);
    prints("System ready!\n\n");
    
    char cmd_buf[128];
    int cmd_idx = 0;
    
    while (1) {
        show_key_indicators();
        
        // Показываем приглашение с директорией
        set_color(COLOR_LIGHT_GREEN);
        prints("CyberForge:");
        set_color(COLOR_CYAN);
        prints(current_dir);
        set_color(COLOR_LIGHT_GREEN);
        prints("> ");
        set_color(COLOR_WHITE);
        
        cmd_idx = 0;
        cmd_buf[0] = '\0';
        history_index = history_count;
        
        while (1) {
            char c = keyboard_getchar();
            
            show_key_indicators();
            
            // Стрелки
            if(c == 0x01) {
                show_prev_history(cmd_buf, &cmd_idx);
                continue;
            }
            if(c == 0x02) {
                show_next_history(cmd_buf, &cmd_idx);
                continue;
            }
            if(c == 0x05) {
                cmd_idx = 0;
                cmd_buf[0] = '\0';
                prints("\r");
                set_color(COLOR_LIGHT_GREEN);
                prints("CyberForge:");
                set_color(COLOR_CYAN);
                prints(current_dir);
                set_color(COLOR_LIGHT_GREEN);
                prints("> ");
                set_color(COLOR_WHITE);
                continue;
            }
            
            // Обычные символы
            putchar(c);
            
            if(c == '\n') {
                cmd_buf[cmd_idx] = '\0';
                newline();
                if(cmd_idx > 0) {
                    add_to_history(cmd_buf);
                    run_command(cmd_buf);
                }
                break;
            } else if(c == '\b') {
                if(cmd_idx > 0) {
                    cmd_idx--;
                    cmd_buf[cmd_idx] = '\0';
                }
            } else if(c >= 32 && c <= 126 && cmd_idx < 127) {
                cmd_buf[cmd_idx] = c;
                cmd_idx++;
                cmd_buf[cmd_idx] = '\0';
            }
            // Ctrl+C
            else if(c == 0x03) {
                prints("^C\n");
                cmd_idx = 0;
                cmd_buf[0] = '\0';
                break;
            }
        }
    }
}
