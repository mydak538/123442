#include "../include/string.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

int strlen(const char* s) {
    int len = 0;
    while(s[len]) len++;
    return len;
}

void strcpy(char* dst, const char* src) {
    while((*dst++ = *src++));
}

int strcmp(const char* a, const char* b) {
    while(*a && *a == *b) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

void memcpy(void* dst, void* src, int len) {
    char* d = dst;
    char* s = src;
    while(len--) *d++ = *s++;
}

void memset(void* ptr, int value, int num) {
    unsigned char* p = ptr;
    while(num--) *p++ = value;
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest;
    while(*ptr) ptr++;
    while((*ptr++ = *src++));
    return dest;
}

char* strchr(const char* s, int c) {
    while(*s) {
        if(*s == (char)c) return (char*)s;
        s++;
    }
    return NULL;
}

char* strrchr(const char* s, int c) {
    const char* last = NULL;
    while(*s) {
        if(*s == (char)c) last = s;
        s++;
    }
    return (char*)last;
}

char* strstr(const char* haystack, const char* needle) {
    if(!*needle) return (char*)haystack;
    for(const char* p = haystack; *p; p++) {
        const char* h = p, *n = needle;
        while(*h && *n && *h == *n) { h++; n++; }
        if(!*n) return (char*)p;
    }
    return NULL;
}

/* strncpy - копирует не более n символов */
char* strncpy(char* dest, const char* src, int n) {
    char* ptr = dest;
    int i = 0;
    
    while(i < n && src[i]) {
        dest[i] = src[i];
        i++;
    }
    
    while(i < n) {
        dest[i] = '\0';
        i++;
    }
    
    return ptr;
}

