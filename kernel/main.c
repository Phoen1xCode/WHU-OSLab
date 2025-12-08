#include "./include/defs.h"
#include "./include/memlayout.h"
#include "./include/param.h"
#include "./include/riscv.h"

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
  printf("[INIT] Initializing process management...\n");
  procinit();
  trapinit();
  trapinithart();
  plicinit();
  plicinithart();

  // 初始化文件系统和设备
  printf("[INIT] Initializing block cache...\n");
  binit(); // buffer cache
  printf("[INIT] Initializing inode table...\n");
  iinit(); // inode table
  printf("[INIT] Initializing file table...\n");
  fileinit(); // file table
  printf("[INIT] Initializing virtio disk...\n");
  virtio_disk_init(); // emulated hard disk
  printf("[INIT] Initializing file system...\n");
  fsinit(ROOTDEV); // file system

  // printf("[INIT] create first user process\n");
  // // 创建用户进程
  // userinit(); // first user process

  // 创建内核线程运行测试（包括新的 sys_hello 系统调用测试）
  printf("[INIT] creating test thread...\n");
  // kthread_create(test_main);

  // 进入调度器循环
  printf("[INIT] starting scheduler...\n");
  scheduler();
}