/******************************************************************************
 * start.S
 *
 * Startup code for the am335x beaglebone
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *****************************************************************************/
#include "chconf.h"
#define BIT_0  0x00000001
#define BIT_1  0x00000002
#define BIT_2  0x00000004
#define BIT_3  0x00000008
#define BIT_4  0x00000010
#define BIT_5  0x00000020
#define BIT_6  0x00000040
#define BIT_7  0x00000080
#define BIT_8  0x00000100
#define BIT_9  0x00000200
#define BIT_10 0x00000400
#define BIT_12 0x00001000
#define BIT_13 0x00002000
#define BIT_14 0x00004000
#define BIT_15 0x00008000
#define BIT_16 0x00010000
#define BIT_17 0x00020000
#define BIT_18 0x00040000
#define BIT_19 0x00080000
#define BIT_20 0x00100000
#define BIT_21 0x00200000
#define BIT_22 0x00400000
#define BIT_23 0x00800000
#define BIT_24 0x01000000
#define BIT_25 0x02000000
#define BIT_26 0x04000000
#define BIT_27 0x08000000
#define BIT_28 0x10000000
#define BIT_29 0x20000000
#define BIT_30 0x40000000
#define BIT_31 0x80000000

/* CPSR Modes */
#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_ABT 0x17
#define MODE_UND 0x1B
#define MODE_SYS 0x1F

/* CPSR Control Bits */
#define I_BIT    0x80 /* IRQ Disable */
#define F_BIT    0x40 /* FIQ Disable */

                        /* Linker script defines */
    .global _exception_table_addr
    .global _stack_bottom
    .global _stack_irq_top
    .global _stack_fiq_top
    .global _stack_und_top
    .global _stack_abt_top
    .global _stack_svc_top
    .global _stack_top
    .global _bss_start
    .global _bss_end
    .global _heap_start
    .global _heap_end

    .text
    .section .exception_table,"ax",%progbits
    .code 32

_exception_table:
    b _reset_eh
    b _undefined_eh
    b _swi_eh
    b _prefetch_abort_eh
    b _data_abort_eh
    b .
    b _irq_eh
    b _fiq_eh

    .text
    .section .exception_table,"ax",%progbits
    .code 32
    .align 4

/******************************************************************************
 * _reset_eh
 *
 *      Reset exception handler. Configures the stack, ctr0 init,
 *  enables branch predicition then hands off to main()
 *
 ******************************************************************************/
_reset_eh:
    /* Initialize stacks for each mode
     * Note that cpsr_c is an ARM intrinsic for the control bits of the cpsr */
    msr cpsr_c, #(MODE_UND | I_BIT | F_BIT)
    ldr r0, =_stack_und_top
    mov sp, r0

    msr cpsr_c, #(MODE_ABT | I_BIT | F_BIT)
    ldr r0, =_stack_abt_top
    mov sp, r0

    msr cpsr_c, #(MODE_FIQ | I_BIT | F_BIT)
    ldr r0, =_stack_fiq_top
    mov sp, r0

    msr cpsr_c, #(MODE_IRQ | I_BIT | F_BIT)
    ldr r0, =_stack_irq_top
    mov sp, r0

    msr cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
    ldr r0, =_stack_svc_top
    mov sp, r0

    msr cpsr_c, #(MODE_SYS | I_BIT | F_BIT)
    ldr r0, =_stack_top
    mov sp, r0

    /* Watermark the stacks */
    ldr r0, =_stack_bottom
    ldr r1, =_stack_top
    ldr r2, =0x5a5a5a5a
    /* Descending stack, fill from bottom up */
stack_loop:
    str r2, [r0], #4
    cmp r0, r1
    blt stack_loop

    /* Watermark the heap */
    ldr r0, =_heap_start
    ldr r1, =_heap_end
    ldr r2, =0xefefefef
heap_loop:
    str r2, [r0], #4
    cmp r0, r1
    blt heap_loop

    /* Zero out .bss section */
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
bss_loop:
    str r2, [r0], #4
    cmp r0, r1
    blt bss_loop

call_main:
    ldr r10,=main   /* Addres of main() */
    mov lr,pc       /* Dummy return */
    bx  r10         /* Bye-bye */

dead_loop:
    b dead_loop


/******************************************************************************
 * _irq_eh
 *
 *      IRQ exception handler. Doesn't support nesting with ChibiOS... yet
 *
 *  Refer to ARM DEN0013C s12
 *
 ******************************************************************************/
#define INTC_SIR_IRQ   0x48200040
#define MASK_SIR_IRQ   0x7f
#define INTC_CONTROL   0x48200048
#define MASK_NEWIRQAGR 0x1
#define INTC_PRIORITY  0x48200060
#define MASK_PRIORITY  0x7f
#define INTC_THRESHOLD 0x48200068
#define MASK_THRESHOLD 0xff
    .global isrVectorTable
    .align 4
_irq_eh:
    stmfd sp!, {r0-r3,r12,lr} /* Save context */

#if defined(CH_DBG_SYSTEM_STATE_CHECK)
    bl   dbg_check_enter_isr
#endif
                              /* Change to MODE_SYS, no IRQs enabled.
                                 Executing ISRs in this mode allows use
                                 of greater stack */
    msr   cpsr_c, #(MODE_SYS | I_BIT)

    stmfd sp!, {lr}           /* Save MODE_SYS lr */

    ldr   r1, =INTC_SIR_IRQ   /* Grab current IRQ number */
    ldr   r2, [r1]
    and   r2, r2, #MASK_SIR_IRQ

    ldr   r0, =isrVectorTable  /* Load vector table address */
    ldr   r1, [r0, r2, lsl #2] /* Load address of the ISR */
    blx   r1                   /* Jump to isr in ARM Mode */

    ldmfd sp!, {lr}            /* Restore MODE_SYS lr */

    msr   cpsr_c, #(MODE_IRQ | I_BIT)

    mov r0, #MASK_NEWIRQAGR    /* Enable new IRQs */
    ldr r1, =INTC_CONTROL
    str r0, [r1]
    dsb

#if defined(CH_DBG_SYSTEM_STATE_CHECK)
    bl   dbg_check_leave_isr
#endif

    b _port_irq_common

/******************************************************************************
 * Data Abort EH
 *
 *      Read out fault information into registers
 *
 * r0: Address of the offending data address
 * r1: Fault Type
 * r2: Domain of the address
 *
 * Fault Types defined in s3.2.35 of DDI0344D: DFSR Register
 *
 ******************************************************************************/
    .align 4
_data_abort_eh:
    mrc p15, #0, r0, c6, c0, #0     /* Read the data fault address register */
    mrc p15, #0, r3, c5, c0, #0     /* Read the data fault status register */

    and r1, r3, #0xf
    tst r3, #BIT_10                 /* Bitwise AND, updates the Z flag */
    orrne r1, r1, #BIT_4            /* OR if zero flag not set */
    tst r3, #BIT_12
    orrne r1, r1, #BIT_5            /* Fault Type stored in r1 */

    lsr r2, r3, #4
    and r2, r2, #0xf                /* Domain stored in r2 */

_data_abort_dead_loop:
    b   _data_abort_dead_loop

/******************************************************************************
 * unhandled exceptions
 *
 *      Loop forever
 *
 ******************************************************************************/
    .align 4
_undefined_eh:
    b _undefined_eh

    .align 4
_swi_eh:
    b _swi_eh

    .align 4
_prefetch_abort_eh:
    b _prefetch_abort_eh

    .align 4
_fiq_eh:
    b   _fiq_eh     /* Not supported on this chip */

    .end


