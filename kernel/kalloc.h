// Enhanced Memory Allocator Header
// Provides macros for tracked memory allocation

#ifndef KALLOC_H
#define KALLOC_H

// 函数声明
void kinit(void);
void *kalloc(void);
void kfree(void *pa);
void *kalloc_internal(const char *file, int line);
void kfree_internal(void *pa, const char *file, int line);
int kref(void *pa);

// 内存统计和调试函数
void kmem_stats(void);
void kmem_leak_check(void);
void kmem_dump_freelist(void);

// 带追踪的分配/释放宏
// 使用 KALLOC() 和 KFREE() 可以自动记录调用位置
#ifdef KMEM_DEBUG
  #define KALLOC() kalloc_internal(__FILE__, __LINE__)
  #define KFREE(pa) kfree_internal(pa, __FILE__, __LINE__)
#else
  #define KALLOC() kalloc()
  #define KFREE(pa) kfree(pa)
#endif

// 引用计数宏
#define KREF(pa) kref(pa)

// 内存统计宏（可以随时调用）
#define KMEM_STATS() kmem_stats()
#define KMEM_LEAK_CHECK() kmem_leak_check()
#define KMEM_DUMP_FREELIST() kmem_dump_freelist()

#endif // KALLOC_H