// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../include/types.h"
#include "../sync/spinlock.h"

// kernel.ld 定义的内核代码结束地址
extern char end[];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  void *buf[PAGE_POOL_CAP];
  int n;
} page_cache;

static int initialized = 0;

void kmem_init() {
  initlock(&kmem.lock, "kmem");
  initlock(&page_cache.lock, "page_cache");
  page_cache.n = 0;
  initialized = 0;
  freerange(end, (void *)PHYSTOP);
  initialized = 1;
}

void freerange(void *pa_start, void *pa_end) {
  // 对齐起始地址
  char *start = (char *)PAGEROUNDUP((uint64)pa_start);
  // 按物理地址从高到低释放
  for (char *p = (char *)pa_end - PAGESIZE; p >= start; p -= PAGESIZE)
    free_page(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kmem_init above.)
void free_page(void *pa) {

  if (((uint64)pa % PAGESIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("free_page: invalid page");

  // 跳过初始化阶段的 memset
  if (initialized) {
    memset(pa, 1, PAGESIZE);
  }

  acquire(&page_cache.lock);
  if (page_cache.n < PAGE_POOL_CAP) {
    page_cache.buf[page_cache.n++] = pa;
    release(&page_cache.lock);
    return;
  }
  release(&page_cache.lock);

  free_page_to_freelist(pa);
}

void free_page_to_freelist(void *page) {
  struct run *r = (struct run *)page;

  acquire(&kmem.lock);

  // 按物理地址有序插入，保证freelist从低地址到高地址，能够分配连续页面
  if (kmem.freelist == NULL || (char *)r < (char *)kmem.freelist) {
    r->next = kmem.freelist;
    kmem.freelist = r;
  } else {
    struct run *prev = kmem.freelist;
    while (prev->next && (char *)prev->next < (char *)r) {
      prev = prev->next;
    }
    r->next = prev->next;
    prev->next = r;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *alloc_page(void) {
  // 优先从页面池分配
  acquire(&page_cache.lock);
  if (page_cache.n > 0) {
    void *p = page_cache.buf[--page_cache.n];
    release(&page_cache.lock);
    // 填充垃圾数据
    memset((char *)p, 5, PAGESIZE);
    return p;
  }
  release(&page_cache.lock);

  // 页面池为空，从空闲页链表分配
  struct run *r;
  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);
  if (r)
    // 填充垃圾数据
    memset((char *)r, 5, PAGESIZE);
  return (void *)r;
}

void *alloc_pages(int n) {
  if (n <= 0)
    return NULL;

  acquire(&kmem.lock);

  struct run *prev = NULL;
  struct run *cur = kmem.freelist;

  // 候选连续区间的起点、起点前驱、尾节点、长度
  struct run *seg_start = NULL;
  struct run *seg_prev = NULL;
  struct run *seg_end = NULL;
  int seg_len = 0;

  while (cur) {
    if (seg_len == 0) {
      // 新的候选区间
      seg_start = cur;
      seg_prev = prev;
      seg_end = cur;
      seg_len = 1;
    } else if ((char *)cur == (char *)seg_end + PAGESIZE) {
      // 当前节点与区间末尾连续，延长区间
      seg_end = cur;
      seg_len++;
    } else {
      // 不连续，从当前节点开始新的候选区间
      seg_start = cur;
      seg_prev = prev;
      seg_end = cur;
      seg_len = 1;
    }

    if (seg_len == n) {
      // 找到n个连续页面
      struct run *cut_end = seg_start;
      for (int i = 1; i < n; i++) {
        cut_end = cut_end->next;
      }
      struct run *rest = cut_end->next; // 剩余链表起点
      // 从空闲页链表中删除连续页面
      if (seg_prev)
        seg_prev->next = rest;
      else
        kmem.freelist = rest;

      // 断开连续页面
      cut_end->next = NULL;

      release(&kmem.lock);

      // 填充垃圾数据
      for (int i = 0; i < n; i++) {
        memset((void *)((char *)seg_start + i * PAGESIZE), 3, PAGESIZE);
      }

      return (void *)seg_start; // 返回连续页面的起始地址
    }

    // 遍历空闲页链表
    prev = cur;
    cur = cur->next;
  }

  release(&kmem.lock);
  panic("alloc_pages: not enough free pages\n");
  return NULL;
}