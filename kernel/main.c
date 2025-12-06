#include "defs.h"
#include "memlayout.h"
#include "param.h"
#include "riscv.h"
extern void test_main(void);

int main() {
  // 初始化控制台和打印系统
  consoleinit();
  printfinit();

  printf("=====================================\n");
  printf("  WHU-OSLab xv6 kernel is booting...\n");
  printf("=====================================\n");
  printf("  Hart ID: %d\n", cpuid());
  printf("=====================================\n\n");

  // 初始化内存管理
  printf("[INIT] Initializing physical memory allocator...\n");
  kinit(); // 初始化物理内存分配器
  printf("[INIT] Initializing virtual memory system...\n");
  kvminit(); // 初始化虚拟内存管理
  printf("[INIT] Enabling paging...\n");
  kvminithart(); // turn on paging

  // 初始化进程和中断
  procinit();
  trapinit();
  trapinithart();
  plicinit();
  plicinithart();

  // // 初始化文件系统和设备

  // printf("[INIT] create first user process\n");
  // // 创建用户进程
  // userinit(); // first user process

  // 创建内核线程运行测试（包括新的 sys_hello 系统调用测试）
  printf("[INIT] creating test thread...\n");
  kthread_create(test_main);

  // 进入调度器循环
  printf("[INIT] starting scheduler...\n");
  scheduler();
}