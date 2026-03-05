CC = gcc
NASM = nasm
LD = ld
QEMU = qemu-system-i386

CFLAGS = -m32 -std=gnu99 -ffreestanding -Wall -Wextra
NASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o kernel.o

all: myos.bin

myos.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o myos.bin $(OBJS)

boot.o: boot.s
	$(NASM) $(NASMFLAGS) -o boot.o boot.s

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

run: myos.bin
	$(QEMU) -kernel myos.bin

clean:
	rm -f *.o myos.bin
