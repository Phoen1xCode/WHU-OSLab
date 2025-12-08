#include "../include/types.h"

void *memset(void *dst, int c, uint n) {
  char *cdst = (char *)dst;
  int i;
  for (i = 0; i < n; i++) {
    cdst[i] = c;
  }
  return dst;
}

int memcmp(const void *v1, const void *v2, uint n) {
  const uchar *s1, *s2;

  s1 = v1;
  s2 = v2;
  while (n-- > 0) {
    if (*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

void *memmove(void *dst, const void *src, uint n) {
  const char *s;
  char *d;

  if (n == 0)
    return dst;

  s = src;
  d = dst;
  if (s < d && s + n > d) {
    s += n;
    d += n;
    while (n-- > 0)
      *--d = *--s;
  } else
    while (n-- > 0)
      *d++ = *s++;

  return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void *memcpy(void *dst, const void *src, uint n) {
  return memmove(dst, src, n);
}

int strncmp(const char *p, const char *q, uint n) {
  while (n > 0 && *p && *p == *q)
    n--, p++, q++;
  if (n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

char *strncpy(char *s, const char *t, int n) {
  char *os;

  os = s;
  while (n-- > 0 && (*s++ = *t++) != 0)
    ;
  while (n-- > 0)
    *s++ = 0;
  return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char *safestrcpy(char *s, const char *t, int n) {
  char *os;

  os = s;
  if (n <= 0)
    return os;
  while (--n > 0 && (*s++ = *t++) != 0)
    ;
  *s = 0;
  return os;
}

int strlen(const char *s) {
  int n;

  for (n = 0; s[n]; n++)
    ;
  return n;
}

// Compare two strings
int strcmp(const char *p, const char *q) {
  while (*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

// Format string with variable arguments (simplified version)
// Only supports %d, %u, %x, %s, %c
int snprintf(char *buf, int size, const char *fmt, ...) {
  int *argp;
  char *s;
  int c;
  int i, j;
  int base;
  uint x;
  char digits[] = "0123456789abcdef";
  char tmp[32];
  int len = 0;

  if (size <= 0)
    return 0;

  argp = (int *)(&fmt + 1);
  for (i = 0; fmt[i] && len < size - 1; i++) {
    if (fmt[i] != '%') {
      buf[len++] = fmt[i];
      continue;
    }
    i++;
    switch (fmt[i]) {
    case 'd': // signed decimal
      x = *argp++;
      if ((int)x < 0) {
        if (len < size - 1)
          buf[len++] = '-';
        x = -(int)x;
      }
      j = 0;
      do {
        tmp[j++] = digits[x % 10];
      } while ((x /= 10) != 0);
      while (j > 0 && len < size - 1) {
        buf[len++] = tmp[--j];
      }
      break;
    case 'u': // unsigned decimal
    case 'x': // hexadecimal
      base = (fmt[i] == 'x') ? 16 : 10;
      x = *argp++;
      j = 0;
      do {
        tmp[j++] = digits[x % base];
      } while ((x /= base) != 0);
      while (j > 0 && len < size - 1) {
        buf[len++] = tmp[--j];
      }
      break;
    case 's': // string
      s = (char *)*argp++;
      if (s == 0)
        s = "(null)";
      while (*s && len < size - 1) {
        buf[len++] = *s++;
      }
      break;
    case 'c': // character
      c = *argp++;
      if (len < size - 1)
        buf[len++] = c;
      break;
    case '%':
      if (len < size - 1)
        buf[len++] = '%';
      break;
    default:
      if (len < size - 1)
        buf[len++] = '%';
      if (len < size - 1)
        buf[len++] = fmt[i];
      break;
    }
  }
  buf[len] = '\0';
  return len;
}
