K=kernel
U=user

# Kernel object files organized by subsystem
OBJS = \
	$(K)/entry.o \
	$(K)/main.o \
	$(K)/driver/console.o \
	$(K)/driver/plic.o \
	$(K)/driver/uart.o \
	$(K)/driver/virtio_disk.o \
	$(K)/lib/printf.o \
	$(K)/lib/string.o \
	$(K)/mm/kalloc.o \
	$(K)/mm/vm.o \
	$(K)/proc/proc.o \
	$(K)/proc/swtch.o \
	$(K)/sync/sleeplock.o \
	$(K)/sync/spinlock.o \
	$(K)/trap/kernelvector.o \
	$(K)/trap/syscall.o \
	$(K)/trap/sysfile.o \
	$(K)/trap/sysproc.o \
	$(K)/trap/trampoline.o \
	$(K)/trap/trap.o \
	$(K)/fs/bio.o \
	$(K)/fs/file.o \
	$(K)/fs/fs.o \
	$(K)/fs/log.o \
	$(K)/ipc/pipe.o \
	$(K)/test/lab2.o \
	$(K)/test/lab3.o \
	$(K)/test/lab4.o \
	$(K)/test/lab5.o \

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
INCLUDES = -I. -I$(K)/include
CFLAGS = $(RISCV_ARCH) -ffreestanding -fno-builtin -fno-builtin-log -fno-stack-protector -nostdlib -O2 -g -mcmodel=medany $(INCLUDES)
ASFLAGS = $(RISCV_ARCH) -ffreestanding -nostdlib -O2 -g $(INCLUDES)
LDFLAGS = -nostdlib -T $(K)/kernel.ld -Wl,--build-id=none

.PHONY: all clean run qemu fs.img

all: $(K)/kernel.elf

$(K)/kernel.elf: $(OBJS) $(K)/kernel.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Pattern rules for kernel source files in root
$(K)/%.o: $(K)/%.S
	$(CC) $(ASFLAGS) -c -o $@ $<

$(K)/%.o: $(K)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Pattern rules for kernel source files in subdirectories
$(K)/driver/%.o: $(K)/driver/%.S
	$(CC) $(ASFLAGS) -c -o $@ $<

$(K)/driver/%.o: $(K)/driver/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/lib/%.o: $(K)/lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/mm/%.o: $(K)/mm/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/sync/%.o: $(K)/sync/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/proc/%.o: $(K)/proc/%.S
	$(CC) $(ASFLAGS) -c -o $@ $<

$(K)/proc/%.o: $(K)/proc/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/trap/%.o: $(K)/trap/%.S
	$(CC) $(ASFLAGS) -c -o $@ $<

$(K)/trap/%.o: $(K)/trap/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/fs/%.o: $(K)/fs/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/ipc/%.o: $(K)/ipc/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(K)/test/%.o: $(K)/test/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# mkfs tool - compiled for host
mkfs/mkfs: mkfs/mkfs.c $(K)/fs/fs.h $(K)/include/param.h $(K)/fs/stat.h
	gcc -Wno-unknown-attributes -I. -o mkfs/mkfs mkfs/mkfs.c

# File system image
UPROGS=\

fs.img: mkfs/mkfs README.md $(UPROGS)
	mkfs/mkfs fs.img README.md $(UPROGS)

clean:
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg
	rm -f $(K)/*.o $(K)/kernel.elf fs.img mkfs/mkfs
	rm -f $(K)/driver/*.o $(K)/lib/*.o $(K)/mm/*.o $(K)/sync/*.o
	rm -f $(K)/proc/*.o $(K)/trap/*.o $(K)/fs/*.o $(K)/ipc/*.o $(K)/test/*.o

# QEMU options
QEMUOPTS = -machine virt -bios none -kernel $(K)/kernel.elf -m 128M -smp 1 -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

run: all fs.img
	$(QEMU) $(QEMUOPTS)

qemu: run

qemu-gdb: all fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::26000
