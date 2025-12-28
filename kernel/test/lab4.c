#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../proc/proc.h"

#define IRQ_TIMER 1

// 读取 time CSR
unsigned long get_time(void) { return r_time(); }

// 计时器中断处理函数
volatile int interrupt_count = 0;

void timer_interrupt_handler(void) {
  interrupt_count++;
  printf("Timer interrupt triggered! Count: %d\n", interrupt_count);

  // 设置下一次时钟中断
  w_stimecmp(r_time() + 1000000);
}

void test_timer_interrupt(void) {
  printf("Testing timer interrupt...\n");

  // 重置中断计数
  interrupt_count = 0;

  // 注册时钟中断处理函数
  register_interrupt(IRQ_TIMER, timer_interrupt_handler);

  // 开启时钟中断
  enable_interrupt(IRQ_TIMER);

  // 先安排第一次时钟中断
  unsigned long start_time = get_time();
  w_stimecmp(start_time + 1000000); // 1M cycles 后触发

  // 等待 5 次中断
  while (interrupt_count < 5) {
    printf("Waiting for interrupt %d...\n", interrupt_count + 1);
    // 简单延时
    for (volatile int i = 0; i < 10000000; i++)
      ;
  }

  // 记录中断后的时间
  unsigned long end_time = get_time();

  // 打印测试结果
  printf("Timer test completed:\n");
  printf("Start time: %lx\n", start_time);
  printf("End time: %lx\n", end_time);
  printf("Total interrupts: %d\n", interrupt_count);

  // 关闭时钟中断
  disable_interrupt(IRQ_TIMER);

  // 注销时钟中断处理函数
  unregister_interrupt(IRQ_TIMER);
}

void test_exception_handling(void) {
  printf("Testing exception handling...\n");

  // 非法指令异常
  printf("Illegal Instruction Test\n");
  asm volatile(".word 0xFFFFFFFF\n"
               "nop\n"
               "nop\n"
               "nop\n");

  // 加载页异常
  printf("Load Page Fault Test\n");
  volatile unsigned long *bad_load = (unsigned long *)0xFFFFFFFF00000000UL;
  unsigned long bad_value = *bad_load;
  (void)bad_value;
  asm volatile("nop\nnop\nnop\n");

  // 存储页异常
  printf("Store Page Fault Test\n");
  volatile unsigned long *bad_store = (unsigned long *)0xFFFFFFFF00000000UL;
  *bad_store = 0x66;
  asm volatile("nop\nnop\nnop\n");

  printf("Exception tests completed\n");
}

void pt_init(void) {
  // 内存管理初始化
  kmem_init();
  kvm_init();
  kvm_inithart();
}
