#include "../fs/buf.h"
#include "../fs/fs.h"
#include "../include/defs.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../sync/sleeplock.h"
#include "../sync/spinlock.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  int n;                // 日志中的块数量
  int block[LOGBLOCKS]; // 每个日志块对应的磁盘块号
};

struct log {
  struct spinlock lock;
  int start;           // 日志在磁盘上的起始块号
  int outstanding;     // 正在执行的文件系统调用数量
  int committing;      // 是否正在提交日志 1 表示正在提交
  int dev;             // 设备号
  struct logheader lh; // 内存中的日志头
};
struct log logbuf;

static void recover_from_log(void);
static void commit(void);

void initlog(int dev, struct superblock *sb) {
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&logbuf.lock, "log");
  logbuf.start = sb->logstart;
  logbuf.dev = dev;
  recover_from_log(); // 进行崩溃恢复
}

// Copy committed blocks from log to their home location
static void install_trans(int recovering) {
  int tail;

  for (tail = 0; tail < logbuf.lh.n; tail++) {
    if (recovering) {
      printf("recovering tail %d dst %d\n", tail, logbuf.lh.block[tail]);
    }
    struct buf *lbuf =
        bread(logbuf.dev, logbuf.start + tail + 1); // read log block
    struct buf *dbuf = bread(logbuf.dev, logbuf.lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE); // copy block to dst
    bwrite(dbuf);                           // write dst to disk
    if (recovering == 0)
      bunpin(dbuf);
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void read_head(void) {
  struct buf *buf = bread(logbuf.dev, logbuf.start);
  struct logheader *lh = (struct logheader *)(buf->data);
  int i;
  logbuf.lh.n = lh->n;
  for (i = 0; i < logbuf.lh.n; i++) {
    logbuf.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void write_head(void) {
  struct buf *buf = bread(logbuf.dev, logbuf.start);
  struct logheader *hb = (struct logheader *)(buf->data);
  int i;
  hb->n = logbuf.lh.n;
  for (i = 0; i < logbuf.lh.n; i++) {
    hb->block[i] = logbuf.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void recover_from_log(void) {
  read_head();
  install_trans(1); // if committed, copy from log to disk
  logbuf.lh.n = 0;
  write_head(); // clear the log
}

// called at the start of each FS system call.
// 开始文件系统操作
void begin_op(void) {
  acquire(&logbuf.lock);
  while (1) {
    if (logbuf.committing) {
      sleep(&logbuf, &logbuf.lock);
    } else if (logbuf.lh.n + (logbuf.outstanding + 1) * MAXOPBLOCKS >
               LOGBLOCKS) { // 防止日志空间不足
      // this op might exhaust log space; wait for commit.
      sleep(&logbuf, &logbuf.lock);
    } else {
      logbuf.outstanding += 1;
      release(&logbuf.lock);
      break;
    }
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
// 结束文件系统操作
void end_op(void) {
  int do_commit = 0;

  acquire(&logbuf.lock);
  logbuf.outstanding -= 1;
  if (logbuf.committing)
    panic("log.committing");
  if (logbuf.outstanding == 0) {
    do_commit = 1;
    logbuf.committing = 1;
  } else {
    // begin_op() may be waiting for log space,
    // and decrementing logbuf.outstanding has decreased
    // the amount of reserved space.
    wakeup(&logbuf);
  }
  release(&logbuf.lock);

  if (do_commit) {
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    commit();
    acquire(&logbuf.lock);
    logbuf.committing = 0;
    wakeup(&logbuf);
    release(&logbuf.lock);
  }
}

// Copy modified blocks from cache to log.
static void write_log(void) {
  int tail;

  for (tail = 0; tail < logbuf.lh.n; tail++) {
    struct buf *to = bread(logbuf.dev, logbuf.start + tail + 1); // log block
    struct buf *from = bread(logbuf.dev, logbuf.lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to); // write the log
    brelse(from);
    brelse(to);
  }
}

static void commit(void) {
  if (logbuf.lh.n > 0) {
    write_log();      // Write modified blocks from cache to log
    write_head();     // Write header to disk -- the real commit
    install_trans(0); // Now install writes to home locations
    logbuf.lh.n = 0;
    write_head(); // Erase the transaction from the log
  }
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache by increasing refcnt.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...) // 读取块
//   modify bp->data[] // 修改块数据
//   log_write(bp) // 记录日志
//   brelse(bp) // 释放块
// 记录块修改 用来替换bwrite()
void log_write(struct buf *b) {
  int i;

  acquire(&logbuf.lock);
  // 安全检查
  if (logbuf.lh.n >= LOGBLOCKS)
    panic("too big a transaction");
  if (logbuf.outstanding < 1)
    panic("log_write outside of trans");

  for (i = 0; i < logbuf.lh.n; i++) {
    if (logbuf.lh.block[i] == b->blockno) // log absorption
      break;
  }
  logbuf.lh.block[i] = b->blockno;
  if (i == logbuf.lh.n) { // Add new block to log?
    bpin(b);
    logbuf.lh.n++;
  }
  release(&logbuf.lock);
}