/*
 * sysproc.c - 进程相关系统调用实现
 *
 * 实现进程管理、内存管理和系统信息相关的系统调用
 */

#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../include/types.h"
#include "../proc/proc.h"
#include "../sync/spinlock.h"
#include "../fs/file.h"

// 外部声明
extern struct spinlock tickslock;
extern uint ticks;

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  argint(n, &fd);
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

/*
 * sys_fork - 创建子进程
 *
 * 复制当前进程，创建一个新的子进程
 * 返回值：父进程返回子进程PID，子进程返回0
 */
uint64 sys_fork(void) { return fork(); }

/*
 * sys_exit - 退出当前进程
 *
 * 参数：a0 = 退出状态码
 * 不返回（noreturn）
 */
uint64 sys_exit(void) {
  int n;
  argint(0, &n);
  exit(n);
  return 0; // 不会到达这里
}

/*
 * sys_wait - 等待子进程退出
 *
 * 参数：a0 = 存放退出状态的用户空间地址（可为0表示不需要）
 * 返回值：退出的子进程PID，无子进程返回-1
 */
uint64 sys_wait(void) {
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

/*
 * sys_getpid - 获取当前进程ID
 *
 * 返回值：当前进程的PID
 */
uint64 sys_getpid(void) { return myproc()->pid; }

/*
 * sys_kill - 杀死指定进程
 *
 * 参数：a0 = 目标进程PID
 * 返回值：成功返回0，失败返回-1
 */
uint64 sys_kill(void) {
  int pid;
  argint(0, &pid);
  return kill(pid);
}

/*
 * sys_sleep - 睡眠指定的时钟周期
 *
 * 参数：a0 = 睡眠的时钟周期数
 * 返回值：成功返回0，被杀死返回-1
 */
uint64 sys_sleep(void) {
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;

  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

/*
 * sys_sbrk - 增长或缩小进程内存
 *
 * 参数：a0 = 增长/缩小的字节数（负数表示缩小）
 * 返回值：成功返回原来的内存大小，失败返回-1
 */
uint64 sys_sbrk(void) {
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;

  if (growproc(n) < 0)
    return -1;

  return addr;
}

/*
 * sys_uptime - 获取系统运行时间
 *
 * 返回值：系统启动以来的时钟周期数
 */
uint64 sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/*
 * sys_write - 写入数据 目前仅支持控制台输出
 *
 * 参数：
 *   a0 = 文件描述符（1=stdout, 2=stderr 输出到控制台）
 *   a1 = 用户空间缓冲区地址
 *   a2 = 写入字节数
 * 返回值：实际写入的字节数，失败返回-1
 */
uint64 sys_write(void) {
  struct file *f;
  int n;
  uint64 p;

  argaddr(1, &p);
  argint(2, &n);
  if(argfd(0, 0, &f) < 0)
    return -1;

  return filewrite(f, p, n);
}

/*
 * sys_read - 读取数据（目前仅支持控制台输入）
 *
 * 参数：
 *   a0 = 文件描述符（0=stdin 从控制台读取）
 *   a1 = 用户空间缓冲区地址
 *   a2 = 读取字节数
 * 返回值：实际读取的字节数，失败返回-1
 *
 */
uint64 sys_read(void) {
  struct file *f;
  int n;
  uint64 p;

  argaddr(1, &p);
  argint(2, &n);
  if(argfd(0, 0, &f) < 0)
    return -1;
  return fileread(f, p, n);
}

/*
 * sys_hello - 打印问候消息 自定义系统调用示例
 */
uint64 sys_hello(void) {
  struct proc *p = myproc();
  printf("[syscall] Hello from sys_hello! PID=%d, name=%s\n", p->pid, p->name);
  return p->pid;
}
