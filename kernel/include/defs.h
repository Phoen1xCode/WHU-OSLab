#include "../include/types.h"

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct sleeplock;
struct spinlock;
struct stat;
struct superblock;

// uart.c
void uart_putc(char c);
void uart_puts(const char *s);
int uart_getc(void);

// console.c
void console_putc(int c);
void console_puts(const char *s);
void clear_screen(void);

// printf.c
void print_int(long long xx, int base, int sign);
void print_ptr(uint64 x);
int printf(const char *fmt, ...);
void panic(char *) __attribute__((noreturn));

// string.c
void *memset(void *dst, int c, uint n);
int memcmp(const void *, const void *, uint);
void *memmove(void *, const void *, uint);
void *memcpy(void *, const void *, uint);
int strncmp(const char *, const char *, uint);
char *strncpy(char *, const char *, int);
char *safestrcpy(char *, const char *, int);
int strlen(const char *);
int strcmp(const char *, const char *);

// kalloc.c
void kmem_init(void);
void freerange(void *, void *);
void free_page(void *);
void free_page_to_freelist(void *);
void *alloc_page(void);
void *alloc_pages(int n);

// vm.c
pagetable_t create_pagetable(void);
void destroy_pagetable(pagetable_t);
int map_page(pagetable_t, uint64, uint64, uint64, int);
void unmap_page(pagetable_t, uint64, uint64, int);
void map_region(pagetable_t, uint64, uint64, uint64, int);
void kvm_init(void);
void kvm_inithart(void);
pte_t *walk_create(pagetable_t, uint64);
pte_t *walk_lookup(pagetable_t, uint64);
uint64 walkaddr(pagetable_t, uint64);
int copy_pagetable(pagetable_t, pagetable_t, uint64);
int copyout(pagetable_t, uint64, char *, uint64);
int copyin(pagetable_t, char *, uint64, uint64);
int copyinstr(pagetable_t, char *, uint64, uint64);
int either_copyout(int, uint64, void *, uint64);
int either_copyin(void *, int, uint64, uint64);

void uvmfirst(pagetable_t, uchar *, uint64);
uint64 uvmalloc(pagetable_t, uint64, uint64, int);
uint64 uvmdealloc(pagetable_t, uint64, uint64);
int uvmcopy(pagetable_t, pagetable_t, uint64);
int ismapped(pagetable_t, uint64);
uint64 vmfault(pagetable_t, uint64, int);

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
void argint(int, int *);
void argaddr(int, uint64 *);
int fetchaddr(uint64, uint64 *);
int fetchstr(uint64, char *, int);
int argstr(int, char *, int);

// sysproc.c
int kill(int);

// spinlock.c
void initlock(struct spinlock *, char *);
int holding(struct spinlock *);
void push_off(void);
void pop_off(void);

// proc.c
int cpuid(void);
struct cpu *mycpu(void);
struct proc *myproc(void);
void proc_mapstacks(pagetable_t);
void procinit(void);
void userinit(void);
int growproc(int);
void forkret(void);
void sleep(void *, struct spinlock *);
void wakeup(void *);
void yield(void);
void exit(int);
int fork(void);
int kthread_create(void (*func)());
int wait(uint64);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void setkilled(struct proc *);
int killed(struct proc *);

// swtch.S
void swtch(struct context *, struct context *);

// spinlock.c
void initlock(struct spinlock *, char *);
void acquire(struct spinlock *);
void release(struct spinlock *);
int holding(struct spinlock *);
void push_off(void);
void pop_off(void);

// plic.c
void plicinit(void);
void plicinithart(void);
int plic_claim(void);
void plic_complete(int);

// test.c
void test_main(void);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

// sleeplock.c
void initsleeplock(struct sleeplock *, char *);
void acquiresleep(struct sleeplock *);
void releasesleep(struct sleeplock *);
int holdingsleep(struct sleeplock *);

// bio.c
void binit(void);
struct buf *bread(uint, uint);
void brelse(struct buf *);
void bwrite(struct buf *);
void bpin(struct buf *);
void bunpin(struct buf *);

// log.c
void initlog(int, struct superblock *);
void log_write(struct buf *);
void begin_op(void);
void end_op(void);

// fs.c
void fsinit(int);
void iinit(void);
struct inode *ialloc(uint, short);
struct inode *idup(struct inode *);
void ilock(struct inode *);
void iunlock(struct inode *);
void iput(struct inode *);
void iunlockput(struct inode *);
void iupdate(struct inode *);
void itrunc(struct inode *);
void stati(struct inode *, struct stat *);
int readi(struct inode *, int, uint64, uint, uint);
int writei(struct inode *, int, uint64, uint, uint);
int namecmp(const char *, const char *);
struct inode *dirlookup(struct inode *, char *, uint *);
int dirlink(struct inode *, char *, uint);
struct inode *namei(char *);
struct inode *nameiparent(char *, char *);

// file.c
void fileinit(void);
struct file *filealloc(void);
struct file *filedup(struct file *);
void fileclose(struct file *);
int filestat(struct file *, uint64);
int fileread(struct file *, uint64, int);
int filewrite(struct file *, uint64, int);

// virtio_disk.c
void virtio_disk_init(void);
void virtio_disk_rw(struct buf *, int);
void virtio_disk_intr(void);

// pipe.c - Inter-Process Communication (kernel/ipc/)
int pipealloc(struct file **, struct file **);
void pipeclose(struct pipe *, int);
int piperead(struct pipe *, uint64, int);
int pipewrite(struct pipe *, uint64, int);

// TEST
// lab2.c
void test_printf_basic();
void test_printf_edge_cases();
void test_clear_screen();
// lab3.c
void test_physical_memory();
void test_pagetable();
void test_virtual_memory();
void test_alloc_pages();
// lab4.c
void pt_init(void);
void test_timer_interrupt(void);
void test_exception_handling(void);
