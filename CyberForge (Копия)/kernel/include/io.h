#ifndef IO_H
#define IO_H

#include "types.h"

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0,%1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char r;
    __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(port));
    return r;
}

static inline u16 inw(unsigned short port) {
    u16 r;
    __asm__ volatile("inw %1,%0" : "=a"(r) : "Nd"(port));
    return r;
}

static inline void outw(unsigned short port, u16 val) {
    __asm__ volatile("outw %0,%1" : : "a"(val), "Nd"(port));
}

#endif
