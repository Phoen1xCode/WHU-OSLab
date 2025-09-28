//
// kernel console output
//
#include <stdarg.h>

#include "defs.h"
#include "types.h"

#define BACKSPACE 0x100
#define C(x) ((x) - '@') // Control-x

void consputc(int c) {
  if (c == BACKSPACE) {
    // if user typed backspace, overwrite with a space.
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
  }

  uartputc(c);
}

void consputs(const char *s) {
  while (*s) {
    consputc(*s);
    s++;
  }
}

// Clear screen via ANSI escape sequences: ESC[2J (clear) and ESC[H (home)
void clear_screen(void) { consputs("\033[2J\033[H"); }

struct {
// input
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} console;

//
// user write()s to the console go here.
// Can't understand this function
int consolewrite(int user_src, uint64 src, int n) {
  (void)user_src; // this minimal kernel treats src as kernel pointer
  const char *p = (const char *)src;
  int i;
  for (i = 0; i < n; i++) {
    consputc((unsigned char)p[i]);
  }
  return n;
}

void consoleinit(void) {}

void consoleintr(int c) {}