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
#define IER 1 // interrupt enable register
#define IER_RX_ENABLE (1 << 0)
#define IER_TX_ENABLE (1 << 1)
#define FCR 2 // FIFO control register
#define ISR 2 // interrupt status register
#define LCR 3 // line control register
#define LCR_EIGHT_BITS (3 << 0)
#define LCR_BAUD_LATCH (1 << 7) // special mode to set baud rate
#define LSR 5                   // line status register
#define LSR_RX_READY (1 << 0)   // input is waitting to be read from RHR
#define LSR_TX_IDLE (1 << 5)    // THR can accept another character to send

void uartinit(void) {
  // disable interrupts.
  WriteReg(IER, 0x00);

  //
}

void uartwrite(char buf[], int n);

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

// read one input character from the UART.
// return -1 if none is watting
int uartgetc(void) {
  if (ReadReg(LSR) & LSR_RX_READY) {
    return ReadReg(RHR);
  } else {
    return -1;
  }
}

void uartintr(void);