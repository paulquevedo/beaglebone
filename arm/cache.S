/******************************************************************************
 *
 * arm/cache.S
 *
 * Functions to control the ARMv7 cache
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *****************************************************************************/
.text
.code 32

.global _dcache_enable
.global _dcache_disable
.global _dcache_flush

.global _icache_enable
.global _icache_disable
.global _icache_flush

.global _branch_predict_enable
.global _branch_predict_disable
.global _branch_predict_invalidate

/******************************************************************************
 *
 * _dcache_enable
 *
 * Enables the L2 data cache for Cortex A8 Processors
 *          ARM DDI0344D s8.3 Enabling the L2 Cache Controller
 * Enables the L1 data cache in the system control register [SCTLR]
 *          DDI0406C sB2.2.3 Cache enabling and disabling
 *
 *****************************************************************************/
 _dcache_enable:
    mrc p15, #0, r0, c0, c0, #0     /* Read Main ID Register */
    lsr r0, r0, #4
    ldr r1,=0xfff
    and r0, r0, r1                  /* Get the Primary Part Number */
    ldr r1,=0xc08                   /* Part number of the Cortex-A8 */
    cmp r0, r1
    bne _dcache_enable_L1           /* Just enable L1 cache */
    mrc p15, #0, r0, c1, c0, #1     /* Read the Aux Ctrl register */
    orr r0, r0, #0x2                /* Set the L2EN bit */
    mcr p15, #0, r0, c1, c0, #1

_dcache_enable_L1:
    mrc p15, #0, r0, c1, c0, #0
    orr r0, r0, #0x4                /* C_BIT is BIT_2 */
    mcr p15, #0, r0, c1, c0, #0
    bx lr

/******************************************************************************
 *
 * _dcache_disable
 *
 * Disables the data cache in the system control register [SCTLR]
 *          DDI0406C sB2.2.3 Cache enabling and disabling
 *
 *****************************************************************************/
 _dcache_disable:
    mrc p15, #0, r0, c1, c0, #0
    bic r0, r0, #0x4                /* C_BIT is BIT_2 */
    mcr p15, #0, r0, c1, c0, #0
    dsb
    bx lr

/******************************************************************************
 *
 * _dcache_flush
 *
 * Flushes the entire data cache to the point of coherency
 *          DDI0406C sB2.2.7 Performing cache maintenance operations
 *
 *****************************************************************************/
_dcache_flush:
    push {r4-r11}
    dmb
    mrc  p15, #1, r0, c0, c0, #1    /* Read CLIDR into R0 */
    ands r3, r0, #0x07000000
    mov  r3, r3, lsr #23            /* Cache level value (naturally aligned) */
    beq  _dcache_flush_finished
    mov  r10, #0

_dcache_flush_loop1:
    add  r2, r10, r10, lsr #1       /* Work out 3 x cachelevel */
    mov  r1, r0, lsr r2             /* 3 lsb are cache type for this level */
    and  r1, r1, #7                 /* grab the 3 lsb */
    cmp  r1, #2
    blt  _dcache_flush_skip         /* no cache or only icache at this level */
    mcr  p15, #2, r10, c0, c0, #0   /* write CSSELR from r10 */
    isb                             /* ISB to sync the change to the CCSIDR */
    mrc  p15, #1, r1, c0, c0, #0    /* read current CSSIDR to R1 */
    and  r2, r1, #7                 /* extract the line length field */
    add  r2, r2, #4                 /* add 4, line length offset */
    ldr  r4, =0x3ff
    ands r4, r4, r1, lsr #3         /* R4 is max num of the way size */
    clz  r5, r4                     /* R5 is bit posn of the way size incr */
    mov  r9, r4                     /* R9 working copy of the max way size */

_dcache_flush_loop2:
    ldr  r7, =0x7fff
    ands r7, r7, r1, lsr #13        /* R7 is the max num of the index size */

_dcache_flush_loop3:
    orr  r11, r10, r9, lsl r5       /* factor in the way & cache num into R11 */
    orr  r11, r11, r7, lsl r2       /* factor in the index number */
    mcr  p15, #0, r11, c7, c10, #2  /* DCCSW. clean by set/way */
    subs r7, r7, #1                 /* decrement the index */
    bge  _dcache_flush_loop3
    subs r9, r9, #1                 /* decrement the way number */
    bge  _dcache_flush_loop2

_dcache_flush_skip:
    add r10, r10, #2                /* increment the cache number */
    cmp r3, r10
    bgt _dcache_flush_loop1

_dcache_flush_finished:
    dsb
    isb
    pop {r4-r11}
    bx  lr

/******************************************************************************
 *
 * _icache_enable
 *
 * Enables the instruction cache in the system control register [SCTLR]
 *          DDI0406C sB2.2.3 Cache enabling and disabling
 *
 *****************************************************************************/
 _icache_enable:
    mrc p15, #0, r0, c1, c0, #0
    orr r0, r0, #0x1000             /* I_BIT is BIT_12 */
    mcr p15, #0, r0, c1, c0, #0
    bx lr

/******************************************************************************
 *
 * _icache_disable
 *
 * Disables the instruction cache in the system control register [SCTLR]
 *          DDI0406C sB2.2.3 Cache enabling and disabling
 *
 *****************************************************************************/
 _icache_disable:
    mrc p15, #0, r0, c1, c0, #0
    bic r0, r0, #0x1000             /* I_BIT is BIT_12 */
    mcr p15, #0, r0, c1, c0, #0
    isb
    bx lr

/******************************************************************************
 *
 * _icache_flush
 *
 * Flushes the entire instruction cache to the point of unification
 *          DDI0406C sB3.18.6 Cache maintenance operations, [ICIALLU]
 *
 *****************************************************************************/
_icache_flush:
    mcr p15, #0, r0, c7, c5, #0
    isb
    bx lr


/******************************************************************************
 *
 * _branch_predict_enable
 *
 * Enables the branch predictor in the system control register [SCTLR]
 *          DDI0406C sB2.2.3 Cache enabling and disabling
 *
 *****************************************************************************/
 _branch_predict_enable:
    mrc p15, #0, r0, c1, c0, #0
    orr r0, r0, #0x800              /* Z_BIT is BIT_11 */
    mcr p15, #0, r0, c1, c0, #0
    bx lr

/******************************************************************************
 *
 * _branch_predict_disable
 *
 * Disables the branch predictor in the system control register [SCTLR]
 *          DDI0406C sB2.2.3 Cache enabling and disabling
 *
 *****************************************************************************/
 _branch_predict_disable:
    mrc p15, #0, r0, c1, c0, #0
    bic r0, r0, #0x800              /* Z_BIT is BIT_11 */
    mcr p15, #0, r0, c1, c0, #0
    isb
    bx lr

/******************************************************************************
 *
 * _branch_predict_invalidate
 *
 * Invalidates the branch predictor
 *          DDI0406C sB3.18.6 Cache maintenance operations, [BPIALL]
 *
 *****************************************************************************/
_branch_predict_invalidate:
    mcr p15, #0, r0, c7, c5, #6
    isb
    bx lr

