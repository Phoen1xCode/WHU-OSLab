#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/param.h"
#include "../include/riscv.h"
#include "../include/types.h"
#include "../proc/proc.h"

// the kernel's page table
pagetable_t kernel_pagetable;

// kernel.ld sets this to end of kernel code
extern char etext[];
// trampoline.s
extern char trampoline[];

// make a direct-map page for the kernel
// 内核直映页表构建
pagetable_t create_pagetable(void) {
  pagetable_t kpgtbl;
  kpgtbl = (pagetable_t)alloc_page();
  if (!kpgtbl)
    panic("create_pagetable: out of memory");
  memset(kpgtbl, 0, PAGESIZE);

  return kpgtbl;
}

void destroy_pagetable(pagetable_t pagetable) {
  for (int i = 0; i < 512; i++) {
    pte_t pte = pagetable[i];
    if ((pte & PTE_V) && !(pte & (PTE_R | PTE_W | PTE_X))) {
      // 中间级页表
      pagetable_t child = (pagetable_t)PTE2PA(pte);
      destroy_pagetable(child);
    }
  }
  free_page((void *)pagetable);
}

int map_page(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size,
             int perm) {
  uint64 a, last;
  pte_t *pte;

  if ((va % PAGESIZE) != 0)
    panic("map_page: va not aligned");

  if ((size % PAGESIZE) != 0)
    panic("map_page: size not aligned");

  if (size == 0)
    panic("map_page: size");

  a = va;
  last = va + size - PAGESIZE;
  for (;;) {
    if ((pte = walk_create(pagetable, a)) == 0)
      return -1;
    if (*pte & PTE_V)
      panic("map_page: remap");
    *pte = PA2PTE(pa) | perm | PTE_V;
    if (a == last)
      break;
    a += PAGESIZE;
    pa += PAGESIZE;
  }
  return 0;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void unmap_page(pagetable_t pagetable, uint64 va, uint64 npages, int do_free) {
  uint64 a;
  pte_t *pte;

  if ((va % PAGESIZE) != 0)
    panic("unmap_page: not aligned");

  for (a = va; a < va + npages * PAGESIZE; a += PAGESIZE) {
    if ((pte = walk_create(pagetable, a)) == 0)
      continue;
    if ((*pte & PTE_V) == 0)
      continue;
    if (PTE_FLAGS(*pte) == PTE_V)
      panic("unmap_page: not a leaf");
    if (do_free) {
      uint64 pa = PTE2PA(*pte);
      free_page((void *)pa);
    }
    *pte = 0;
  }
}

void map_region(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
  // 建立映射
  if (map_page(kpgtbl, va, pa, sz, perm) != 0)
    panic("map_region");
}

// Initialize the kernel_pagetable
void kvm_init(void) {
  kernel_pagetable = create_pagetable();

  // uart registers
  map_region(kernel_pagetable, UART0, UART0, PAGESIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  map_region(kernel_pagetable, VIRTIO0, VIRTIO0, PAGESIZE, PTE_R | PTE_W);

  // PLIC
  map_region(kernel_pagetable, PLIC, PLIC, 0x4000000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  map_region(kernel_pagetable, KERNBASE, KERNBASE, (uint64)etext - KERNBASE,
             PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  map_region(kernel_pagetable, (uint64)etext, (uint64)etext,
             PHYSTOP - (uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  map_region(kernel_pagetable, TRAMPOLINE, (uint64)trampoline, PAGESIZE,
             PTE_R | PTE_X);

  // allocate and map a kernel stack for each process.
  proc_mapstacks(kernel_pagetable);
}

// switch to the kernel page table
void kvm_inithart() {
  sfence_vma();
  w_satp(MAKE_SATP(kernel_pagetable));
  sfence_vma();
}

// 必要时创建中间级页表
pte_t *walk_create(pagetable_t pt, uint64 va) {
  for (int level = 2; level > 0; level--) {
    // 提取当前层级的页表项
    pte_t *pte = &pt[PX(level, va)];
    if (*pte & PTE_V) {
      // 页表项存在且有效，跳转到下一层页表
      pt = (pagetable_t)PTE2PA(*pte);
    } else {
      // 页表项无效，分配新的页表页
      pt = (pagetable_t)alloc_page();
      // 分配失败
      if (!pt)
        return NULL;
      memset(pt, 0, PAGESIZE);
      *pte = PA2PTE((unsigned long)pt) | PTE_V;
    }
  }
  return &pt[PX(0, va)];
}

// 不创建中间级页表
pte_t *walk_lookup(pagetable_t pt, uint64 va) {
  for (int level = 2; level > 0; level--) {
    pte_t *pte = &pt[PX(level, va)];
    if (*pte & PTE_V)
      // 页表项存在且有效，跳转到下一层页表
      pt = (pagetable_t)PTE2PA(*pte);
    else
      return NULL;
  }
  return &pt[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64 walkaddr(pagetable_t pagetable, uint64 va) {
  if (va >= MAXVA)
    return 0;

  uint64 pa;
  pte_t *pte = walk_create(pagetable, va);
  if (pte == 0)
    return 0;
  if ((*pte & PTE_V) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int copy_pagetable(pagetable_t old, pagetable_t new, uint64 sz) {
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for (i = 0; i < sz; i += PAGESIZE) {
    if ((pte = walk_create(old, i)) == 0)
      continue;
    if ((*pte & PTE_V) == 0)
      continue;
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if ((mem = alloc_page()) == 0) {
      unmap_page(new, 0, i / PAGESIZE, 1);
      return -1;
    }
    memmove(mem, (char *)pa, PAGESIZE);
    if (map_page(new, i, (unsigned long)mem, PAGESIZE, flags) != 0) {
      free_page(mem);
      unmap_page(new, 0, i / PAGESIZE, 1);
      return -1;
    }
  }
  return 0;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len) {
  uint64 n, va0, pa0;

  while (len > 0) {
    va0 = PAGEROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if (pa0 == 0)
      return -1;
    n = PAGESIZE - (dstva - va0);
    if (n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PAGESIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len) {
  uint64 n, va0, pa0;

  while (len > 0) {
    va0 = PAGEROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if (pa0 == 0)
      return -1;
    n = PAGESIZE - (srcva - va0);
    if (n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PAGESIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max) {
  uint64 n, va0, pa0;
  int got_null = 0;

  while (got_null == 0 && max > 0) {
    va0 = PAGEROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if (pa0 == 0)
      return -1;
    n = PAGESIZE - (srcva - va0);
    if (n > max)
      n = max;

    char *p = (char *)(pa0 + (srcva - va0));
    while (n > 0) {
      if (*p == '\0') {
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PAGESIZE;
  }
  if (got_null) {
    return 0;
  } else {
    return -1;
  }
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len) {
  struct proc *p = myproc();
  if (user_dst) {
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len) {
  struct proc *p = myproc();
  if (user_src) {
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Wrapper function for uvmfirst: copy data to the first user page
void uvmfirst(pagetable_t pagetable, uchar *data, uint64 sz) {
  char *mem;

  // Allocate one page
  mem = alloc_page();
  if (mem == 0)
    panic("uvmfirst: out of memory");

  // Copy data to the page
  memmove(mem, data, sz);

  // Map the page at virtual address 0
  if (map_page(pagetable, 0, (uint64)mem, PAGESIZE, PTE_W | PTE_R | PTE_X) < 0)
    panic("uvmfirst: map_page failed");
}

// Wrapper function for uvmalloc: allocate and map pages from oldsz to newsz
uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int perm) {
  char *mem;
  uint64 a;

  if (newsz < oldsz)
    return oldsz;

  for (a = PAGEROUNDUP(oldsz); a < newsz; a += PAGESIZE) {
    mem = alloc_page();
    if (mem == 0)
      return 0;
    memset(mem, 0, PAGESIZE);
    if (map_page(pagetable, a, (uint64)mem, PAGESIZE, perm) < 0) {
      free_page(mem);
      return 0;
    }
  }
  return newsz;
}

// Wrapper function for uvmdealloc: unmap and free pages from newsz down to
// newsz2
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz) {
  if (newsz >= oldsz)
    return oldsz;

  // Unmap and free pages from newsz to oldsz
  unmap_page(pagetable, newsz, (oldsz - newsz) / PAGESIZE, 1);

  return newsz;
}

// Wrapper function for uvmcopy: copy user memory from parent to child
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz) {
  return copy_pagetable(old, new, sz);
}

int ismapped(pagetable_t pagetable, uint64 va) {
  pte_t *pte = walk_create(pagetable, va);
  if (pte == 0) {
    return 0;
  }
  if (*pte & PTE_V) {
    return 1;
  }
  return 0;
}

uint64 vmfault(pagetable_t pagetable, uint64 va, int read) {
  uint64 mem;
  struct proc *p = myproc();

  if (va >= p->sz)
    return 0;
  va = PAGEROUNDDOWN(va);
  if (ismapped(pagetable, va)) {
    return 0;
  }
  mem = (uint64)alloc_page();
  if (mem == 0)
    return 0;
  memset((void *)mem, 0, PAGESIZE);
  if (map_page(p->pagetable, va, mem, PAGESIZE, PTE_W | PTE_U | PTE_R) != 0) {
    free_page((void *)mem);
    return 0;
  }
  return mem;
}
