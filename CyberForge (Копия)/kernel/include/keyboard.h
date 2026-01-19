#ifndef KEYBOARD_H
#define KEYBOARD_H

char keyboard_getchar();
char getch_with_arrows();
unsigned char get_key();

/* Функции для получения состояния клавиш */
unsigned char get_shift_state();
unsigned char get_caps_state();
unsigned char get_ctrl_state();
unsigned char get_alt_state();

/* Сброс состояния */
void keyboard_reset();

#endif
