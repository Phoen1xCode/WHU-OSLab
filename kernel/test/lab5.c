// Lab5
#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../proc/proc.h"

// 使用 myproc() 作为 current_proc
#define current_proc myproc()

// 简单的 assert 宏
#define assert(x)                                                              \
  do {                                                                         \
    if (!(x)) {                                                                \
      printf("Assertion failed: %s at %s:%d\n", #x, __FILE__, __LINE__);       \
      for (;;)                                                                 \
        ;                                                                      \
    }                                                                          \
  } while (0)

// 前向声明
void shared_buffer_init(void);
void producer_task(void);
void consumer_task(void);

void simple_task(void) { int a = 1; }
void test_process_creation(void) {
  printf("Testing process creation...\n");

  // 测试基本的进程创建
  int pid = create_process(simple_task);
  assert(pid > 0);

  // 测试进程表限制，应该创建64个
  int pids[NPROC];
  int count = 1;
  for (int i = 0; i < NPROC + 5; i++) {
    int pid = create_process(simple_task);
    if (pid > 0) {
      pids[count++] = pid;
    } else {
      break;
    }
  }
  printf("Created %d processes\n", count);

  // 清理测试进程
  for (int i = 0; i < count; i++) {
    wait_process(0);
  }

  printf("Process creation test completed\n");
}
void cpu_task_high(void) {
  volatile unsigned long sum = 0;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 1000000; j++)
      sum += j;
    printf("HIGH iter %d\n", i);
    // 让出CPU使得调度器运行其他进程
    yield();
  }
  printf("HIGH task completed\n");

  // 终止当前进程
  exit_process(current_proc, 0);
}
void cpu_task_med(void) {
  volatile unsigned long sum = 0;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 500000; j++)
      sum += j;
    printf("MED  iter %d\n", i);
    yield();
  }
  printf("MED  task completed\n");

  exit_process(current_proc, 0);
}

void cpu_task_low(void) {
  volatile unsigned long sum = 0;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 100000; j++)
      sum += j;
    printf("LOW  iter %d\n", i);
    yield();
  }
  printf("LOW  task completed\n");

  exit_process(current_proc, 0);
}

void test_scheduler(void) {
  printf("Testing scheduler...\n");

  int pid_high = create_process(cpu_task_high);
  int pid_med = create_process(cpu_task_med);
  int pid_low = create_process(cpu_task_low);

  if (pid_high <= 0 || pid_med <= 0 || pid_low <= 0) {
    printf("create_process failed: %d %d %d\n", pid_high, pid_med, pid_low);
    return;
  }
  printf("Created processes: HIGH = %d, MED = %d, LOW = %d\n", pid_high,
         pid_med, pid_low);

  // 设置优先级，为了循环运行，每次差值为3
  set_proc_priority(pid_high, 50);
  set_proc_priority(pid_med, 49);
  set_proc_priority(pid_low, 48);
  printf("Set process priorities\n");

  // 启动优先级调度器
  scheduler_priority();
  printf("Scheduler test completed\n");
}

void test_synchronization(void) {
  printf("Starting synchronization test\n");
  shared_buffer_init();

  int pid_p = create_process(producer_task);
  int pid_c = create_process(consumer_task);
  if (pid_p <= 0 || pid_c <= 0) {
    printf("create_process failed: %d %d\n", pid_p, pid_c);
    return;
  }

  // 启动轮转调度器
  scheduler_rotate();
  printf("Synchronization test completed\n");
}

// 共享缓冲区用于生产者-消费者测试
#define BUFFER_SIZE 8
int shared_buffer[BUFFER_SIZE];
int buffer_count = 0;
int buffer_in = 0;
int buffer_out = 0;
struct spinlock buffer_lock;

void shared_buffer_init(void) {
  initlock(&buffer_lock, "buffer_lock");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    shared_buffer[i] = 0;
  }
  buffer_count = 0;
  buffer_in = 0;
  buffer_out = 0;
}

void producer_task(void) {
  for (int i = 0; i < 10; i++) {
    acquire(&buffer_lock);
    while (buffer_count >= BUFFER_SIZE) {
      // 缓冲区满，等待
      release(&buffer_lock);
      yield();
      acquire(&buffer_lock);
    }
    // 生产数据
    shared_buffer[buffer_in] = i;
    buffer_in = (buffer_in + 1) % BUFFER_SIZE;
    buffer_count++;
    printf("Producer produced: %d\n", i);
    release(&buffer_lock);
    yield();
  }
  printf("Producer completed\n");
  exit_process(current_proc, 0);
}

void consumer_task(void) {
  for (int i = 0; i < 10; i++) {
    acquire(&buffer_lock);
    while (buffer_count <= 0) {
      // 缓冲区空，等待
      release(&buffer_lock);
      yield();
      acquire(&buffer_lock);
    }
    // 消费数据
    int data = shared_buffer[buffer_out];
    buffer_out = (buffer_out + 1) % BUFFER_SIZE;
    buffer_count--;
    printf("Consumer consumed: %d\n", data);
    release(&buffer_lock);
    yield();
  }
  printf("Consumer completed\n");
  exit_process(current_proc, 0);
}