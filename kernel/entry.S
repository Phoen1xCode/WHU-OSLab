        # qemu -kernel loads the kernel at 0x80000000.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
        # debug code
        li t2, 0x10000000 # UART0
        li t1, 'S' # start message
        sb t1, 0(t2)
        # debug code

        # set up a stack for C.
        la sp, stack_top
        
        # set bss section to 0: [sbss, ebss)
        la t0, sbss
        la t1, ebss
1:
        bgeu t0, t1, 2f
        sb zero, 0(t0)
        addi t0, t0, 1
        j 1b
2:
        li t1, 'P'
        sb t1, 0(t2)

        # jump to start() in start.c
        call main
spin:
        j spin
