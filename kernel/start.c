#include "defs.h"
#include "riscv.h"
#include "types.h"

int main();
void timerinit();

void start(void) {
  // 设置M-Mode的前一个特权级为S-Mode
  // 当执行 mret 指令时，CPU 会切换到 S-Mode
  unsigned long x = r_mstatus(); // 读取 mstatus 寄存器的值
  x &= ~MSTATUS_MPP_MASK;      // 清除 MPP (Machine Previous Privilege) 字段的位
  x |= MSTATUS_MPP_Supervisor; // 设置 MPP 字段为 Supervisor Mode
  w_mstatus(x);                // 写回 mstatus 寄存器

  // 设置 mepc (Machine Exception Program Counter) 为 main 函数的地址
  // mret 指令会将 PC 设置为 mepc 的值，从而跳转到 main()
  w_mepc((uint64)main);

  // 禁用分页
  w_satp(0);

  // 将所有中断和异常委托给 S-Mode 处理
  w_medeleg(0xffff);                    // 委托所有异常
  w_mideleg(0xffff);                    // 委托所有中断
  w_sie(r_sie() | SIE_SEIE | SIE_STIE); // 启用 S-Mode 的外部和时钟中断

  // 配置 PMP (Physical Memory Protection) 允许 S-mode 访问所有物理内存
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // 请求时钟中断
  timerinit();

  // save each CPU's hartid in its tp register for cpuid()
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// 配置时钟中断，使内核可以处理时钟中断，进行进程调度
// 在RISC-V规范中，只有M-Mode可以直接读写mtimecmp寄存器来设定下一次的中断时间，而进程调度(Scheduler)是OS(S-Mode)的功能，OS需要通过时钟中断来驱动时间片轮转
// 在较新的 RISC-V 实现中，引入了 sstc 扩展，允许 S-Mode 通过
// stimecmp寄存器来设定时钟中断，本系统的设计使用了这个实现
void timerinit() {
  // enable supervisor-mode timer interrupts.
  w_mie(r_mie() | MIE_STIE); // 启用 S-Mode 时钟中断

  // enable the sstc extension (i.e. stimecmp).
  // 设置 menvcfg 寄存器的第63位，启用 sstc 扩展
  w_menvcfg(r_menvcfg() | (1L << 63));

  // allow supervisor to use stimecmp and time.
  // 允许 S-Mode 访问 time 寄存器
  w_mcounteren(r_mcounteren() | 2);

  // ask for the very first timer interrupt.
  // 设置下一次时钟中断出发的时间点 + 1000000个周期
  w_stimecmp(r_time() + 1000000);
}