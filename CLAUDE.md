
# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

WHU-OSLab is a RISC-V operating system kernel implementation for Wuhan University OS laboratory assignments. The project is built as a bare-metal OS kernel running on QEMU's RISC-V 64-bit virtual machine, implementing core OS subsystems including memory management, trap handling, and process management.

## Build and Run Commands

```bash
# Build the kernel
make

# Run the kernel in QEMU
make run

# Clean build artifacts
make clean
```

**QEMU Exit**: Press `Ctrl-A` then `X` to exit QEMU.

## Toolchain Requirements

- **RISC-V GNU Toolchain**: One of `riscv64-unknown-elf-*`, `riscv64-elf-*`, `riscv64-linux-gnu-*`, or `riscv64-unknown-linux-gnu-*`
- **QEMU**: `qemu-system-riscv64` version 7.2 or later
- **Architecture**: RISC-V 64-bit with `rv64gc` ISA and `lp64` ABI

## Architecture Overview

### Memory Layout

The kernel uses RISC-V Sv39 paging with a direct-mapped kernel address space:

**Physical Memory**:

- `0x80000000` - Kernel entry point (loaded by QEMU)
- `0x80000000 + end` - Free memory begins
- `0x80000000 + 128MB` - Physical memory end (PHYSTOP)

**Memory-Mapped I/O**:

- `0x10000000` - UART0 registers
- `0x10001000` - VirtIO disk interface
- `0x0c000000` - PLIC (Platform-Level Interrupt Controller)

**Virtual Memory**:

- `MAXVA - PAGESIZE` - Trampoline page (shared between user/kernel space)
- `TRAMPOLINE - PAGESIZE` - Trapframe page (per-process)
- `TRAMPOLINE - (p+1)*2*PAGESIZE` - Kernel stack for process p (with guard pages)

### Key Subsystems

**Physical Memory Allocator** (`kernel/kalloc.c`):

- Free list-based page allocator with reference counting
- Debug features: allocation tracking, leak detection, memory statistics
- Use `kalloc()` to allocate a 4KB page, `kfree()` to free
- Macros `kalloc_internal(__FILE__, __LINE__)` track allocation sites for debugging

**Virtual Memory System** (`kernel/vm.c`):

- Implements Sv39 three-level page tables
- `kvminit()` - Creates and activates kernel page table
- `kvminithart()` - Enables paging by setting SATP register
- `walk()` - Page table traversal for address translation
- `mappages()` - Maps virtual addresses to physical pages
- Direct-mapped kernel pages (VA = PA) for simplicity

**Trap Handling** (`kernel/trap.c`, `kernel/trampoline.S`, `kernel/kernelvector.S`):

- **User traps**: Flow through `trampoline.S:uservec` → `trap.c:usertrap()` → `trap.c:usertrapret()` → `trampoline.S:userret`
- **Kernel traps**: Flow through `kernelvector.S:kernelvec` → `trap.c:kerneltrap()` → return
- Trampoline page mapped at highest VA in both kernel and user page tables to enable page table switching
- Trapframe structure stores all 32 RISC-V registers plus kernel state

**Process Management** (`kernel/proc.c`, `kernel/proc.h`):

- Single-core CPU management with `mycpu()` function
- Process structure contains page table, trapframe, kernel stack, and PID
- Spinlocks for synchronization (even on single-core when interrupts are enabled)
- Kernel thread creation via `kthread_create()`
- Process states: UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE

**System Call Interface** (`kernel/syscall.c`, `kernel/sysproc.c`, `kernel/syscall.h`):

- System call numbers defined in `syscall.h` (1-11)
- User mode issues system calls via `ecall` instruction
- Kernel handles system calls in `usertrap()` and dispatches via `syscall()`
- Parameters passed in a0-a5 registers, return value in a0

## Code Organization

- `kernel/entry.S` - Initial entry point from QEMU bootloader
- `kernel/start.c` - Kernel initialization sequence
- `kernel/kernel.ld` - Linker script defining memory layout
- `kernel/defs.h` - Function declarations across all kernel subsystems
- `kernel/types.h` - Basic type definitions (uint64, etc.)
- `kernel/riscv.h` - RISC-V CSR access macros and page table bit definitions
- `kernel/memlayout.h` - Physical and virtual memory layout constants
- `kernel/param.h` - System parameters
- `kernel/console.c`, `kernel/uart.c`, `kernel/printf.c` - Console I/O
- `kernel/string.c` - String utilities (memset, memmove, etc.)
- `kernel/test.c` - Kernel test suite

## Important Implementation Details

### Coding Guidelines

From `AGENTS.md`:

- **Keep it clear**: Write code that is easy to read, maintain, and explain
- **Match the house style**: Reuse existing patterns, naming, and conventions
- **Search smart**: Prefer `ast-grep` for semantic queries; fall back to `rg`/`grep` when needed

### Page Table Mechanics

- Page table entries (PTEs) are 64-bit values with flags in low 10 bits
- `PTE_V` - Valid bit (must be set)
- `PTE_R`, `PTE_W`, `PTE_X` - Read/Write/Execute permissions
- `PTE_U` - User accessible (clear for kernel-only pages)
- Use `PA2PTE()` to convert physical address to PTE, `PTE2PA()` for reverse
- Always call `sfence_vma()` after modifying page tables

### Trap Handling Requirements

When implementing trap handlers:

1. Initialize with `trapinit()` and `trapinithart()` before enabling interrupts
2. For system calls: syscall number is in `trapframe->a7`, return value goes in `trapframe->a0`
3. After handling `ecall`, increment `trapframe->epc` by 4 to skip the instruction
4. Use `intr_off()` / `intr_on()` to disable/enable interrupts in critical sections

### Common scause Values

- `8` - Environment call from U-mode (system call)
- `13` - Load page fault
- `15` - Store/AMO page fault
- `0x8000000000000005` - Supervisor timer interrupt
- `0x8000000000000009` - Supervisor external interrupt

### System Call Numbers

Defined in `kernel/syscall.h`:

- `SYS_fork` (1) - Create child process
- `SYS_exit` (2) - Exit process
- `SYS_wait` (3) - Wait for child process
- `SYS_getpid` (4) - Get current PID
- `SYS_kill` (5) - Kill process
- `SYS_sleep` (6) - Sleep for ticks
- `SYS_sbrk` (7) - Grow process memory
- `SYS_uptime` (8) - Get system ticks
- `SYS_write` (9) - Write to console
- `SYS_read` (10) - Read from console
- `SYS_hello` (11) - Custom hello syscall

## Lab Structure

The repository appears to be organized by lab assignments:

- **Lab 1**: Basic kernel boot and console output
- **Lab 2**: Memory management (physical and virtual)
- **Lab 3+**: Process management and trap handling (files present but not fully integrated)

Temporary files and example code from xv6-riscv are ignored in `.gitignore`.

## Compilation Flags

- `-march=rv64gc` - Target RISC-V 64-bit with general + compressed instructions
- `-mabi=lp64` - Use LP64 ABI (long and pointer are 64-bit)
- `-ffreestanding` - No hosted environment (no stdlib)
- `-fno-builtin` - Don't assume standard function names have special meaning
- `-mcmodel=medany` - Medium any code model (supports position-independent code)
- `-O2` - Optimization level 2

## Testing

The kernel includes a built-in test suite in `kernel/test.c` that runs automatically on boot. The test suite covers:

- System call tests (hello, fork, wait, exit, etc.)
- Interrupt tests (timer, exception handling)
- Process management tests (creation, scheduling, synchronization)

To modify or add tests, edit `kernel/test.c` and rebuild the kernel.
