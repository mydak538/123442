#ifndef STDLIB_H
#define STDLIB_H

int atoi(const char* s);
void itoa(int value, char* str, int base);
int rand(void);
void srand(unsigned int seed);
void delay(int seconds);

#endif
