
MBALIGN  equ  1 << 0            ; 4kb
MEMINFO  equ  1 << 1            ; carte mémoire
FLAGS    equ  MBALIGN | MEMINFO ; les drapeaux
MAGIC    equ  0x1BADB002        ; le nombre magique que cherche le bootloader
CHECKSUM equ -(MAGIC + FLAGS)   ; checksum requis

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384 ; 16 KiB de pile
stack_top:

section .text
global _start:function (_start.end - _start)
_start:

    mov esp, stack_top


    extern kernel_main
    call kernel_main


    cli
.hang:
    hlt
    jmp .hang
.end:
