//
// minimal uart driver for 16550a UART.
//

#include "defs.h"
#include "memlayout.h"
#include "types.h"

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
// #define UART0 0x10000000L
#define Reg(reg) ((volatile unsigned char *)(UART0 + (reg)))
#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

#define RHR 0 // receive holding register (for input bytes)
#define THR 0 // transmit holding register (for output bytes)

#define LSR 5                 // line status register
#define LSR_RX_READY (1 << 0) // input is waitting to be read from RHR
#define LSR_TX_IDLE (1 << 5)  // THR can accept another character to send

void uartputc(char c) {
  // wait until THR is ready to send
  while ((ReadReg(LSR) & LSR_TX_IDLE) == 0)
    ; // busy wait until THR is ready to send

  // write the character to the THR register
  WriteReg(THR, c);
}

void uartputs(const char *s) {
  while (*s != '\0') {
    uartputc(*s);
    s++;
  }
}