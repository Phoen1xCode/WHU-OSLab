// File and inode structures
#include "../include/types.h"
#include "../sync/sleeplock.h"
#include "./fs.h"

#ifndef FILE_H
#define FILE_H

// File types for struct file
enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE };

struct file {
  enum { FD_NONE_F, FD_PIPE_F, FD_INODE_F, FD_DEVICE_F } type;
  int ref;           // 引用计数
  char readable;     // 可读标志
  char writable;     // 可写标志
  struct pipe *pipe; // 管道指针
  struct inode *ip;  // inode 指针
  uint off;          // 文件偏移量
  short major;       // 设备主设备号
};

// Extract major and minor device numbers from device ID
#define major(dev) ((dev) >> 16 & 0xFFFF)
#define minor(dev) ((dev) & 0xFFFF)
#define mkdev(m, n) ((uint)((m) << 16 | (n)))

// In-memory copy of an inode
struct inode {
  uint dev;              // 设备号
  uint inum;             // Inode 编号
  int ref;               // 引用计数
  struct sleeplock lock; // 睡眠锁
  int valid;             // inode 是否已从磁盘读取

  // Copy of disk inode 磁盘 Inode 副本
  short type;
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT + 1];
};

// Map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1

#endif // FILE_H