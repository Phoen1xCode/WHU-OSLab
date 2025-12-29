#include "./include/defs.h"
#include "./include/memlayout.h"
#include "./include/param.h"
#include "./include/riscv.h"

void run_lab(int lab) {
  switch (lab) {
  // Lab1
  case 1:
    uart_puts("Hello OS");
    break;

  // Lab2
  case 2:
    test_printf_basic();
    test_clear_screen();
    test_printf_edge_cases();
    break;

  // Lab3
  case 3:
    kmem_init();
    test_physical_memory();
    test_pagetable();
    test_virtual_memory();
    test_alloc_pages();
    break;

  // Lab4
  case 4:
    pt_init();
    trapinit();
    trapinithart(); // 设置陷阱向量
    // test_timer_interrupt();
    break;

  // Lab5
  case 5:
    pt_init();
    procinit();
    test_process_creation();
    test_scheduler();
    test_synchronization();
    break;
  
  // Lab6
  case 6:
    
  default:
    uart_puts("No such lab!");
    break;
  }
}

int main() {
  int lab = 5;
  run_lab(lab);
}