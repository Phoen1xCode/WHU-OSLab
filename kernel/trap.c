#include "defs.h"
#include "memlayout.h"
#include "proc.h"
#include "riscv.h"
#include "spinlock.h"
#include "types.h"

/*
Trap 陷阱是一种从用户态切换到内核态的机制
用于处理各种需要操作系统介入的时间
可以理解为程序执行过程中的 打断 or 转移

陷阱的三大类型
- 系统调用 System Call
  - 用户程序主动发起，向操作系统请求服务
- 异常 Exception
  - 程序执行过程中发生的意外情况
- 中断 Interrupt
  - 外部设备发起的请求，要求 CPU 停止当前任务，转而处理该请求

Trap Vector 陷阱向量是一个内存地址，指向处理陷阱的代码入口
当陷阱发生时，CPU 会跳转到该地址执行相应的处理程序
在 RISC-V 架构中，陷阱向量通常存储在 stvec 寄存器中
*/

struct spinlock tickslock;
// 全局时钟滴答计数器
uint ticks;

extern char trampoline[], uservector[];

// in kernelvector.S, calls kerneltrap().
void kernelvector();

extern int devintr();

// 初始化陷阱处理系统
// 在系统启动时调用一次
void trapinit(void) {
  ticks = 0;
  initlock(&tickslock, "time");
}

// 为当前 hart（硬件线程）设置陷阱向量
// 在每个 CPU 核心启动时调用（单核系统只调用一次）
void trapinithart(void) {
  // 设置内核陷阱向量，指向 kernelvector
  w_stvec((uint64)kernelvector);
}

// 处理来自用户空间的异常、中断或系统调用
// 从 trampoline.S 调用，并返回到 trampoline.S
// 返回值是用户页表的 satp 值，供 trampoline.S 切换使用
uint64 usertrap(void) {
  int which_dev = 0;

  // 验证陷阱确实来自用户模式
  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // 现在我们在内核中，将陷阱向量指向内核陷阱处理程序
  w_stvec((uint64)kernelvector);

  // 保存用户程序计数器
  struct proc *p = myproc();

  // 访问当前进程的 trapframe
  p->trapframe->epc = r_sepc();

  uint64 scause = r_scause(); // 获取陷阱原因码

  if (scause == 8) {
    // system call

    // sepc 指向 ecall 指令，但我们想返回到下一条指令
    p->trapframe->epc += 4;

    // 现在可以安全地启用中断了
    // 因为我们已经处理完了 sepc、scause 和 sstatus
    intr_on();

    // 调用系统调用处理程序
    // syscall(); // 暂未实现

  } else if ((which_dev = devintr()) != 0) {
    // 设备中断，已经被 devintr() 处理
    // ok
  } else if (scause == 15 || scause == 13) {
    // 页面错误：15=存储页面错误，13=加载页面错误
    uint64 stval = r_stval(); // 导致错误的虚拟地址

    // 这里可以处理懒分配、写时复制等
    // int is_write = (scause == 15) ? 1 : 0;
    // if (vmfault(pagetable, stval, is_write) != 0) {
    //   // 页面错误处理失败
    //   printf("usertrap(): page fault at 0x%lx\n", stval);
    //   // 杀死进程或采取其他措施
    // }

    printf("usertrap(): page fault at 0x%lx\n", stval);
    panic("page fault");

  } else {
    // 未知的陷阱类型
    printf("usertrap(): unexpected scause 0x%lx\n", scause);
    printf("            sepc=0x%lx stval=0x%lx\n", p->trapframe->epc,
           r_stval());
    panic("usertrap");
  }

  // 如果是时钟中断，让出 CPU
  // if (which_dev == 2)
  //   yield();  // 需要实现进程调度

  // 准备返回用户空间
  usertrapret();

  // 返回用户页表（这里需要根据你的进程管理实现）
  // return MAKE_SATP(current_process->pagetable);
  return 0; // 临时返回值
}

//
// 准备从内核返回到用户空间
// 设置 trapframe 和控制寄存器
//
void usertrapret(void) {
  // 关闭中断，因为我们要切换陷阱向量
  // 如果在切换过程中发生中断会很危险
  intr_off();

  // 将陷阱向量指向用户空间的陷阱处理程序（在 trampoline 中）
  uint64 trampoline_uservec = TRAMPOLINE + (uservector - trampoline);
  w_stvec(trampoline_uservec);

  // 设置 trapframe 的值，供下次陷入内核时使用
  // 这里需要根据你的进程管理实现来填充
  // struct proc *p = myproc();
  // p->trapframe->kernel_satp = r_satp();
  // p->trapframe->kernel_sp = p->kstack + PAGESIZE;
  // p->trapframe->kernel_trap = (uint64)usertrap;
  // p->trapframe->kernel_hartid = 0;  // 单核系统

  // 设置 sstatus 寄存器，准备返回用户模式
  uint64 x = r_sstatus();
  x &= ~SSTATUS_SPP; // 清除 SPP 位，设置为用户模式
  x |= SSTATUS_SPIE; // 启用用户模式中断
  w_sstatus(x);

  // 设置返回地址（保存的用户 PC）
  // w_sepc(p->trapframe->epc);
}

// 处理来自内核代码的中断和异常
// 通过 kernelvec 调用，使用当前的内核栈
void kerneltrap(void) {
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  // 验证陷阱来自 supervisor 模式
  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");

  // 验证中断已关闭
  if (intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  // 确定中断来源
  which_dev = devintr();
  // 处理设备中断
  if (which_dev == 0) {
    // 未知的中断或陷阱
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, sepc, r_stval());
    panic("kerneltrap");
  }

  // 如果是时钟中断，可能需要让出 CPU
  if (which_dev == 2 && myproc() != 0) {
    //   yield();
  }

  // 恢复 sepc 和 sstatus，因为 yield() 可能改变了它们
  w_sepc(sepc);
  w_sstatus(sstatus);
}

// 时钟中断处理程序
// 更新全局时钟计数器并设置下一次中断
void clockintr(void) {
  acquire(&tickslock);
  // 增加时钟滴答计数
  ticks++;
  printf("Timer interrupt: %d\n", ticks);

  // 唤醒等待时钟的进程
  // wakeup(&ticks);
  release(&tickslock);

  // 设置下一次时钟中断
  w_stimecmp(r_time() + 1000000);
}

// 检查并处理设备中断
//   2 - 时钟中断
//   1 - 其他设备中断
//   0 - 无法识别的中断
int devintr(void) {
  uint64 scause = r_scause();

  if (scause == 0x8000000000000009L) {
    // 外部中断（通过 PLIC - 平台级中断控制器）
    // 获取中断请求号
    int irq = plic_claim();

    // 根据 IRQ 调用相应的处理程序
    if (irq == UART0_IRQ) {
      uartintr();
    } else if (irq == VIRTIO0_IRQ) {
      // virtio_disk_intr();
    } else if (irq) {
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // 通知 PLIC 中断已处理
    if (irq)
      plic_complete(irq);

    return 1;
  } else if (scause == 0x8000000000000005L) {
    clockintr(); // 时钟中断
    return 2;
  } else {
    return 0; // 无法识别的中断
  }
}
