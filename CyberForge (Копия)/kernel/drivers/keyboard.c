#include "../include/keyboard.h"
#include "../include/io.h"

/* Состояние клавиш */
static unsigned char shift_pressed = 0;
static unsigned char caps_lock = 0;
static unsigned char ctrl_pressed = 0;
static unsigned char alt_pressed = 0;

/* Таблицы сканов - ТОЛЬКО 59 элементов как было раньше */
static const char normal_tbl[59] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

static const char shift_tbl[59] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

/* Функция для получения символа с учетом Shift/Caps */
char keyboard_getchar() {
    static unsigned char extended = 0;
    
    while(1) {
        if(inb(0x64) & 1) {
            unsigned char sc = inb(0x60);
            
            /* ОБРАБОТКА ОТПУСКАНИЯ КЛАВИШ */
            if(sc & 0x80) {
                sc &= 0x7F; // Убираем бит отпускания
                
                if(sc == 0x2A || sc == 0x36) { // Shift
                    shift_pressed = 0;
                }
                else if(sc == 0x1D) { // Ctrl
                    ctrl_pressed = 0;
                }
                else if(sc == 0x38) { // Alt
                    alt_pressed = 0;
                }
                continue;
            }
            
            /* ОБРАБОТКА НАЖАТИЯ КЛАВИШ */
            
            // Расширенные клавиши (стрелки)
            if(sc == 0xE0) {
                extended = 1;
                continue;
            }
            
            // Модификаторы
            if(sc == 0x2A || sc == 0x36) { // Left/Right Shift
                shift_pressed = 1;
                continue;
            }
            else if(sc == 0x1D) { // Ctrl
                ctrl_pressed = 1;
                continue;
            }
            else if(sc == 0x38) { // Alt
                alt_pressed = 1;
                continue;
            }
            else if(sc == 0x3A) { // Caps Lock
                caps_lock = !caps_lock;
                continue;
            }
            
            /* ОБРАБОТКА СТРЕЛОК */
            if(extended) {
                extended = 0;
                switch(sc) {
                    case 0x48: return 0x01; // Up
                    case 0x50: return 0x02; // Down
                    case 0x4B: return 0x03; // Left
                    case 0x4D: return 0x04; // Right
                    case 0x47: return 0x05; // Home
                    case 0x4F: return 0x06; // End
                }
                continue;
            }
            
            /* ОБЫЧНЫЕ СИМВОЛЫ */
            if(sc < 59) {
                char result = 0;
                
                // Выбираем таблицу в зависимости от Shift
                if(shift_pressed) {
                    result = shift_tbl[sc];
                } else {
                    result = normal_tbl[sc];
                }
                
                // Обработка Caps Lock (только для букв)
                if(caps_lock && result >= 'a' && result <= 'z') {
                    if(!shift_pressed) {
                        result = result - 'a' + 'A'; // Верхний регистр
                    } else {
                        result = result - 'A' + 'a'; // Нижний регистр при нажатом Shift
                    }
                }
                
                // Обработка Ctrl+символ
                if(ctrl_pressed && result >= 'a' && result <= 'z') {
                    result = result - 'a' + 1; // Ctrl+A = 0x01 и т.д.
                }
                
                if(result != 0) {
                    return result;
                }
            }
        }
    }
}

/* Получение состояния модификаторов */
unsigned char get_shift_state() {
    return shift_pressed;
}

unsigned char get_caps_state() {
    return caps_lock;
}

unsigned char get_ctrl_state() {
    return ctrl_pressed;
}

unsigned char get_alt_state() {
    return alt_pressed;
}

char getch_with_arrows() {
    return keyboard_getchar();
}

unsigned char get_key() {
    while(!(inb(0x64) & 1));
    return inb(0x60);
}

/* Сброс состояния клавиатуры */
void keyboard_reset() {
    shift_pressed = 0;
    caps_lock = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
}
