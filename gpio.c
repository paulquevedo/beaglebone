/******************************************************************************
 * gpio.c
 *
 * GPIO Driver for the beaglebone/am335x processor
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *****************************************************************************/
#include "globalDefs.h"
#include "am335x.h"
#include "hardware.h"

#define CLK_INPUT_FREQ          (CORE_CLKOUTM4 / 2)
#define CLK_DEBOUNCE_INPUT_FREQ (PER_CLKOUTM2 / 5859.375)

static uint32_t inst2Base[] = {
    [GPIO_0] = GPIO0_BASE_ADDR,
    [GPIO_1] = GPIO1_BASE_ADDR,
    [GPIO_2] = GPIO2_BASE_ADDR,
    [GPIO_3] = GPIO3_BASE_ADDR,
};

void gpioConfig(uint32_t inst, uint32_t pin, uint32_t opt)
{
    uint32_t base = inst2Base[inst];

    /* GPIO Clock */
    switch (inst) {
    case GPIO_1:
        CM_MODULEMODE_ENABLE(CM_PER_GPIO1_CLKCTRL);
        CM_PER_GPIO1_CLKCTRL |= BIT_18; /* Debounce Clk */
        while(!(CM_PER_L4LS_CLKSTCTRL & CM_CLKACTIVITY_GPIO_1_GDBCLK))
            ;
        break;
    }

    /* Pin mux */
    switch (inst) {
    case GPIO_1:
        if (pin >= 16 && pin <= 27)
            CTRLM_CONF_GPMC_A(pin-16) = CTRLM_CONF_MUXMODE(7);
        break;
    }

    /* I/O */
    if (opt & GPIO_CFG_OUTPUT)
        GPIO_OE(base) &= ~(1 << pin);
    else if (opt & GPIO_CFG_INPUT)
        GPIO_OE(base) |=  (1 << pin);
}

void gpioSet(uint32_t inst, uint32_t pin)
{
    uint32_t base = inst2Base[inst];
    GPIO_SETDATAOUT(base) = (1 << pin);
}

void gpioClear(uint32_t inst, uint32_t pin)
{
    uint32_t base = inst2Base[inst];
    GPIO_CLEARDATAOUT(base) = (1 << pin);
}

void gpioToggle(uint32_t inst, uint32_t pin)
{
    uint32_t base = inst2Base[inst];
    if (GPIO_SETDATAOUT(base)   & (1 << pin))
        GPIO_CLEARDATAOUT(base) = (1 << pin);
    else
        GPIO_SETDATAOUT(base)   = (1 << pin);
}

