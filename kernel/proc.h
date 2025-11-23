#ifndef PROC_H
#define PROC_H

#include "param.h"
#include "defs.h"
#include "spinlock.h"

/*
两种上下文
- struct context: 内核态上下文切换时保存的寄存器状态
- struct trapframe: 用户态陷入内核态时保存的寄存器状态，用于用户态<->内核态切换
*/

// 保存内核态上下文切换时的寄存器状态
struct context {
  uint64 ra; // 返回地址寄存器
  uint64 sp; // 栈指针寄存器

  // callee-saved 寄存器(被调用者保存)
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state
struct cpu {
  struct proc *proc;      // 当前 CPU 上运行的进程，若无则为 null
  struct context context; // 切换到调度器时使用的上下文
  uint noff;              // push_off() 嵌套深度
  uint intena;            // push_off() 之前中断是否启用
};

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.

// 保存用户态的所有寄存器状态
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

enum procstate {
  UNUSED, // 进程槽位未使用
  USED, // 进程已分配但未初始化完成
  SLEEPING, // 进程睡眠等待
  RUNNABLE, // 就绪，等待调度
  RUNNING, // 正在运行
  ZOMBIE // 僵尸状态 已退出等待父进程回收
};

// Per-process state
struct proc {
  struct spinlock lock; // 保护进程状态的自旋锁

  // p->lock must be held when using these:
  enum procstate state;        // Process state 进程状态
  void *chan;                 // If non-zero, sleeping on chan 睡眠通道(等待的事件地址)
  int killed;                 // If non-zero, have been killed 是否被杀死
  int xstate;                 // Exit status to be returned to parent's wait 退出状态 返回给父进程
  int pid;                    // Process ID 进程ID

  // wait_lock must be held when using this:
  struct proc *parent;        // Parent process 父进程

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;              // Virtual address of kernel stack 内核栈虚拟地址
  uint64 sz;                  // Size of process memory (bytes) 进程内存大小(字节)
  pagetable_t pagetable;      // User page table 用户页表
  struct trapframe *trapframe; // data page for trampoline.S 进程陷阱帧
  struct context context;     // swtch() here to run process 切换到进程的上下文
  // struct file *ofile[NOFILE]; // Open files 打开的文件
  // struct inode *cwd;          // Current directory 当前工作目录
  char name[16];              // Process name (debugging) 进程名称
};

#endif // PROC_H