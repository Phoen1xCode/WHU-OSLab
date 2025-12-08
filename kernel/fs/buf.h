// Buffer cache buffer structure
#include "../fs/fs.h"
#include "../include/types.h"
#include "../sync/sleeplock.h"

#ifndef BUF_H
#define BUF_H

struct buf {
  uint valid;            // 数据是否有效
  uint disk;             // 磁盘是否拥有此缓冲区
  uint dev;              // 设备号
  uint blockno;          // 块号
  struct sleeplock lock; // 睡眠锁
  uint refcnt;           // 引用计数
  struct buf *prev;      // LRU 链表 指向最久未使用的
  struct buf *next;      // LRU 链表 指向最近使用的
  uchar data[BSIZE];     // 块数据
};

#endif // BUF_H