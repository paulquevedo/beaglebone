/******************************************************************************
 *
 * arm/asm.h
 *
 * Common ARM asm instructions
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *****************************************************************************/

#ifndef __ARM_ASM_H__
#define __ARM_ASM_H__

#define _dsb()                                                              \
{                                                                           \
    asm volatile("dsb");                                                    \
}
#define _isb()                                                              \
{                                                                           \
    asm volatile("isb");                                                    \
}

#define _swap16(x)                                                          \
{                                                                           \
    asm volatile ("rev16 %[out], %[in]" : [out] "=r" (x) : [in]   "r" (x)); \
}
#define _swap32(x)                                                          \
{                                                                           \
    asm volatile ("rev %[out], %[in]" : [out] "=r" (x) : [in]   "r" (x));   \
}

#define _irq_enable()                                                       \
{                                                                           \
    asm volatile ("cpsie i")                                                \
}
#define _irq_disable()                                                      \
{                                                                           \
    asm volatile ("cpsid i");                                               \
}
#define _irq_set_addr(addr)                                                 \
{                                                                           \
    asm volatile ("mcr p15, #0, %[in], c12, c0, #0" : : [in] "r" (addr));   \
    asm volatile ("dsb");                                                   \
}


extern void _dcache_enable(void);
extern void _dcache_disable(void);
extern void _dcache_flush(void);

extern void _icache_enable(void);
extern void _icache_disable(void);
extern void _icache_flush(void);

extern void _branch_predict_enable(void);
extern void _branch_predict_disable(void);
extern void _branch_predict_invalidate(void);
#endif
