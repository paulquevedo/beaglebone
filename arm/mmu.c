/******************************************************************************
 *
 * arm/mmu.c
 *
 * Functions to setup the ARMv7 MMU
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
#include "mmu.h"

#define PAGE_TYPE_FAULT   ((0x0 << 18) | (0x0))
#define PAGE_TYPE_L2      ((0x0 << 18) | (0x1))
#define PAGE_TYPE_L1_1MB  ((0x0 << 18) | (0x2))
#define PAGE_TYPE_L1_16MB ((0x1 << 18) | (0x2))

/*
 * ARM Doc DEN0013C, s10.7.3
 * Domains are deprecated as of ARMv7. Still necessary to assign a
 * dummy domain and enable client permissions though.
 */
#define DOMAIN_DEPRECATED  (0x1 << 5)

static uint32_t __attribute__ ((aligned(16*1024))) ttb0[4096];

/* TODO: Support 16MB regions to reduce TLB entries */
void _mmu_add_region(mmuRegion_t *region)
{
    uint32_t idx   = region->virtAddr >> 20; /* 1MB Pages */
    uint32_t entry = (region->physAddr & 0xfff00000)
                   |  region->attributes
                   |  region->permissions
                   |  DOMAIN_DEPRECATED
                   |  PAGE_TYPE_L1_1MB;
    int i;

    for (i = 0; i < region->pages; i++)
        ttb0[idx++] = entry + (i << 20); /* 1MB Pages */
}

void _mmu_enable(void)
{
    /* Enable client access for all domains */
    asm volatile ("ldr r0,=0x55555555\n\t"
                  "mcr p15,0,r0,c3,c0,0\n\t");
    /* Set TTB0 address */
    asm volatile ("mcr p15,0,%[in],c2,c0,0" : : [in] "r"(ttb0));
    /* Set TTB1 address */
    asm volatile ("mcr p15,0,%[in],c2,c0,1" : : [in] "r"(ttb0));
    /* Flush both instruction and data TLB */
    asm volatile ("mcr p15,0,r0,c8,c7,0");
    /* Enable the MMU */
    asm volatile ("mrc p15,0,r0,c1,c0,0\n\t"
                  "orr r0,r0,#0x01     \n\t"
                  "mcr p15,0,r0,c1,c0,0\n\t");
    asm volatile ("isb");
    asm volatile ("dsb");
}

void _mmu_disable(void)
{
    asm volatile ("mrc p15,0,r0,c1,c0,0\n\t"
                  "bic r0,r0,#0x01     \n\t"
                  "mcr p15,0,r0,c1,c0,0\n\t");
    asm volatile ("dsb");
}
