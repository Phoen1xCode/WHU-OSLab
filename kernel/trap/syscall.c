/*
 * syscall.c - 系统调用分发逻辑
 *
 * 实现系统调用的参数获取和分发机制
 * 当用户程序执行 ecall 指令时，通过 trap.c 的 usertrap() 调用 syscall()
 * syscall() 从 trapframe->a7 获取系统调用号，然后分发到具体的处理函数
 */

#include "../trap/syscall.h"
#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../include/types.h"
#include "../proc/proc.h"
#include "../sync/spinlock.h"

/*
 * 系统调用参数获取函数
 * RISC-V 调用规约：参数通过 a0-a5 寄存器传递
 * 这些寄存器在陷入时已保存到 trapframe 中
 */

// 获取第 n 个参数的原始值 (64位)
static uint64 argraw(int n) {
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw: invalid argument index");
  return -1;
}

// 获取第 n 个 32 位整数参数
void argint(int n, int *ip) { *ip = argraw(n); }

// 获取第 n 个 64 位地址参数
void argaddr(int n, uint64 *ip) { *ip = argraw(n); }

// 从用户空间获取地址处的 uint64 值
int fetchaddr(uint64 addr, uint64 *ip) {
  struct proc *p = myproc();
  // 检查地址是否在进程地址空间内
  if (addr >= p->sz || addr + sizeof(uint64) > p->sz)
    return -1;
  // 从用户空间复制数据到内核
  if (copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// 从用户空间获取以 null 结尾的字符串
// 返回字符串长度（不包括 null），失败返回 -1
int fetchstr(uint64 addr, char *buf, int max) {
  struct proc *p = myproc();
  if (copyinstr(p->pagetable, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

// 获取第 n 个参数作为字符串，复制到 buf 中
// 最多复制 max 字节
// 返回字符串长度，失败返回 -1
int argstr(int n, char *buf, int max) {
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

/*
 * 系统调用处理函数声明
 * 这些函数在 sysproc.c 中实现
 */
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_sleep(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_uptime(void);
extern uint64 sys_write(void);
extern uint64 sys_read(void);
extern uint64 sys_hello(void);

/*
 * 系统调用分发表
 * 将系统调用号映射到处理函数
 * 使用 C99 designated initializers 语法
 */
static uint64 (*syscalls[])(void) = {
    [SYS_fork] sys_fork,     [SYS_exit] sys_exit,     [SYS_wait] sys_wait,
    [SYS_getpid] sys_getpid, [SYS_kill] sys_kill,     [SYS_sleep] sys_sleep,
    [SYS_sbrk] sys_sbrk,     [SYS_uptime] sys_uptime, [SYS_write] sys_write,
    [SYS_read] sys_read,     [SYS_hello] sys_hello,
};

// 系统调用名称 for 调试
static char *syscall_names[] = {
    [SYS_fork] "fork",     [SYS_exit] "exit",     [SYS_wait] "wait",
    [SYS_getpid] "getpid", [SYS_kill] "kill",     [SYS_sleep] "sleep",
    [SYS_sbrk] "sbrk",     [SYS_uptime] "uptime", [SYS_write] "write",
    [SYS_read] "read",     [SYS_hello] "hello",
};

/*
 * syscall - 系统调用主分发函数
 *
 * 从 trapframe->a7 获取系统调用号
 * 查表调用相应的处理函数
 * 将返回值存入 trapframe->a0（用户态可见）
 */
void syscall(void) {
  int num;
  struct proc *p = myproc();

  // 从 a7 寄存器获取系统调用号
  num = p->trapframe->a7;

  // 验证系统调用号有效性
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    // 调用对应的系统调用处理函数
    // 返回值存入 a0，用户态将通过 a0 获取
    p->trapframe->a0 = syscalls[num]();

    // for test
    // printf("syscall %s -> %d\n", syscall_names[num], (int)p->trapframe->a0);
  } else {
    // 未知的系统调用号
    printf("pid %d %s: unknown syscall %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1; // 返回错误
  }
}