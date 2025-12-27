//
// kernel printf and console output
//
#include "../include/defs.h"
#include "../include/types.h"
#include <stdarg.h>

volatile int panicking = 0; // printing a panic message
volatile int panicked = 0;  // spinning forever at end of a panic

static char digits[] = "0123456789abcdef";

// print integer
void print_int(long long xx, int base, int sign) {
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
    console_putc(buf[i]);
  }
}

// print the 64-bit pointer value in hexadecimal format.
void print_ptr(uint64 x) {
  /*
   * x: the pointer value to print
   */
  int i;
  console_putc('0');
  console_putc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4) {
    console_putc(digits[x >> (sizeof(uint64) * 8 - 4)]);
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
      console_putc(cx);
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
      print_int(va_arg(ap, int), 10, 1);
    } else if (c0 == 'l' && c1 == 'd') { // long 64bit
      print_int(va_arg(ap, uint64), 10, 1);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') { // long long 64bit
      print_int(va_arg(ap, uint64), 10, 1);
    } else if (c0 == 'u') { // unsigned int 32bit
      print_int(va_arg(ap, uint32), 10, 0);
    } else if (c0 == 'l' && c1 == 'u') { // unsigned long 64bit
      print_int(va_arg(ap, uint64), 10, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' &&
               c2 == 'u') { // unsigned long long 64bit
      print_int(va_arg(ap, uint64), 10, 0);
      i += 2;
    } else if (c0 == 'x') { // hex unsigned int 32bit
      print_int(va_arg(ap, uint32), 16, 0);
    } else if (c0 == 'l' && c1 == 'x') { // hex unsigned long 64bit
      print_int(va_arg(ap, uint64), 16, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' &&
               c2 == 'x') { // hex unsigned long long 64bit
      print_int(va_arg(ap, uint64), 16, 0);
      i += 2;
    } else if (c0 == 'p') { // pointer
      print_ptr(va_arg(ap, uint64));
    } else if (c0 == 'c') { // char
      console_putc(va_arg(ap, uint));
    } else if (c0 == 's') { // string
      if ((s = va_arg(ap, char *)) == 0) {
        s = "(null)";
      }
      for (; *s; s++) {
        console_putc(*s);
      }
    } else if (c0 == '%') { // %%
      console_putc('%');
    } else if (c0 == 0) { // end of format string
      break;
    } else { // unknown % sequence, print it as is
      console_putc('%');
      console_putc(c0);
    }
  }
  va_end(ap);

  return 0;
}

void panic(char *s) {
  printf("panic: ");
  printf("%s\n", s);
  for (;;)
    ;
}
