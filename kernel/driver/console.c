//
// kernel console output
//
#include "../include/defs.h"
#include <stdarg.h>

#define BACKSPACE 0x100
#define C(x) ((x) - '@') // Control-x

// send one character to the uart.
// called by printf(), and to echo input characters,
// but not from write().
void console_putc(int c) {
  if (c == BACKSPACE) {
    // if user typed backspace, overwrite with a space.
    uart_putc('\b');
    uart_putc(' ');
    uart_putc('\b');
  } else {
    uart_putc(c);
  }
}

void console_puts(const char *s) {
  while (*s) {
    console_putc(*s);
    s++;
  }
}

// Clear screen via ANSI escape sequences: ESC[2J (clear) and ESC[H (home)
void clear_screen(void) {
  console_puts("\033[2J"); // 清除整个屏幕
  console_puts("\033[H");  // 将光标移到左上角
}