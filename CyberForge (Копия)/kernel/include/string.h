#ifndef STRING_H
#define STRING_H

void memcpy(void* dst, void* src, int len);
int strcmp(const char* a, const char* b);
int strlen(const char* s);
void strcpy(char* dst, const char* src);
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
char* strcat(char* dest, const char* src);
char* strrchr(const char* s, int c);
int strcasecmp(const char* a, const char* b);
void trim_whitespace(char* str);
void memset(void* ptr, int value, int num);

#endif
