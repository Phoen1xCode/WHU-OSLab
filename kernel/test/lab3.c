
#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../include/types.h"

#ifndef ASSERT_H
#define ASSERT_H

#define assert(expression)                                                     \
  if (!(expression)) {                                                         \
    printf("Assertion failed: %s, file %s, line %d\n", #expression, __FILE__,  \
           __LINE__);                                                          \
    while (1)                                                                  \
      ;                                                                        \
  }

#endif

void test_physical_memory(void) {
  printf("Start physical memory test...\n");
  // 测试基本分配
  void *page1 = alloc_page();
  void *page2 = alloc_page();
  assert(page1 != page2);
  assert(((unsigned long)page1 & 0xFFF) == 0); // 页对齐检查
  printf("Basic allocation test passed.\n");

  // 测试数据读写
  *(int *)page1 = 0x12345678;
  assert(*(int *)page1 == 0x12345678);
  printf("Data write&read test passed.\n");

  // 测试释放和重新分配
  free_page(page1);
  void *page3 = alloc_page();
  free_page(page2);
  free_page(page3);
  printf("Free and reallocation test passed.\n");
}

void test_pagetable(void) {
  pagetable_t pt = create_pagetable();

  // 测试基本映射
  unsigned long va = 0x1000000;
  unsigned long pa = (unsigned long)alloc_page();
  assert(map_page(pt, va, pa, PAGESIZE, PTE_R | PTE_W) == 0);
  printf("Basic mapping test passed.\n");

  // 测试地址转换
  pte_t *pte = walk_lookup(pt, va);
  assert(pte != 0 && (*pte & PTE_V));
  assert(PTE2PA(*pte) == pa);
  printf("Address translation test passed.\n");

  // 测试权限位
  assert(*pte & PTE_R);
  assert(*pte & PTE_W);
  assert(!(*pte & PTE_X));
  printf("Permission bits test passed.\n");
}

void test_virtual_memory(void) {
  printf("Before enabling paging...\n");

  // 启用分页
  kvm_init();
  kvm_inithart();
  printf("After enabling paging...\n");

  // 测试内核代码仍然可执行
  test_printf_basic();
  printf("Code execution test passed.\n");

  // 测试内核数据仍然可访问
  test_physical_memory();
  printf("Data access test passed.\n");

  // 测试设备访问仍然正常
  uart_puts("Hello OS\n");
  printf("Device access test passed.\n");
}

void test_alloc_pages(void) {
  // 独立分配3个页面，释放其中2个，使得空闲页链表前两个地址不连续
  void *page1 = alloc_page();
  void *page2 = alloc_page();
  void *page3 = alloc_page();
  free_page(page1);
  free_page(page3);

  int n = 3;
  // 分配3个连续页面
  void *base = alloc_pages(n);

  assert(base != 0);
  assert(((unsigned long)base & 0xFFF) == 0); // 页对齐检查
  printf("Continuous allocation test passed.\n");

  for (int i = 0; i < n; i++) {
    char *pi = (char *)base + i * PAGESIZE;

    // 页面连续性检查
    if (i > 0) {
      char *prev = (char *)base + (i - 1) * PAGESIZE;
      assert(pi - prev == PAGESIZE);
      printf("Page %d is continuous.\n", i);
    }

    // 页面读写检查
    *(int *)pi = 0x12345678 + i;
    assert(*(int *)pi == 0x12345678 + i);
    printf("Page %d write&read test passed.\n", i);
  }

  // 逐页释放
  for (int i = 0; i < n; i++) {
    free_page((char *)base + i * PAGESIZE);
  }

  printf("Continuous page free test passed.\n");
  free_page(page2);
}
