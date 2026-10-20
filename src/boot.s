; Multiboot header constants
MBALIGN  equ  1 << 0            ; align loaded modules on 4 KiB page boundaries
MEMINFO  equ  1 << 1            ; ask the bootloader for a memory map
FLAGS    equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002        ; lets the bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)   ; MAGIC + FLAGS + CHECKSUM must equal 0

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384                  ; 16 KiB kernel stack
stack_top:

section .text
global _start:function (_start.end - _start)
extern kernel_main
_start:
    ; The bootloader leaves us in 32-bit protected mode with interrupts and
    ; paging disabled, but without a stack.
    mov esp, stack_top

    ; Start from a known EFLAGS state
    push 0
    popf

    ; kernel_main(uint32_t magic, struct multiboot_info *mbi)
    ; Keep the stack 16-byte aligned at the call, as the System V ABI requires.
    sub esp, 8
    push ebx                    ; physical address of the multiboot info structure
    push eax                    ; multiboot magic number
    call kernel_main

    ; kernel_main should never return: if it does, halt the CPU forever.
    cli
.hang:
    hlt
    jmp .hang
.end:
