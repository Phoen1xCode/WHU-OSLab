#include "types.h"

// Forward type declarations
typedef uint64 pte_t;
typedef uint64 *pagetable_t;

// console.c
void consputc(int c);
void consputs(const char *s);
void clear_screen(void);

// printf.c
int printf(const char *fmt, ...);
void panic(char *) __attribute__((noreturn));

// uart.c
void uartputc(char c);
void uartputs(const char *s);


// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// kalloc.c
void* kalloc(void);
void* kalloc_internal(const char*, int);
void kfree(void*);
void kfree_internal(void*, const char*, int);
void kinit(void);
void freerange(void*, void*);
int kref(void*);
void kmem_stats(void);
void kmem_leak_check(void);
void kmem_dump_freelist(void);

// vm.c
void kvminit(void);
void kvminithart(void);
pagetable_t kvmmake(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
int mappages(pagetable_t, uint64, uint64, uint64, int);
pte_t* walk(pagetable_t, uint64, int);
uint64 walkaddr(pagetable_t, uint64);
pagetable_t uvmcreate(void);
void uvmfirst(pagetable_t, uchar*, uint);
uint64 uvmalloc(pagetable_t, uint64, uint64, int);
uint64 uvmdealloc(pagetable_t, uint64, uint64);
void uvmunmap(pagetable_t, uint64, uint64, int);
int uvmcopy(pagetable_t, pagetable_t, uint64);
void uvmclear(pagetable_t, uint64);
int copyout(pagetable_t, uint64, char*, uint64);
int copyin(pagetable_t, char*, uint64, uint64);
int copyinstr(pagetable_t, char*, uint64, uint64);
void freewalk(pagetable_t);
void uvmfree(pagetable_t, uint64);
void proc_mapstacks(pagetable_t);
int ismapped(pagetable_t, uint64);
uint64 vmfault(pagetable_t, uint64, uint64);
