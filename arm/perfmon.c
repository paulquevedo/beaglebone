/******************************************************************************
 *
 * arm/perfmon.c
 *
 * Functions to utilize the ARMv7 Perfomance monitor module
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *****************************************************************************/

#include <stdint.h>
#include "perfmon.h"

static uint32_t numCnt;

int _perfmon_add(uint32_t counterSel, uint32_t event)
{
    if (numCnt == 0) {
        asm volatile("mrc p15,0,%[out],c9,c12,0" : [out] "=r"(numCnt) : );
        numCnt = (numCnt >> 11) & 0x1f;
    }

    if (counterSel == PERF_MON_CYCLE_COUNTER)
        return 0;

    if (counterSel >= numCnt)
        return -1;

    /* Set counter [counterSel] to track [event] */
    asm volatile("mcr p15, 0, %[in], c9, c12, 5" : : [in] "r"(counterSel));
    asm volatile("mcr p15, 0, %[in], c9, c13, 1" : : [in] "r"(event));

    /* Enable counter */
    counterSel = 1 << counterSel;
    asm volatile("mcr p15, 0, %[in], c9, c12, 1" : : [in] "r"(counterSel));

    return 0;
}

uint32_t _perfmon_get(uint32_t counterSel)
{
    volatile uint32_t reg;

    if (counterSel == PERF_MON_CYCLE_COUNTER) {
        /* Check for overflow */
        asm volatile("mrc p15, 0, %[out],c9, c12, 3" : [out] "=r"(reg) : );
        if (reg & (1 << 31))
            return 0xffffffff;

        /* Read Cycle Count Register */
        asm volatile("mrc p15, 0, %[out],c9, c13, 0" : [out] "=r"(reg) : );
        return reg;
    }

    if (counterSel >= numCnt)
        return 0;

    /* Check for counter overflow */
    asm volatile("mrc p15, 0, %[out],c9, c12, 3" : [out] "=r"(reg) : );
    if (reg & (1 << counterSel))
        return 0xffffffff;

    /* Select Counter */
    asm volatile("mcr p15, 0, %[in], c9, c12, 5" : : [in] "r"(counterSel));
    /* Read event counter */
    asm volatile("mrc p15, 0, %[out],c9, c13, 2" : [out] "=r"(reg) : );

    return reg;
}

int _perfmon_reset(uint32_t counterSel)
{
    volatile uint32_t reg = 0;

    if (counterSel == PERF_MON_CYCLE_COUNTER) {
        asm volatile("mcr p15, 0, %[in],c9, c13, 0" : : [in] "r"(reg));
    }
    else if (counterSel <= numCnt) {
        asm volatile("mcr p15, 0, %[in],c9, c12, 5" : : [in] "r"(counterSel));
        asm volatile("mcr p15, 0, %[in],c9, c13, 2" : : [in] "r"(reg));
    } else {
        return -1;
    }
    return 0;
}

int _perfmon_enable(void)
{
    volatile uint32_t reg;

    if (numCnt == 0)
        return -1;

    /* Enable the cpu cycle counter */
    reg = 0x80000000;
    asm volatile("mcr p15, 0, %[in], c9, c12, 1" : : [in] "r"(reg));

    asm volatile("mrc p15, 0, %[out], c9, c12, 0" : [out] "=r"(reg) : );
    reg |= 0x7; /* Reset Cycle Counter,
                   Reset all performance counters,
                   Enable all counters */
    asm volatile("mcr p15, 0, %[in], c9, c12, 0" : : [in] "r"(reg) );

    return 0;
}

int _perfmon_disable(void)
{
    volatile uint32_t reg;
    asm volatile("mrc p15, 0, %[out], c9, c12, 0" : [out] "=r"(reg) : );
    reg &= ~0x1;
    asm volatile("mcr p15, 0, %[in], c9, c12, 0" : : [in] "r"(reg) );

    return 0;
}
