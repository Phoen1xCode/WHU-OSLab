#include "defs.h"
#include "types.h"
#include "riscv.h"

void start(void) {
  printf("\n");
  printf("=====================================\n");
  printf("  WHU-OSLab Booting...\n");
  printf("=====================================\n");
  printf("  Hart ID: %lu\n", r_mhartid());
  printf("=====================================\n\n");
  
  // 初始化物理内存分配器
  printf("[INIT] Initializing physical memory allocator...\n");
  kinit();
  
  // 初始化虚拟内存管理
  printf("[INIT] Initializing virtual memory system...\n");
  kvminit();
  
  printf("[INIT] Enabling paging...\n");
  kvminithart();
  

  printf("=====================================\n");
  printf("  System Halted\n");
  printf("=====================================\n");
  printf("  Press Ctrl-A then X to exit QEMU\n");
  printf("=====================================\n\n");
  
  // 进入空闲循环
  for (;;) {
    __asm__ volatile("wfi");
  }
}
