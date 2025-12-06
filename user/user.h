/*
 * user.h - 用户态系统调用声明
 *
 * 用户程序包含此头文件来使用系统调用
 * 实际的系统调用存根由 usys.pl 生成
 */

#ifndef USER_H
#define USER_H

// 类型定义
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned long  uint64;

// 系统调用声明
// 进程相关
int fork(void);
void exit(int status) __attribute__((noreturn));
int wait(int *status);
int getpid(void);
int kill(int pid);
int sleep(int ticks);

// 内存相关
char* sbrk(int n);

// 系统信息
int uptime(void);

// I/O 相关
int write(int fd, const void *buf, int n);
int read(int fd, void *buf, int n);

// 自定义系统调用
void hello(void);

// 标准文件描述符
#define STDIN  0
#define STDOUT 1
#define STDERR 2

#endif // USER_H