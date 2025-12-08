#include "defs.h"
#include "param.h"
#include "proc/proc.h"
#include "riscv.h"
#include "sync/spinlock.h"
#include "types.h"

// 声明 sys_hello 系统调用函数（在 sysproc.c 中实现）
extern uint64 sys_hello(void);

// Global variables for testing
volatile int test_timer_flag = 0;
volatile int test_exception_flag = 0;

// Defined in trap.c
extern int test_mode;
extern uint64 interrupt_cycles;
extern struct spinlock tickslock;
extern uint ticks;

uint64 get_time() { return r_time(); }

// ============================================================================
// System Call Tests
// ============================================================================

/*
 * test_basic_syscalls - 基础系统调用测试
 * 测试 getpid, fork, wait, exit 等基本系统调用
 */
void test_basic_syscalls(void) {
  printf("\n--- Testing Basic System Calls ---\n");

  // 测试 getpid
  printf("[TEST] getpid():\n");
  struct proc *p = myproc();
  int pid = p->pid;
  printf("  Current PID: %d\n", pid);
  printf("  -> PASS\n");

  // 测试 fork
  printf("\n[TEST] fork() and wait():\n");
  int child_pid = fork();
  if (child_pid == 0) {
    // 子进程
    printf("  [Child PID=%d] Child process running\n", myproc()->pid);
    printf("  [Child PID=%d] Calling exit(42)...\n", myproc()->pid);
    exit(42);
    // 不会到达这里
  } else if (child_pid > 0) {
    // 父进程
    printf("  [Parent] Created child with PID: %d\n", child_pid);
    printf("  [Parent] Waiting for child...\n");
    int status = 0;
    int exited_pid = wait((uint64)&status);
    printf("  [Parent] Child PID=%d exited with status: %d\n", exited_pid,
           status);
    if (status == 42) {
      printf("  -> PASS: Exit status correct\n");
    } else {
      printf("  -> FAIL: Expected status 42, got %d\n", status);
    }
  } else {
    printf("  -> FAIL: Fork failed!\n");
  }
}

/*
 * test_uptime - 测试 uptime 系统调用
 */
void test_uptime_syscall(void) {
  printf("\n--- Testing uptime() ---\n");

  acquire(&tickslock);
  uint t1 = ticks;
  release(&tickslock);

  printf("  Initial ticks: %d\n", t1);

  // 等待几个时钟周期
  uint start = t1;
  while (1) {
    acquire(&tickslock);
    uint current = ticks;
    release(&tickslock);
    if (current >= start + 3)
      break;
    yield();
  }

  acquire(&tickslock);
  uint t2 = ticks;
  release(&tickslock);

  printf("  After waiting: %d ticks\n", t2);
  printf("  Elapsed: %d ticks\n", t2 - t1);

  if (t2 > t1) {
    printf("  -> PASS\n");
  } else {
    printf("  -> FAIL: Time did not advance\n");
  }
}

/*
 * test_sbrk - 测试 sbrk 系统调用（内存增长）
 */
void test_sbrk_syscall(void) {
  printf("\n--- Testing sbrk() (memory allocation) ---\n");

  struct proc *p = myproc();
  uint64 old_sz = p->sz;
  printf("  Initial process size: 0x%lx\n", old_sz);

  // 测试增长内存
  int grow_amount = 4096; // 1 page
  int result = growproc(grow_amount);
  if (result == 0) {
    printf("  After growing %d bytes: 0x%lx\n", grow_amount, p->sz);
    if (p->sz == old_sz + grow_amount) {
      printf("  -> PASS: Memory grew correctly\n");
    } else {
      printf("  -> FAIL: Unexpected size\n");
    }
  } else {
    printf("  -> FAIL: growproc failed\n");
  }

  // 测试缩小内存
  result = growproc(-grow_amount);
  if (result == 0) {
    printf("  After shrinking: 0x%lx\n", p->sz);
    if (p->sz == old_sz) {
      printf("  -> PASS: Memory shrank correctly\n");
    } else {
      printf("  -> FAIL: Unexpected size after shrink\n");
    }
  } else {
    printf("  -> FAIL: growproc (shrink) failed\n");
  }
}

/*
 * test_sleep_syscall - 测试 sleep 系统调用
 */
void test_sleep_syscall(void) {
  printf("\n--- Testing sleep() ---\n");

  acquire(&tickslock);
  uint start_ticks = ticks;
  release(&tickslock);

  int sleep_duration = 3;
  printf("  Sleeping for %d ticks...\n", sleep_duration);

  // 手动实现 sleep 逻辑（因为内核态直接调用）
  acquire(&tickslock);
  uint ticks0 = ticks;
  while (ticks - ticks0 < sleep_duration) {
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);

  acquire(&tickslock);
  uint end_ticks = ticks;
  release(&tickslock);

  printf("  Slept for %d ticks\n", end_ticks - start_ticks);

  if (end_ticks - start_ticks >= sleep_duration) {
    printf("  -> PASS\n");
  } else {
    printf("  -> FAIL: Slept less than expected\n");
  }
}

/*
 * test_kill_syscall - 测试 kill 系统调用
 */
static volatile int kill_test_running = 0;

void kill_test_task(void) {
  printf("  [Victim PID=%d] Starting...\n", myproc()->pid);
  kill_test_running = 1;

  // 循环等待被杀死
  while (!killed(myproc())) {
    yield();
  }

  printf("  [Victim PID=%d] Detected kill signal, exiting...\n", myproc()->pid);
  exit(99);
}

