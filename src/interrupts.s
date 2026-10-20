; Interrupt entry points for vectors 0-47 (CPU exceptions and remapped IRQs).
;
; The CPU pushes an error code for some exceptions only: the other stubs push a
; dummy 0 so that every interrupt reaches isr_common with the same stack layout,
; which C sees as a `struct registers`.

extern isr_handler

section .text

; void idt_load(const struct idt_ptr *ptr)
global idt_load
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

%assign i 0
%rep 48
isr_stub_%[i]:
  %if !(i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30)
    push dword 0                ; dummy error code
  %endif
    push dword i                ; vector number
    jmp isr_common
%assign i i + 1
%endrep

isr_common:
    pusha
    xor eax, eax                ; EAX is saved by pusha: clear it so the high
    mov ax, ds                  ; half of the pushed DS is zero
    push eax                    ; save the data segment

    mov ax, 0x10                ; switch to the kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    cld                         ; the C ABI expects the direction flag clear
    push esp                    ; struct registers *
    call isr_handler
    add esp, 4

    pop eax                     ; restore the original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8                  ; drop the vector number and error code
    iret

section .data

; Table of stub addresses, used by idt.c to fill the IDT.
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 48
    dd isr_stub_%[i]
%assign i i + 1
%endrep
