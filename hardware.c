/*******************************************************************************
 *
 * hardware.c
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "arm/asm.h"
#include "arm/mmu.h"
#include "arm/perfmon.h"

#include "ch.h"

#include "globalDefs.h"
#include "am335x.h"
#include "hardware.h"

/****************************
 * Interrupt Controller
 ****************************/
void __attribute__ ((section (".bss"))) (*isrVectorTable[NUM_IRQS])(void);

void hwClearIRQ(uint32_t irqNum)
{
    INTC_ISR_CLEAR(irqNum / 32) = 1 << (irqNum & 0x1f);
}

void hwInstallIRQ(uint32_t irqNum, void (*isrPtr)(void), int priority)
{
    uint32_t irqBank = irqNum / 32;
    uint32_t irqBit  = 1 << (irqNum & 0x1f);

    if (irqNum >= NUM_IRQS || priority < INT_PRIORITY_MAX)
        return;

    INTC_MIR_SET(irqBank)   = irqBit; /* Disable IRQ */
    INTC_ILR(irqNum) = INTC_ILR_PRIORITY(priority);
    isrVectorTable[irqNum]  = isrPtr;
    INTC_ISR_CLEAR(irqBank) = irqBit;
    INTC_MIR_CLEAR(irqBank) = irqBit; /* Enable IRQ */
}

/****************************
 * System Tick
 ****************************/
static void systickISR(void)
{
    SYSTICK_TISR |= SYSTICK_TISR_OVF_IT_FLAG;
    hwClearIRQ(IRQ_TINT1_1MS);

    chSysLockFromIsr();
    chSysTimerHandlerI();
    chSysUnlockFromIsr();
}

static void systickInit(void)
{
   /* Interface  clock is CORE_CLKOUTM4 / 2 : PD_WKUP_L4_WKUP_GCLK
    * Functional clock is PER_CLKOUTM2 / 5859.375 : PD_WKUP_TIMER1_GLCK
    *                  -> CLK_32KHz                                     */
    CM_MODULEMODE_ENABLE (CM_WKUP_TIMER1_CLKCTRL);
    CM_MODULE_IDLEST_FUNC(CM_WKUP_TIMER1_CLKCTRL);

    CM_CLKSEL_TIMER1MS_CLK = 0x1;  /* CLK_32KHZ */


    /* Soft-reset */
    SYSTICK_TIOCP_CFG = SYSTICK_TIOCP_SOFTRESET;
    while (!(SYSTICK_TISTAT & SYSTICK_TISTAT_RESETDONE))
        ;
    SYSTICK_TIOCP_CFG = SYSTICK_TIOCP_IDLEMODE(0x1);  /* Ignore idle req */


    SYSTICK_TSICR = SYSTICK_TSICR_POSTED;

    /* s20.2.3.1.1 1ms Tick Generation from 32KHz functional clock */
    SYSTICK_TPIR =  232000; /* (INT(Fclk*Ttick) + 1)*1e6 - (Fclk*Ttick)*1e6 */
    while (SYSTICK_TWPS & SYSTICK_TWPS_W_PEND_TPIR)
        ;
    SYSTICK_TNIR = -768000; /*  INT(Fclk*Ttick)*1e6 - (Fclk*Ttick)*1e6      */
    while (SYSTICK_TWPS & SYSTICK_TWPS_W_PEND_TNIR)
        ;
    SYSTICK_TLDR = 0xFFFFFFE0; /* Overflow after 32 counts */

    while (SYSTICK_TWPS & SYSTICK_TWPS_W_PEND_TLDR)
        ;
    SYSTICK_TTGR = 0x0; /* Trigger load of TLDR */
    while (SYSTICK_TWPS & SYSTICK_TWPS_W_PEND_TTGR)
        ;

    hwInstallIRQ(IRQ_TINT1_1MS, systickISR, INT_PRIORITY_DEFAULT);

    SYSTICK_TIER = SYSTICK_TIER_OVF_IT_ENA; /* Enable overflow irq */

    SYSTICK_TCLR = SYSTICK_TCLR_AR | SYSTICK_TCLR_ST; /* Start, auto-reload */
}

/****************************
 * MMU
 ****************************/
static void memInit(void)
{
    mmuRegion_t ddr = {
        .physAddr    = 0x80000000,
        .virtAddr    = 0x80000000,
        .pages       = 512, /* MBs */
        .attributes  = MMU_ATTR_INNER_WT_NOWA
                     | MMU_ATTR_OUTER_WB_WA,
        .permissions = MMU_PERM_SYS_RW_USR_RW,
    };
    mmuRegion_t ocmc = {
        .physAddr    = 0x40300000,
        .virtAddr    = 0x40300000,
        .pages       = 1, /* MBs */
        .attributes  = MMU_ATTR_INNER_WT_NOWA
                     | MMU_ATTR_OUTER_WB_WA,
        .permissions = MMU_PERM_SYS_RW_USR_RW,
    };
    mmuRegion_t mmio = {
        .physAddr    = 0x44000000,
        .virtAddr    = 0x44000000,
        .pages       = 960, /* 0x440000 to 0x80000000 */
        .attributes  = MMU_ATTR_DEVICE_SHARED   /* via L3/L4 interconnect bus */
                     | MMU_ATTR_EXECUTE_NEVER,
        .permissions = MMU_PERM_SYS_RW_USR_RW,
    };

    _mmu_add_region(&ddr);
    _mmu_add_region(&ocmc);
    _mmu_add_region(&mmio);

    _dcache_flush();
    _icache_flush();
    _branch_predict_invalidate();

    _dcache_enable();
    _icache_enable();
    _branch_predict_enable();

    _mmu_enable();
}