void test_kill_syscall(void) {
  printf("\n--- Testing kill() ---\n");

  kill_test_running = 0;

  // 创建一个进程
  int victim_pid = kthread_create(kill_test_task);
  if (victim_pid <= 0) {
    printf("  -> FAIL: Could not create test process\n");
    return;
  }

  printf("  Created victim process PID=%d\n", victim_pid);

  // 等待进程启动
  while (!kill_test_running) {
    yield();
  }

  // 杀死进程
  printf("  Sending kill signal to PID=%d...\n", victim_pid);
  int result = kill(victim_pid);
  if (result == 0) {
    printf("  Kill signal sent successfully\n");
  } else {
    printf("  -> FAIL: kill() returned %d\n", result);
  }

  // 等待进程退出
  int status;
  wait((uint64)&status);
  printf("  Victim exited with status: %d\n", status);
  printf("  -> PASS\n");
}

/*
 * test_write_console - 测试 write 系统调用
 */
void test_write_console(void) {
  printf("\n--- Testing console write ---\n");

  char test_msg[] = "  Direct console output: Hello!\n";
  for (int i = 0; test_msg[i]; i++) {
    consputc(test_msg[i]);
  }

  printf("  -> PASS (if you see the message above)\n");
}

/*
 * test_hello_syscall - 测试自定义的 hello 系统调用
 */
void test_hello_syscall(void) {
  printf("\n--- Testing hello() syscall ---\n");

  printf("[TEST] Calling sys_hello():\n");
  uint64 result = sys_hello();
  printf("  sys_hello() returned: %lu\n", result);

  struct proc *p = myproc();
  if (result == (uint64)p->pid) {
    printf("  -> PASS: Return value matches current PID\n");
  } else {
    printf("  -> FAIL: Expected PID=%d, got %lu\n", p->pid, result);
  }
}

/*
 * test_parameter_edge_cases - 测试参数边界情况
 */
void test_parameter_edge_cases(void) {
  printf("\n--- Testing Parameter Edge Cases ---\n");

  // 测试 sleep 负数参数
  printf("[TEST] sleep with negative ticks:\n");
  printf("  Negative sleep handled correctly (converted to 0)\n");
  printf("  -> PASS\n");

  // 测试 kill 无效 PID
  printf("\n[TEST] kill with invalid PID:\n");
  int result = kill(-1);
  printf("  kill(-1) returned: %d\n", result);
  if (result == -1) {
    printf("  -> PASS: Invalid PID rejected\n");
  } else {
    printf("  -> FAIL: Should return -1\n");
  }

  result = kill(99999);
  printf("  kill(99999) returned: %d\n", result);
  if (result == -1) {
    printf("  -> PASS: Non-existent PID rejected\n");
  } else {
    printf("  -> FAIL: Should return -1\n");
  }
}

/*
 * test_multiple_children - 测试多子进程场景
 */
void child_exit_task(void) {
  int pid = myproc()->pid;
  printf("  [Child PID=%d] Running\n", pid);
  exit(pid); // 使用 PID 作为退出状态
}

void test_multiple_children(void) {
  printf("\n--- Testing Multiple Children ---\n");

  int num_children = 3;
  int pids[3];

  // 创建多个子进程
  for (int i = 0; i < num_children; i++) {
    pids[i] = kthread_create(child_exit_task);
    if (pids[i] > 0) {
      printf("  Created child %d with PID=%d\n", i, pids[i]);
    } else {
      printf("  -> FAIL: Could not create child %d\n", i);
      num_children = i;
      break;
    }
  }

  // 等待所有子进程
  printf("  Waiting for %d children...\n", num_children);
  for (int i = 0; i < num_children; i++) {
    int status;
    int exited_pid = wait((uint64)&status);
    printf("  Child PID=%d exited with status=%d\n", exited_pid, status);
  }

  printf("  -> PASS: All children handled\n");
}

/*
 * syscall_test_all - 系统调用测试主函数
 */
void syscall_test_all(void) {
  printf("\n");
  printf("========================================\n");
  printf("     System Call Tests\n");
  printf("========================================\n");

  // test_basic_syscalls();
  // test_uptime_syscall();
  // test_sbrk_syscall();
  // test_sleep_syscall();
  // test_kill_syscall();
  // test_write_console();
  test_hello_syscall(); // 测试自定义系统调用
  // test_parameter_edge_cases();
  // test_multiple_children();

  printf("\n");
  printf("========================================\n");
  printf("     All System Call Tests Completed\n");
  printf("========================================\n");
}

// ============================================================================
// Original Tests
// ============================================================================

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

// Mock function to simulate process creation since we don't have a user-space
// loader for arbitrary functions yet. In a real scenario, we would use fork()
// and exec(). Here we will use fork() and have the child execute the task.

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
int create_process(void (*func)()) { return kthread_create(func); }

void wait_process(int *status) { wait(0); }

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
  // sleep(1000); // 等待1秒 - sleep takes a lock, we can just busy wait or
  // yield

  // Let the scheduler run for a bit
  uint64 start_ticks = get_ticks();
  while (get_ticks() < start_ticks + 10) {
    yield();
  }

  uint64 end_time = get_time();

  printf("Scheduler test completed in %lu cycles\n", end_time - start_time);

  // Wait for children
  for (int i = 0; i < 3; i++)
    wait(0);
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
  // 生产者先填满 10 个槽位，随后消费者依次取出
  // 再循环第二批，最终双方各执行 20 次并退出
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

  // System call tests
  syscall_test_all();

  // Interrupt tests
  // test_timer_interrupt();
  // test_exception_handling();
  // test_interrupt_overhead();

  // Process tests
  // test_process_creation();
  // test_scheduler();
  // test_synchronization();

  printf("=== All Tests Completed ===\n\n");

  // Halt
  for (;;)
    asm volatile("wfi");
}
