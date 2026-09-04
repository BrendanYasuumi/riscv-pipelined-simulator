ENTRY(_start)

SECTIONS
{
    . = 0x00010000;

    .text : {
        *(.text*)
    }

    . = ALIGN(4);

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }
}
