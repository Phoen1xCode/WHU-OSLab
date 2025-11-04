
// Enhanced Physical Memory Allocator
// Features:
// - Memory statistics (free/used/total pages)
// - Reference counting to prevent double-free
// - Allocation tracking for leak detection
// - Debug information output

#include "defs.h"
#include "memlayout.h"
#include "riscv.h"
#include "types.h"

void kfree(void *pa);
void freerange(void *pa_start, void *pa_end);

// First address after kernel defined by kernel.ld.
extern char end[];

// 每个物理页的元数据
struct page_info {
  uint32 refcount;        // 引用计数：0=空闲, >0=已分配
  uint32 alloc_line;      // 分配时的代码行号 用于追踪泄漏
  const char *alloc_file; // 分配时的文件名
};

// 空闲链表节点
struct run {
  struct run *next;
};

// 内存分配器全局数据
struct {
  struct run *freelist;         // 空闲页链表
  struct page_info *page_array; // 页信息数组
  uint64 total_pages;           // 总页数
  uint64 free_pages;            // 空闲页数
  uint64 used_pages;            // 已使用页数
  uint64 page_base;             // 第一个可分配页的物理地址
} kmem;

// 物理地址 -> 页号
static inline uint64 pa_to_pagenum(void *pa) {
  return ((uint64)pa - kmem.page_base) / PAGESIZE;
}

// 页号 -> 物理地址
static inline void *pagenum_to_pa(uint64 pagenum) {
  return (void *)(kmem.page_base + pagenum * PAGESIZE);
}

// 获取页信息
static inline struct page_info *get_page_info(void *pa) {
  uint64 pagenum = pa_to_pagenum(pa);
  if (pagenum >= kmem.total_pages)
    return 0;
  return &kmem.page_array[pagenum];
}

/*
  初始化内存分配器
  - 初始化统计信息
  - 初始化页信息数组
  - 释放所有可用内存
*/
void kinit(void) {
  kmem.freelist = 0;
  kmem.free_pages = 0;
  kmem.used_pages = 0;

  // 计算可分配区域的起始地址（页对齐）
  kmem.page_base = PAGEROUNDUP((uint64)end);

  // 计算总页数
  kmem.total_pages = (PHYSTOP - kmem.page_base) / PAGESIZE;

  // 页信息数组放在第一个页中（需要预留足够空间）
  // 每个 page_info 占用 16 字节，一个 4KB 页可以存储 256 个页的信息
  uint64 pages_needed =
      (kmem.total_pages * sizeof(struct page_info) + PAGESIZE - 1) / PAGESIZE;

  kmem.page_array = (struct page_info *)kmem.page_base;

  // 初始化所有页信息为0
  memset(kmem.page_array, 0, pages_needed * PAGESIZE);

  // 将页信息数组占用的页标记为已使用
  for (uint64 i = 0; i < pages_needed; i++) {
    kmem.page_array[i].refcount = 1;
    kmem.used_pages++;
  }

  // 释放剩余的页
  char *start = (char *)pagenum_to_pa(pages_needed);
  freerange(start, (void *)PHYSTOP);

  printf("[KMEM] Memory allocator initialized:\n");
  printf("  Total pages: %lu (%lu MB)\n", kmem.total_pages,
         (kmem.total_pages * PAGESIZE) / (1024 * 1024));
  printf("  Metadata pages: %lu\n", pages_needed);
  printf("  Free pages: %lu (%lu MB)\n", kmem.free_pages,
         (kmem.free_pages * PAGESIZE) / (1024 * 1024));
}

// 将一段连续的物理内存区域按页（4096字节）释放
void freerange(void *pa_start, void *pa_end) {
  char *p;
  p = (char *)PAGEROUNDUP((uint64)pa_start); // 向上对齐到页边界
  for (; p + PAGESIZE <= (char *)pa_end; p += PAGESIZE) {
    kfree(p); // 逐页释放
  }
}

