/******************************************************************************
 * linkerscript.ld
 *
 * Bootloader linker script for the TI AM3559/BeagleBone
 *
 * Paul Quevedo 2012
 ******************************************************************************/

MEMORY
{
    SRAM  (rwx) : ORIGIN = 0x80000000, LENGTH = 1M
    STACK (rwx) : ORIGIN = 0x80100000, LENGTH = 8K
}

IRQ_STACK_SIZE = 0x400;
FIQ_STACK_SIZE = 0x8;
UND_STACK_SIZE = 0x8;
ABT_STACK_SIZE = 0x8;
SVC_STACK_SIZE = 0x8;

SECTIONS
{
    .entry :
    {
        _exception_table_addr = .;
        KEEP(*(.exception_table))
        KEEP(*(.exception_handlers))
    } > SRAM

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
    } > SRAM

    .data :
    {
        . = ALIGN(4);
        *(.data*)
    }  > SRAM

    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss*)
        . = ALIGN(4);
        _bss_end = .;
        _heap_start = .;
    } > SRAM

    /*
     * The rest of the SRAM is used for the heap and stacks.
     * The stack area grows down therefore is starts at the end/top of SRAM.
     * The heap grows up and starts at the end of the bss section and
     * ends at the bottom of the stacks.
     */

    .stack :
    {
        _heap_end = .;
        _stack_bottom = .;

        . += IRQ_STACK_SIZE;
        . = ALIGN(4);
        _stack_irq_top = .;

        . += FIQ_STACK_SIZE;
        . = ALIGN(4);
        _stack_fiq_top = .;

        . += UND_STACK_SIZE;
        . = ALIGN(4);
        _stack_und_top = .;

        . += ABT_STACK_SIZE;
        . = ALIGN(4);
        _stack_abt_top = .;

        . += SVC_STACK_SIZE;
        . = ALIGN(4);
        _stack_svc_top = .;

    } > STACK

    _stack_top = ALIGN (ORIGIN(STACK) + LENGTH(STACK), 8);
}

