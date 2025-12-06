# WHU-OSLab

Operating system experiment code

## Lab1

Please enter this shell code to run qemu:

```shell
make
make run
```

and you will see:

```
SPHello OS
```

## Lab 4

在 RISC-V 架构中，中断（Interrupts）和异常（Exceptions）统称为 Trap。当 Trap 发生时，CPU 会将具体的类型编码写入 `scause`（Supervisor Cause）寄存器。

`scause` 寄存器的最高位（第 63 位）用来区分是 中断 还是 异常：

- 最高位 = 1：表示 中断 (Interrupt) —— 异步发生，通常由外部设备触发（如时钟、磁盘）。
- 最高位 = 0：表示 异常 (Exception) —— 同步发生，由当前执行的指令触发（如系统调用、缺页错误）。

1. 中断

中断会让 CPU 暂停当前任务，转而去处理外部事件

| scause 值（十进制） | 16 进制（64 位） | 名称                          | 描述                                            |
| ------------------- | ---------------- | ----------------------------- | ----------------------------------------------- |
| 1                   | 0x8000...0001    | Supervisor Software Interrupt | 软件中断，通常用于核间通信（IPI）               |
| 5                   | 0x8000...0005    | Supervisor Timer Interrupt    | 时钟中断，用于时间片轮转调度                    |
| 9                   | 0x8000...0009    | Supervisor External Interrupt | 外部中断，来自 PLIC，如 UART 输入、磁盘读写完成 |

- sie Supervisor Interrupt enable
- mie Machine Interrupt enable 中断使能寄存器
- sip Supervisor Interrupt Pedding 中断挂起寄存器
- mideleg Machine Interrupt Delegation 中断委托寄存器
- stvec Supervisor Trap-Vector Base Address 中断向量表

### RISC-V 特权模式

RISC-V 架构定义了三个主要的特权模式 Privilege Modes，用于在不同层级上运行代码，以实现系统的安全性和隔离性

1. Machine Mode (M-mode 机器模式)
   - 特权等级：最高 Level 3
   - 这是 RISC-V 系统复位后进入的第一个模式。M-mode 对硬件拥有完全的访问权限
   - 用途
     - 通常用于运行固件（Firmware）或引导加载程序（Bootloader），如 OpenSBI
     - 在 xv6 中，start.c 就运行在这个模式下。它的主要任务是配置硬件（如时钟、中断委托、物理内存保护 PMP），然后尽快切换到 S-mode。
     - M-mode 可以拦截并处理所有中断和异常，但通常会将它们委托（Delegate）给 S-mode 处理。
2. Supervisor Mode (S-mode, 监管者模式)
   - 特权等级: 中等 (Level 1)
   - 描述: 这是操作系统内核（Kernel）运行的模式。
   - 用途:
     - 允许执行特权指令，如修改页表（Page Tables）、处理中断和异常。
     - 使用虚拟地址（Virtual Memory），通过 MMU（内存管理单元）将虚拟地址映射到物理地址。
     - 在 xv6 中，main() 函数及其调用的所有内核代码都运行在 S-mode。
     - S-mode 无法直接访问 M-mode 的寄存器，必须通过特定的指令或 SBI (Supervisor Binary Interface) 调用来请求 M-mode 的服务。
3. User Mode (U-mode, 用户模式)
   - 特权等级: 最低 (Level 0)
   - 描述: 这是普通应用程序运行的模式。
   - 用途:
     - 权限受限，无法直接访问硬件设备或修改关键寄存器（如 satp 页表寄存器）。
     - 如果应用程序需要执行特权操作（如读写文件、分配内存），必须通过 系统调用 (System Call)（使用 ecall 指令）陷入内核，由运行在 S-mode 的操作系统代为执行。
     - 在 xv6 中，Shell (sh)、ls、cat 等用户程序都运行在 U-mode。

### 中断特权级委托 Delegation

默认情况下，RISC-V 的所有 Trap（中断和异常）都会导致 CPU 切换到 M-mode 处理
然而，现代操作系统（如 Linux, xv6）运行在 S-mode。如果每个缺页异常或时钟中断都要先跳到 M-mode，再由 M-mode 软件手动转发给 S-mode，效率极低且代码复杂

因此，RISC-V 提供了硬件委托机制，允许 M-mode 将特定的 Trap 直接“甩锅”给 S-mode 处理，CPU 在发生这些 Trap 时会自动陷入 S-mode 而不是 M-mode

- `medeleg` (Machine Exception Delegation Register) 异常委托寄存器
  - 作用：控制同步异常（Exception）的委托
  - 机制：: 这是一个位掩码寄存器。如果第 N 位被置为 1，当发生异常代码为 N 的异常时（例如 N=12 是指令缺页异常），CPU 会直接跳转到 stvec 指定的地址，并运行在 S-mode
  - `w_medeleg(0xffff)` 我们简单粗暴地将所有标准异常都委托给 S-mode。这意味着如果用户程序（U-mode）发生除零错误或缺页，S-mode 的内核可以直接捕获并处理
- `mideleg` (Machine Interrupt Delegation Register)
  - 作用: 控制异步中断（Interrupt）的委托。
  - 机制: 同样是位掩码。
    Bit 1: Supervisor Software Interrupt (SSIP)
    Bit 5: Supervisor Timer Interrupt (STIP)
    Bit 9: Supervisor External Interrupt (SEIP)
  - `w_mideleg(0xffff)` 将所有中断委托给 S-mode
