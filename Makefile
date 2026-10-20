CC   = gcc
NASM = nasm
LD   = ld
QEMU = qemu-system-i386

BUILD  = build
KERNEL = $(BUILD)/leoos.bin
ISO    = $(BUILD)/leoos.iso

# Freestanding 32-bit code: no libc, no stack protector (it needs libc support),
# no position independent code, and no FPU/SSE registers in kernel code.
CFLAGS = -m32 -std=gnu99 -O2 -g -ffreestanding -fno-stack-protector -fno-pie \
         -fno-asynchronous-unwind-tables -fno-tree-loop-distribute-patterns \
         -mgeneral-regs-only -Wall -Wextra -Iinclude -MMD -MP
NASMFLAGS = -f elf32 -g
LDFLAGS   = -m elf_i386 -T linker.ld -nostdlib -z noexecstack

C_SRCS   = $(wildcard src/*.c)
ASM_SRCS = $(wildcard src/*.s)
OBJS     = $(C_SRCS:src/%.c=$(BUILD)/%.c.o) $(ASM_SRCS:src/%.s=$(BUILD)/%.s.o)

QEMU_FLAGS = -kernel $(KERNEL) -serial stdio

.PHONY: all run run-nographic debug iso run-iso clean

all: $(KERNEL)

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD)/%.c.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.s.o: src/%.s | $(BUILD)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD):
	mkdir -p $@

# Boot in a QEMU window, with the serial console mirrored to the terminal
run: $(KERNEL)
	$(QEMU) $(QEMU_FLAGS)

# Terminal only: the shell runs over the serial port (quit with Ctrl+A then X)
run-nographic: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -nographic

# Wait for GDB on port 1234: gdb $(KERNEL) -ex 'target remote :1234'
debug: $(KERNEL)
	$(QEMU) $(QEMU_FLAGS) -s -S

# Bootable GRUB image (needs grub-mkrescue and xorriso), e.g. for a USB stick
iso: $(ISO)

$(ISO): $(KERNEL)
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/leoos.bin
	printf 'set timeout=0\nmenuentry "LeoOS" {\n  multiboot /boot/leoos.bin\n}\n' \
		> $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso

run-iso: $(ISO)
	$(QEMU) -cdrom $(ISO) -serial stdio

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
