//
// kernel console output
//
#include <stdarg.h>

#include "../include/defs.h"
#include "../include/types.h"
#include "../sync/spinlock.h"

#define BACKSPACE 0x100
#define C(x) ((x) - '@') // Control-x

// send one character to the uart.
// called by printf(), and to echo input characters,
// but not from write().
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
  struct spinlock lock;
// input
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} console;

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

void consoleintr(int c) {
  acquire(&console.lock);

  switch (c) {
  case C('P'): // Print process list.
    // procdump();
    break;
  case C('U'): // Kill line.
    while (console.e != console.w &&
           console.buf[(console.e - 1) % INPUT_BUF_SIZE] != '\n') {
      console.e--;
      consputc(BACKSPACE);
    }
    break;
  case C('H'): // Backspace
  case '\x7f': // Delete key
    if (console.e != console.w) {
      console.e--;
      consputc(BACKSPACE);
    }
    break;
  default:
    if (c != 0 && console.e - console.r < INPUT_BUF_SIZE) {
      c = (c == '\r') ? '\n' : c;

      // echo back to the user.
      consputc(c);

      // store for consumption by consoleread().
      console.buf[console.e++ % INPUT_BUF_SIZE] = c;

      if (c == '\n' || c == C('D') || console.e - console.r == INPUT_BUF_SIZE) {
        // wake up consoleread() if a whole line (or end-of-file)
        // has arrived.
        console.w = console.e;
        // wakeup(&console.r);
      }
    }
    break;
  }

  release(&console.lock);
}

void consoleinit(void) {
  initlock(&console.lock, "console");
  uartinit();
}