/****************************
 * Performance Monitor
 ****************************/
enum {
    PERF_CNTR_DCACHE_MISS = 0,
    PERF_CNTR_DCACHE_USED,
};
static void perfMonInit(void)
{
    _perfmon_add(PERF_CNTR_DCACHE_MISS, PERF_MON_L1D_CACHE_REFILL);
    _perfmon_add(PERF_CNTR_DCACHE_USED, PERF_MON_L1D_CACHE);

    _perfmon_enable();
}
static void perfMonUpdate(void)
{
    /* TODO: something useful with this */
    volatile uint32_t dcacheMiss = _perfmon_get(PERF_CNTR_DCACHE_MISS);
    volatile uint32_t dcacheUsed = _perfmon_get(PERF_CNTR_DCACHE_USED);

    if (dcacheUsed == 0xffffffff) {
        _perfmon_reset(PERF_CNTR_DCACHE_MISS);
        _perfmon_reset(PERF_CNTR_DCACHE_USED);
    }
}


/*
 * Blinker threads
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *p)
{
  chRegSetThreadName("blinker1");
  while (TRUE) {
    chThdSleepMilliseconds(500);
    gpioToggle(HW_LED1_PORT, HW_LED1_PIN);
  }
  return 0;
}
static WORKING_AREA(waThread2, 128);
static msg_t Thread2(void *p)
{
  chRegSetThreadName("blinker2");
  while (TRUE) {
    chThdSleepMilliseconds(1000);
    gpioToggle(HW_LED2_PORT, HW_LED2_PIN);
    perfMonUpdate();
  }
  return 0;
}
static WORKING_AREA(waThread3, 128);
static msg_t Thread3(void *p)
{
  chRegSetThreadName("blinker3");
  while (TRUE) {
    chThdSleepMilliseconds(2000);
    gpioToggle(HW_LED3_PORT, HW_LED3_PIN);
  }
  return 0;
}


int main(void)
{
    extern uint32_t _exception_table_addr;  /* from linkerscript */
    uartCfg_t uartCfg = {
        .baud = BAUD_115200,
        .fifo = { .enable = TRUE,
                  .rxTrig = 1,
                  .txTrig = 1, },
    };

    gpioConfig(HW_LED0_PORT, HW_LED0_PIN, GPIO_CFG_OUTPUT);
    gpioConfig(HW_LED1_PORT, HW_LED1_PIN, GPIO_CFG_OUTPUT);
    gpioConfig(HW_LED2_PORT, HW_LED2_PIN, GPIO_CFG_OUTPUT);
    gpioConfig(HW_LED3_PORT, HW_LED3_PIN, GPIO_CFG_OUTPUT);

    gpioSet(HW_LED0_PORT, HW_LED0_PIN);
    gpioSet(HW_LED1_PORT, HW_LED1_PIN);
    gpioSet(HW_LED2_PORT, HW_LED2_PIN);
    gpioSet(HW_LED3_PORT, HW_LED3_PIN);

    _irq_disable();
    _irq_set_addr(&_exception_table_addr);

    /* Reset interrupt controller */
    memset(isrVectorTable, 0, sizeof(isrVectorTable));
    INTC_SYSCONFIG = INTC_SYSCONFIG_RESET;
    while (!(INTC_SYSSTATUS & INTC_SYSSTATUS_RESETDONE))
        ;
    INTC_IDLE      = INTC_IDLE_FUNCIDLE; /* Free running clock */
    INTC_THRESHOLD = 0xff;               /* Enable irq generation */

    uartConfig(UART_CONSOLE, &uartCfg);
    perfMonInit();
    memInit();
    systickInit();
    chSysInit();                         /* Enables IRQ's */

    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
    chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);
    chThdCreateStatic(waThread3, sizeof(waThread3), NORMALPRIO, Thread3, NULL);

    gpioClear(HW_LED0_PORT, HW_LED0_PIN);
    gpioClear(HW_LED1_PORT, HW_LED1_PIN);
    gpioClear(HW_LED2_PORT, HW_LED2_PIN);
    gpioClear(HW_LED3_PORT, HW_LED3_PIN);

    while (1) {
        gpioToggle(HW_LED0_PORT, HW_LED0_PIN);
        chThdSleepMilliseconds(250);
    }

    return 0;
}
