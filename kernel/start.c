#include "defs.h"
#include "types.h"

void start(void) {
  printf("Booting... %d %x %s %c %%\n", 42, 0x2a, "ok", 'A');
  clear_screen();
  printf("Screen cleared. Hello OS!\n");
  for (;;) {
    __asm__ volatile("wfi");
  }
}