/*
  内部版本的 kfree，带文件名和行号追踪
  应该通过宏调用: KFREE(pa)
*/
void kfree_internal(void *pa, const char *file, int line) {
  struct page_info *info;

  // 安全检查
  if (((uint64)pa % PAGESIZE != 0) || (char *)pa < (char *)kmem.page_base ||
      (uint64)pa >= PHYSTOP) {
    printf("[KMEM ERROR] kfree: invalid address 0x%p at %s:%d\n", pa, file,
           line);
    panic("kfree");
  }

  info = get_page_info(pa);
  if (!info) {
    printf("[KMEM ERROR] kfree: cannot get page info for 0x%p at %s:%d\n", pa,
           file, line);
    panic("kfree");
  }

  // 检测 double-free
  if (info->refcount == 0 && info->alloc_file != 0) {
    printf("[KMEM ERROR] Double-free detected!\n");
    printf("  Address: 0x%p\n", pa);
    printf("  Called from: %s:%d\n", file, line);
    printf("  Last allocated at: %s:%d\n",
           info->alloc_file ? info->alloc_file : "unknown", info->alloc_line);
    panic("double-free");
  }

  // 记录页面之前是否被使用（用于正确更新统计信息）
  int was_used = (info->refcount > 0);

  // 如果 refcount > 0，减少引用计数
  if (info->refcount > 0) {
    info->refcount--;
  }

  // 如果引用计数为0（或首次释放），真正释放到空闲链表
  if (info->refcount == 0) {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PAGESIZE);

    // 插入空闲链表
    struct run *r = (struct run *)pa;
    r->next = kmem.freelist; // 头插法
    kmem.freelist = r;

    // 更新统计信息
    kmem.free_pages++;
    // 只有当页面之前确实被使用时，才减少 used_pages
    if (was_used) {
      kmem.used_pages--;
    }

    // 清除分配信息
    info->alloc_file = 0;
    info->alloc_line = 0;
  }
}

/*
  标准 kfree 接口（用于 freerange 等内部调用）
*/
void kfree(void *pa) { kfree_internal(pa, "internal", 0); }

/*
  内部版本的 kalloc，带文件名和行号追踪
  应该通过宏调用: KALLOC()
*/
void *kalloc_internal(const char *file, int line) {
  struct run *r;
  struct page_info *info;

  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;

    // 更新统计信息
    kmem.free_pages--;
    kmem.used_pages++;

    // 记录分配信息
    info = get_page_info((void *)r);
    if (info) {
      info->refcount = 1;
      info->alloc_file = file;
      info->alloc_line = line;
    }

    // Fill with junk
    memset((char *)r, 6, PAGESIZE);
  } else {
    printf("[KMEM ERROR] Out of memory! Called from %s:%d\n", file, line);
  }

  return (void *)r;
}

/*
  标准 kalloc 接口（用于不需要追踪的场景）
*/
void *kalloc(void) { return kalloc_internal("unknown", 0); }

/*
  增加页的引用计数（用于页共享）
*/
int kref(void *pa) {
  struct page_info *info = get_page_info(pa);
  if (!info || info->refcount == 0) {
    printf("[KMEM ERROR] kref: invalid page 0x%p\n", pa);
    return -1;
  }
  info->refcount++;
  return 0;
}

/*
  打印内存统计信息
*/
void kmem_stats(void) {
  printf("\n=== Memory Statistics ===\n");
  printf("Total pages:  %lu (%lu MB)\n", kmem.total_pages,
         (kmem.total_pages * PAGESIZE) / (1024 * 1024));
  printf("Free pages:   %lu (%lu MB) [%lu%%]\n", kmem.free_pages,
         (kmem.free_pages * PAGESIZE) / (1024 * 1024),
         (kmem.free_pages * 100) / kmem.total_pages);
  printf("Used pages:   %lu (%lu MB) [%lu%%]\n", kmem.used_pages,
         (kmem.used_pages * PAGESIZE) / (1024 * 1024),
         (kmem.used_pages * 100) / kmem.total_pages);
  printf("========================\n\n");
}

/*
  检测内存泄漏：遍历所有已分配的页，打印分配信息
*/
void kmem_leak_check(void) {
  uint64 leaked = 0;

  printf("\n=== Memory Leak Check ===\n");

  for (uint64 i = 0; i < kmem.total_pages; i++) {
    struct page_info *info = &kmem.page_array[i];
    if (info->refcount > 0) {
      void *pa = pagenum_to_pa(i);
      printf("Page 0x%p: refcount=%d, allocated at %s:%d\n", pa, info->refcount,
             info->alloc_file ? info->alloc_file : "unknown", info->alloc_line);
      leaked++;
    }
  }

  if (leaked == 0) {
    printf("No memory leaks detected!\n");
  } else {
    printf("Total leaked pages: %lu (%lu KB)\n", leaked,
           (leaked * PAGESIZE) / 1024);
  }
  printf("=========================\n\n");
}

/*
  打印空闲链表信息，用于调试函数使用
*/
void kmem_dump_freelist(void) {
  struct run *r = kmem.freelist;
  int count = 0;

  printf("\n=== Free List Dump ===\n");
  printf("Free list head: 0x%p\n", r);

  while (r && count < 10) { // 只打印前10个
    printf("  [%d] 0x%p -> 0x%p\n", count, r, r->next);
    r = r->next;
    count++;
  }

  if (r) {
    printf("  ... (more pages)\n");
  }
  printf("Total free pages in list: %lu\n", kmem.free_pages);
  printf("======================\n\n");
}
