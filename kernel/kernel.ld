OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
    /*
     * ensure that entry.S / _entry is at 0x80000000,
     * where qemu's -kernel jumps.
     */
    . = 0x80000000;
    
    /*
     * define the .text section 
     * where holds the program's executable machine code
     */
    .text : {
        /* 
         * First, put _entry() function of kernel/entry.o file here
         * entry.o is the file generated after compiling entry.S
         */
        kernel/entry.o(_entry)

        /* Then put all the code of other files (.text section) here */
        *(.text .text.*)

        /* Align the current position to the 4096 boundary */
        . = ALIGN(0x1000);

        PROVIDE(etext = .);        
    }

    .rodata : {
        . = ALIGN(16);
        *(.rodata .rodata.*)
    }

    .data : {
        PROVIDE(sdata = .);
        . = ALIGN(16);
        *(.sdata .sdata.*)
        . = ALIGN(16);
        *(.data .data.*)
        PROVIDE(edata = .);
    }

    .bss (NOLOAD) : {
        PROVIDE(sbss = .);
        . = ALIGN(16);
        *(.sbss .sbss.* .bss .bss.* COMMON) 
        PROVIDE(ebss = .);
    }

    /* NOLOAD section for la sp, stack_top */
    . = ALIGN(16);
    __stack_size = 0x4000;
    .stack (NOLOAD) : {
        PROVIDE(stack_bottom = .);
        . = . + __stack_size;
        PROVIDE(stack_top = .);
    }

    PROVIDE(end = .);
} 
