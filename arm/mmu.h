/******************************************************************************
 *
 * arm/mmu.h
 *
 * ARM Doc DEN0013C s10.2
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *****************************************************************************/
#ifndef __ARM_MMU_H__
#define __ARM_MMU_H__

#define MMU_ATTR_DEVICE_SHARED  ((0x0 << 12) | (0x1 << 2)) /* TEX:000 C:0 B:1 */
#define MMU_ATTR_DEVICE_NOSHARE ((0x2 << 12) | (0x0 << 2)) /* TEX:010 C:0 B:0 */
#define MMU_ATTR_STRONG_ORDER   ((0x0 << 12) | (0x0 << 2)) /* TEX:000 C:0 B:0 */
#define MMU_ATTR_INNER_WB_WA    ((0x4 << 12) | (0x1 << 2)) /* TEX:100 C:0 B:1 */
#define MMU_ATTR_INNER_WB_NOWA  ((0x4 << 12) | (0x3 << 2)) /* TEX:100 C:1 B:1 */
#define MMU_ATTR_INNER_WT_NOWA  ((0x4 << 12) | (0x2 << 2)) /* TEX:100 C:1 B:0 */
#define MMU_ATTR_OUTER_WB_WA    ((0x5 << 12) | (0x0 << 2)) /* TEX:101 C:0 B:0 */
#define MMU_ATTR_OUTER_WB_NOWA  ((0x7 << 12) | (0x0 << 2)) /* TEX:111 C:0 B:0 */
#define MMU_ATTR_OUTER_WT_NOWA  ((0x6 << 12) | (0x0 << 2)) /* TEX:110 C:0 B:0 */
#define MMU_ATTR_EXECUTE_NEVER  ((0x1 <<  4))              /* XN:1            */

#define MMU_PERM_NO_ACCESS      ((0x0 << 15) | (0x0 << 10)) /* APX:0 AP:00 */
#define MMU_PERM_SYS_RO_USR_NA  ((0x1 << 15) | (0x1 << 10)) /* APX:1 AP:01 */
#define MMU_PERM_SYS_RO_USR_RO  ((0x1 << 15) | (0x2 << 10)) /* APX:1 AP:10 */
#define MMU_PERM_SYS_RW_USR_NA  ((0x0 << 15) | (0x1 << 10)) /* APX:0 AP:01 */
#define MMU_PERM_SYS_RW_USR_RO  ((0x0 << 15) | (0x2 << 10)) /* APX:0 AP:10 */
#define MMU_PERM_SYS_RW_USR_RW  ((0x0 << 15) | (0x3 << 10)) /* APX:0 AP:11 */

typedef struct {
    uint32_t physAddr;
    uint32_t virtAddr;
    uint32_t pages; /* 1MB per page */
    uint32_t attributes;
    uint32_t permissions;
} mmuRegion_t;

extern void _mmu_enable(void);
extern void _mmu_disable(void);
extern void _mmu_add_region(mmuRegion_t *region);
#endif
