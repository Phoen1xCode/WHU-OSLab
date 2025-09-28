#include "types.h"
// console.c
void consputc(int c);
void consputs(const char *s);
void clear_screen(void);

// printf.c
int printf(const char *fmt, ...);
void panic(char *) __attribute__((noreturn));


// uart.c
void uartputc(char c);
void uartputs(const char *s);


// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);
