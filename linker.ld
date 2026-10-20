ENTRY(_start)

SECTIONS
{
    /* The kernel is loaded at 1 MiB, the conventional place for Multiboot kernels */
    . = 1M;
    kernel_start = .;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot) /* must sit within the first 8 KiB of the file */
        *(.text .text.*)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata .rodata.*)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data .data.*)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note .note.*)
    }
}
