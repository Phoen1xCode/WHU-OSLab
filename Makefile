K=kernel
U=user

OBJS = \
	$(K)/entry.o \
	$(K)/start.o \
	$(K)/uart.o \
	$(K)/console.o \
	$(K)/printf.o \
	$(K)/string.o \
	$(K)/kalloc.o \
	$(K)/kalloc_test.o \
	$(K)/vm.o \
	$(K)/vm_test.o

# riscv64-unknown-elf- or riscv64-linux-gnu-
# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)gcc
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

QEMU = qemu-system-riscv64
MIN_QEMU_VERSION = 7.2


# Build flags
RISCV_ARCH = -march=rv64gc -mabi=lp64
CFLAGS = $(RISCV_ARCH) -ffreestanding -fno-builtin -fno-stack-protector -nostdlib -O2 -g -mcmodel=medany -I$(K)
ASFLAGS = $(RISCV_ARCH) -ffreestanding -nostdlib -O2 -g -I$(K)
LDFLAGS = -nostdlib -T $(K)/kernel.ld -Wl,--build-id=none

.PHONY: all clean run

all: $(K)/kernel.elf

$(K)/kernel.elf: $(OBJS) $(K)/kernel.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(K)/entry.o: $(K)/entry.S
	$(CC) $(ASFLAGS) -c -o $@ $<

$(K)/start.o: $(K)/start.c $(K)/types.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/uart.o: $(K)/uart.c $(K)/memlayout.h $(K)/types.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/console.o: $(K)/console.c $(K)/defs.h $(K)/types.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/printf.o: $(K)/printf.c $(K)/defs.h $(K)/types.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/string.o: $(K)/string.c $(K)/defs.h $(K)/types.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/kalloc.o: $(K)/kalloc.c $(K)/defs.h $(K)/types.h $(K)/memlayout.h $(K)/riscv.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/kalloc_test.o: $(K)/kalloc_test.c $(K)/defs.h $(K)/types.h $(K)/kalloc.h $(K)/riscv.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/vm.o: $(K)/vm.c $(K)/defs.h $(K)/types.h $(K)/memlayout.h $(K)/riscv.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/vm_test.o: $(K)/vm_test.c $(K)/defs.h $(K)/types.h $(K)/memlayout.h $(K)/riscv.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(K)/*.o $(K)/kernel.elf

run: all
	$(QEMU) -machine virt -bios none -nographic -kernel $(K)/kernel.elf
