#include "types.h"

struct context;
struct spinlock;
struct proc;

// console.c
void consoleinit(void);
void consputc(int c);
void consputs(const char *s);
void consoleintr(int c);
void clear_screen(void);


// printf.c
void printfinit(void);
int printf(const char *fmt, ...);
void panic(char *) __attribute__((noreturn));

// uart.c
void uartinit(void);
void uartputc(char c);
void uartputs(const char *s);
void uartintr(void);


// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// kalloc.c
void kinit(void);
void* kalloc(void);
void* kalloc_internal(const char*, int);
void kfree(void*);
void kfree_internal(void*, const char*, int);
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

// trap.c
void trapinit(void);
void trapinithart(void);
uint64 usertrap(void);
void usertrapret(void);
void kerneltrap(void);
void clockintr(void);
int devintr(void);
uint64 get_ticks(void);

// syscall.c
void syscall(void);
void argint(int, int*);
void argaddr(int, uint64*);
int fetchaddr(uint64, uint64*);
int fetchstr(uint64, char*, int);
int argstr(int, char*, int);

// sysproc.c
int kill(int);


// spinlock.c
void initlock(struct spinlock *, char *);
int holding(struct spinlock *);
void push_off(void);
void pop_off(void);

// proc.c
int cpuid(void);
struct cpu* mycpu(void);
struct proc* myproc(void);
void procinit(void);
void userinit(void);
int growproc(int);
void forkret(void);
void sleep(void*, struct spinlock*);
void wakeup(void*);
void yield(void);
void exit(int);
int fork(void);
int kthread_create(void (*func)());
int wait(uint64);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void setkilled(struct proc*);
int killed(struct proc*);

// swtch.S
void swtch(struct context*, struct context*);


// spinlock.c
void initlock(struct spinlock *, char *);
void acquire(struct spinlock *);
void release(struct spinlock *);
int holding(struct spinlock *);
void push_off(void);
void pop_off(void);

// plic.c
void            plicinit(void);
void            plicinithart(void);
int             plic_claim(void);
void            plic_complete(int);

// test.c
void test_main(void);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
