#ifndef PORTS_H
#define PORTS_H

#include "types.h"

/* Порт ввода/вывода */
static inline u8 inb(u16 port) {
    u8 data;
    asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void outb(u16 port, u8 data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 data;
    asm volatile("inw %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void outw(u16 port, u16 data) {
    asm volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

#endif
