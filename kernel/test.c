#include "defs.h"
#include "riscv.h"
#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"

// Global variables for testing
volatile int test_timer_flag = 0;
volatile int test_exception_flag = 0;

// Defined in trap.c
extern int test_mode;
extern uint64 interrupt_cycles;

uint64 get_time() { return r_time(); }

void test_timer_interrupt(void) {
  printf("Testing timer interrupt...\n");

  // 记录中断前的时间
  uint64 start_time = get_time();
  int interrupt_count = 0;

  // Wait for 5 interrupts
  uint start_ticks = get_ticks();
  while (get_ticks() < start_ticks + 5) {
    // Busy wait
  }
  interrupt_count = get_ticks() - start_ticks;

  uint64 end_time = get_time();
  printf("Timer test completed: %d interrupts in %lu cycles\n", interrupt_count,
         end_time - start_time);
}

void test_exception_handling(void) {
  printf("Testing exception handling...\n");

  test_mode = 1; // Enable exception recovery in trap.c

  // 1. Test Illegal Instruction
  printf("  1. Triggering Illegal Instruction...\n");
  asm volatile(".word 0"); // Illegal instruction
  printf("  -> Recovered from Illegal Instruction!\n");

  // 2. Test Invalid Memory Access (Store Page Fault)
  printf("  2. Triggering Store Page Fault...\n");
  *(volatile int *)0 = 0; // Write to null pointer
  printf("  -> Recovered from Store Page Fault!\n");

  test_mode = 0; // Disable test mode
  printf("Exception tests completed\n");
}

void test_interrupt_overhead(void) {
  printf("Testing interrupt overhead...\n");

  // Reset accumulator
  interrupt_cycles = 0;

  // Wait for some interrupts to happen
  uint start_ticks = get_ticks();
  while (get_ticks() < start_ticks + 5)
    ;

  printf("Average interrupt overhead: %lu cycles\n", interrupt_cycles / 5);
}

// --- Process Tests ---

// Mock function to simulate process creation since we don't have a user-space loader for arbitrary functions yet.
// In a real scenario, we would use fork() and exec().
// Here we will use fork() and have the child execute the task.

void simple_task() {
  printf("  [PID %d] Simple task running...\n", myproc()->pid);
  exit(0);
}

void cpu_intensive_task() {
  int pid = myproc()->pid;
  printf("  [PID %d] CPU intensive task started\n", pid);
  uint64 start = get_time();
  while (get_time() - start < 10000000) {
    // Busy loop
  }
  printf("  [PID %d] CPU intensive task finished\n", pid);
  exit(0);
}

// Helper to create a kernel process (simulated via fork)
int create_process(void (*func)()) {
  return kthread_create(func);
}

void wait_process(int *status) {
  wait(0);
}

void test_process_creation(void) {
  printf("Testing process creation...\n");

  // 测试基本的进程创建
  int pid = create_process(simple_task);
  if (pid > 0) {
      printf("Created process with PID %d\n", pid);
      wait_process(0);
  } else {
      printf("Failed to create process\n");
  }

  // 测试进程表限制
  int pids[NPROC];
  int count = 0;
  printf("Testing process table limits...\n");
  for (int i = 0; i < NPROC + 5; i++) {
    int pid = create_process(simple_task);
    if (pid > 0) {
      pids[count++] = pid;
    } else {
      printf("Max processes reached at %d\n", count);
      break;
    }
  }
  printf("Created %d processes\n", count);

  // 清理测试进程
  for (int i = 0; i < count; i++) {
    wait_process(0);
  }
}

void test_scheduler(void) {
  printf("Testing scheduler...\n");

  // 创建多个计算密集型进程
  for (int i = 0; i < 3; i++) {
    create_process(cpu_intensive_task);
  }

  // 观察调度行为
  uint64 start_time = get_time();
  // sleep(1000); // 等待1秒 - sleep takes a lock, we can just busy wait or yield
  
  // Let the scheduler run for a bit
  uint64 start_ticks = get_ticks();
  while(get_ticks() < start_ticks + 10) {
      yield();
  }

  uint64 end_time = get_time();

  printf("Scheduler test completed in %lu cycles\n", end_time - start_time);
  
  // Wait for children
  for(int i=0; i<3; i++) wait(0);
}

// Synchronization Test
struct {
  struct spinlock lock;
  int buffer[10];
  int count;
  int in;
  int out;
} shared_buf;

void shared_buffer_init() {
  initlock(&shared_buf.lock, "shared_buf");
  shared_buf.count = 0;
  shared_buf.in = 0;
  shared_buf.out = 0;
}

void producer_task() {
  for (int i = 0; i < 20; i++) {
    acquire(&shared_buf.lock);
    while (shared_buf.count == 10) {
      sleep(&shared_buf.in, &shared_buf.lock);
    }
    shared_buf.buffer[shared_buf.in] = i;
    shared_buf.in = (shared_buf.in + 1) % 10;
    shared_buf.count++;
    printf("Prod: %d\n", i);
    wakeup(&shared_buf.out);
    release(&shared_buf.lock);
  }
  exit(0);
}

void consumer_task() {
  for (int i = 0; i < 20; i++) {
    acquire(&shared_buf.lock);
    while (shared_buf.count == 0) {
      sleep(&shared_buf.out, &shared_buf.lock);
    }
    int val = shared_buf.buffer[shared_buf.out];
    shared_buf.out = (shared_buf.out + 1) % 10;
    shared_buf.count--;
    printf("Cons: %d\n", val);
    wakeup(&shared_buf.in);
    release(&shared_buf.lock);
  }
  exit(0);
}

void test_synchronization(void) {
  printf("Testing synchronization...\n");
  // 测试生产者-消费者场景
  shared_buffer_init();

  create_process(producer_task);
  create_process(consumer_task);

  // 等待完成
  wait_process(0);
  wait_process(0);

  printf("Synchronization test completed\n");
}

void test_main(void) {
  printf("\n=== Starting Kernel Tests ===\n");
  
  // Interrupt tests
  // test_timer_interrupt();
  // test_exception_handling();
  // test_interrupt_overhead();

  // Process tests
  test_process_creation();
  test_scheduler();
  test_synchronization();

  printf("=== All Tests Completed ===\n\n");
  
  // Halt
  for(;;) asm volatile("wfi");
}
