/*
 * syscall.h - 系统调用号定义
 *
 * 定义所有系统调用的唯一编号
 * 用户态通过将系统调用号放入 a7 寄存器，然后执行 ecall 指令来发起系统调用
 */

#ifndef SYSCALL_H
#define SYSCALL_H

// 系统调用号定义
// 进程相关
#define SYS_fork     1   // 创建子进程
#define SYS_exit     2   // 退出进程
#define SYS_wait     3   // 等待子进程
#define SYS_getpid   4   // 获取当前进程ID
#define SYS_kill     5   // 杀死进程
#define SYS_sleep    6   // 睡眠指定时间

// 内存相关
#define SYS_sbrk     7   // 增长进程内存

// 系统信息
#define SYS_uptime   8   // 获取系统运行时间（ticks）

// 控制台I/O (简化版)
#define SYS_write    9   // 写入（目前仅支持控制台）
#define SYS_read    10   // 读取（目前仅支持控制台）

// 自定义系统调用
#define SYS_hello   11   // say hello

// 未来扩展：文件系统相关
// #define SYS_open    11
// #define SYS_close   12
// #define SYS_fstat   13
// #define SYS_exec    14
// #define SYS_pipe    15
// #define SYS_dup     16
// #define SYS_chdir   17
// #define SYS_mkdir   18
// #define SYS_mknod   19
// #define SYS_unlink  20
// #define SYS_link    21

// 系统调用总数 用于边界检查
#define NSYSCALL    12

#endif // SYSCALL_H