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

// 文件系统相关
#define SYS_open    11  // 打开文件
#define SYS_close   12  // 关闭文件
#define SYS_fstat   13  // 获取文件状态
#define SYS_pipe    14  // 创建管道
#define SYS_dup     15  // 复制文件描述符
#define SYS_chdir   16  // 改变当前目录
#define SYS_mkdir   17  // 创建目录
#define SYS_mknod   18  // 创建设备文件或管道
#define SYS_unlink  19  // 删除文件
#define SYS_link    20  // 创建硬链接

// 自定义系统调用（保留）
// #define SYS_hello   11   // say hello

// 系统调用总数 用于边界检查
#define NSYSCALL    21

#endif // SYSCALL_H