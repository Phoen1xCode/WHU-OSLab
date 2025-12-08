// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "../fs/buf.h"
#include "../fs/fs.h"
#include "../include/defs.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../include/types.h"
#include "../sync/sleeplock.h"
#include "../sync/spinlock.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF]; // 缓冲区数组 固定大小

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head; // 双向链表的哨兵节点
} bcache;

void binit(void) {
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // 创建循环双向链表
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;

  // 将所有缓冲区插入到链表头部
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *bget(uint dev, uint blockno) {
  struct buf *b;

  acquire(&bcache.lock);

  // 检查缓存中是否已有该块 遍历链表
  for (b = bcache.head.next; b != &bcache.head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) { // 缓存命中
      b->refcnt++;                                // 增加引用计数
      release(&bcache.lock);                      // 释放全局锁
      acquiresleep(&b->lock);                     // 获取缓冲区锁
      return b;
    }
  }

  // 缓存未命中 使用 LRU 策略分配新缓冲区
  // 从链表尾部回收最久未使用且未被引用的缓冲区
  for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
    if (b->refcnt == 0) { // 空闲缓冲区
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->disk = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// 读取块数据
// Return a locked buf with the contents of the indicated block.
struct buf *bread(uint dev, uint blockno) {
  struct buf *b;

  b = bget(dev, blockno); // 获取缓冲区
  if (!b->valid) {        // 数据无效 需要从磁盘读取
    virtio_disk_rw(b, 0); // 从磁盘读取数据 b->disk = 0
    b->valid = 1;         // 标记数据为有效
  }
  return b;
}

// 写入块数据
// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) {
  if (!holdingsleep(&b->lock))
    panic("bwrite");

  b->disk = 1;          // 标记缓冲区已修改
  virtio_disk_rw(b, 1); // 写入磁盘
}

// 释放缓冲区
// Move to the head of the most-recently-used list.
void brelse(struct buf *b) {
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock); // 释放缓冲区锁

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) { // 没有其他进程使用
    // 将缓冲区移到链表头部（标记为最近使用）
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }

  release(&bcache.lock);
}

// Pin buffer - increase reference count
void bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++; // 增加引用计数
  release(&bcache.lock);
}

// Unpin buffer - decrease reference count
void bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--; // 减少引用计数
  release(&bcache.lock);
}

// 将设备上所有脏缓冲区写回磁盘
void flush_all_blocks(uint dev) {
  struct buf *b;

  acquire(&bcache.lock);

  for (b = bcache.head.next; b != &bcache.head; b = b->next) {
    // 只处理属于指定设备且有效的缓冲区
    if (b->dev == dev && b->valid) {
      b->refcnt++; // 增加引用计数，防止被回收
      release(&bcache.lock);

      acquiresleep(&b->lock);

      virtio_disk_rw(b, 1); // 写入磁盘

      releasesleep(&b->lock);

      acquire(&bcache.lock);
      b->refcnt--;

      if (b->refcnt == 0) {
        wakeup(b); // 唤醒等待该缓冲区的进程
      }
    }
  }
  release(&bcache.lock);
}