#include "defs.h"
#include "memlayout.h"
#include "param.h"
#include "riscv.h"

int main() {
  // 初始化控制台和打印系统
  consoleinit();
  printfinit();

  printf("=====================================\n");
  printf("  WHU-OSLab xv6 kernel is booting...\n");
  printf("=====================================\n");
  printf("  Hart ID: %lu\n", r_mhartid());
  printf("=====================================\n\n");

  // 初始化内存管理
  printf("[INIT] Initializing physical memory allocator...\n");
  kinit(); // 初始化物理内存分配器
  printf("[INIT] Initializing virtual memory system...\n");
  kvminit(); // 初始化虚拟内存管理
  printf("[INIT] Enabling paging...\n");
  kvminithart(); // turn on paging

  // 初始化进程和中断
  // procinit();
  // trapinit();

  // 初始化文件系统和设备

  // 创建用户进程
  // userinit(); // first user process

  // 进入调度器循环
  // scheduler();
}