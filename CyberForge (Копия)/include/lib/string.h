#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int strlen(const char* s);
void strcpy(char* dst, const char* src);
int strcmp(const char* a, const char* b);
int strcasecmp(const char* a, const char* b);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
char* strcat(char* dest, const char* src);
void trim_whitespace(char* str);
void memcpy(void* dst, void* src, int len);
void memset(void* ptr, int value, int num);

#endif
