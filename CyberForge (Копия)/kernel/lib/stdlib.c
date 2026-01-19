#include "../include/stdlib.h"

int atoi(const char* s) {
    int r = 0;
    while(*s >= '0' && *s <= '9') {
        r = r * 10 + (*s - '0');
        s++;
    }
    return r;
}

void itoa(int value, char* str, int base) {
    char* ptr = str;
    if(value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    while(value > 0) {
        int digit = value % base;
        *ptr++ = digit < 10 ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    *ptr = '\0';
    // reverse
    char* start = str;
    char* end = ptr - 1;
    while(start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}

void delay(int seconds) {
    for(volatile int i = 0; i < seconds * 1000000; i++);
}

static unsigned int rand_seed = 1;

int rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}
