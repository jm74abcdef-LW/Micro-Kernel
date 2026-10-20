; void gdt_flush(const struct gdt_ptr *ptr)
; Loads the new GDT and reloads every segment register with it.
global gdt_flush
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10                ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.reload_cs         ; a far jump is the only way to reload CS
.reload_cs:
    ret
