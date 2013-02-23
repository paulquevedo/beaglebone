/******************************************************************************
 * linkerscript.ld
 *
 * Bootloader linker script for the TI AM3559/BeagleBone
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 ******************************************************************************/

MEMORY
{
    SRAM  (rwx) : ORIGIN = 0x402F0400, LENGTH = 126K
    STACK (rwx) : ORIGIN = 0x4030F800, LENGTH = 2K
}

IRQ_STACK_SIZE = 0x20;
FIQ_STACK_SIZE = 0x8;
UND_STACK_SIZE = 0x8;
ABT_STACK_SIZE = 0x8;
SVC_STACK_SIZE = 0x8;

SECTIONS
{
    .entry :
    {
        _vector_start_addr = .;
        KEEP(*(.vector_table))
        KEEP(*(.default_handlers))
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

