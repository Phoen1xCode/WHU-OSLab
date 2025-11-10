// Mutual exclusion spin locks.
#include "spinlock.h"
#include "defs.h"
#include "memlayout.h"
#include "param.h"
#include "proc.h"
#include "riscv.h"
#include "types.h"

void initlock(struct spinlock *lock, char *name) {
  lock->name = name;
  lock->locked = 0;
  lock->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void acquire(struct spinlock *lock) {
  push_off(); // disable interrupts to avoid deadlock.

  if (holding(lock)) // 检查当前 CPU 是否已持有该锁
    panic("acquire");

  lock->locked = 1; // 单核CPU，直接获取锁

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  // Record info about lock acquisition for holding() and debugging.
  lock->cpu = mycpu();
}

void release(struct spinlock *lock) {
    if(!holding(lock))
        panic("release");
    
    lock->cpu = 0;

    // Tell the C compiler and the CPU to not move loads or stores
    // past this point, to ensure that all the stores in the critical
    // section are visible to other CPUs before the lock is released,
    // and that loads in the critical section occur strictly before
    // the lock is released.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();

    lock->locked = 0; // 释放锁
    pop_off(); // 恢复中断
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int holding(struct spinlock *lock) {
  int r;
  r = (lock->locked && lock->cpu == mycpu());
  return r;
}

// 以可嵌套的方式禁止中断并保存进入前的状态
// 配合 pop_off 实现安全的临界区保护和 per-cpu 数据访问
void push_off(void) {
  int old = intr_get();

  // disable interrupts to prevent an involuntary context
  // switch while using mycpu()
  intr_off(); // 禁用中断
  if (mycpu()->noff == 0) {
    mycpu()->intena = old;
  }
  mycpu()->noff += 1;
}

void pop_off(void) {
  struct cpu *c = mycpu();
  if (intr_get())
    panic("pop_off - interruptible");
  if (c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if (c->noff == 0 && c->intena)
    intr_on(); // 恢复中断
}