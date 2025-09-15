#include "types.h"

void uart_puts(char *s);

void start(void) {
    uart_puts("Hello OS\n");
    for (;;) {
        __asm__ volatile ("wfi");
    }
}


