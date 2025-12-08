//
// kernel printf and console output
//
#include <stdarg.h>

#include "../include/defs.h"
#include "../include/types.h"
#include "../sync/spinlock.h"

volatile int panicking = 0; // printing a panic message
volatile int panicked = 0;  // spinning forever at end of a panic

static struct {
  struct spinlock lock;
} pr;

static char digits[] = "0123456789abcdef";

// print integer
static void printint(long long xx, int base, int sign) {
  /*
   * xx: the number to print
   * base: 10 for decimal, 16 for hex
   * sign: 1 if signed, 0 if unsigned
   */

  char buf[20];
  int i = 0; // index
  unsigned long long x;

  if (sign && (sign = (xx < 0)))
    x = -xx;
  else
    x = xx;

  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign) {
    buf[i++] = '-';
  }

  while (--i >= 0) {
    consputc(buf[i]);
  }
}

// print the 64-bit pointer value in hexadecimal format.
static void printptr(uint64 x) {
  /*
   * x: the pointer value to print
   */
  int i;
  consputc('0');
  consputc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4) {
    consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
  }
}

// 格式化输出函数
int printf(const char *fmt, ...) {
  va_list ap;
  int i, cx, c0, c1, c2;
  char *s;

  va_start(ap, fmt);
  for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
    if (cx != '%') { // 普通字符，直接输出
      consputc(cx);
      continue;
    }
    i++;

    c0 = fmt[i + 0] & 0xff;
    c1 = c2 = 0;
    if (c0)
      c1 = fmt[i + 1] & 0xff;
    if (c1)
      c2 = fmt[i + 2] & 0xff;

    if (c0 == 'd') { // int 32bit
      printint(va_arg(ap, int), 10, 1);
    } else if (c0 == 'l' && c1 == 'd') { // long 64bit
      printint(va_arg(ap, uint64), 10, 1);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') { // long long 64bit
      printint(va_arg(ap, uint64), 10, 1);
    } else if (c0 == 'u') { // unsigned int 32bit
      printint(va_arg(ap, uint32), 10, 0);
    } else if (c0 == 'l' && c1 == 'u') { // unsigned long 64bit
      printint(va_arg(ap, uint64), 10, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' &&
               c2 == 'u') { // unsigned long long 64bit
      printint(va_arg(ap, uint64), 10, 0);
      i += 2;
    } else if (c0 == 'x') { // hex unsigned int 32bit
      printint(va_arg(ap, uint32), 16, 0);
    } else if (c0 == 'l' && c1 == 'x') { // hex unsigned long 64bit
      printint(va_arg(ap, uint64), 16, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' &&
               c2 == 'x') { // hex unsigned long long 64bit
      printint(va_arg(ap, uint64), 16, 0);
      i += 2;
    } else if (c0 == 'p') { // pointer
      printptr(va_arg(ap, uint64));
    } else if (c0 == 'c') { // char
      consputc(va_arg(ap, uint));
    } else if (c0 == 's') { // string
      if ((s = va_arg(ap, char *)) == 0) {
        s = "(null)";
      }
      for (; *s; s++) {
        consputc(*s);
      }
    } else if (c0 == '%') { // %%
      consputc('%');
    } else if (c0 == 0) { // end of format string
      break;
    } else { // unknown % sequence, print it as is
      consputc('%');
      consputc(c0);
    }
  }
  va_end(ap);

  // if (panicking == 0)
  //   release(&pr.lock);

  return 0;
}

void panic(char *s) {
  panicking = 1;
  printf("panic: ");
  printf("%s\n", s);
  panicked = 1;
  for (;;)
    ;
}

void printfinit(void) { initlock(&pr.lock, "pr"); }

// Test functions
/*
void test_printf_basic() {
  printf("Testing integer: %d\n", 42);
  printf("Testing negative: %d\n", -123);
  printf("Testing zero: %d\n", 0);
  printf("Testing hex: 0x%x\n", 0xABC);
  printf("Testing string: %s\n", "Hello");
  printf("Testing char: %c\n", 'X');
  printf("Testing percent: %%\n");
}

void test_printf_edge_cases() {
  printf("INT_MAX: %d\n", 2147483647);
  printf("INT_MIN: %d\n", -2147483648);
  printf("NULL string: %s\n", (char *)0);
  printf("Empty string: %s\n", "");
}

void test_clear_screen() {
  printf("Before clear\n");
  clear_screen();
  printf("After clear\n");
}
*/