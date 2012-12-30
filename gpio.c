/******************************************************************************
 * gpio.c
 *
 * GPIO Driver for the beaglebone/am335x processor
 *
 * Paul Quevedo 2012
 *****************************************************************************/
#include "globalDefs.h"
#include "am335x.h"
#include "hardware.h"

void gpioConfig(uint32_t base, uint32_t pin, uint32_t opt)
{
    /* GPIO Clock */
    switch (base) {
    case GPIO1_BASE_ADDR:
        CM_MODULEMODE_ENABLE(CM_PER_GPIO1_CLKCTRL);
        CM_PER_GPIO1_CLKCTRL |= BIT_18; /* Debounce Clk */
        while(!(CM_PER_L4LS_CLKSTCTRL & CLKACTIVITY_GPIO_1_GDBCLK))
            ;
        break;
    }

    /* Pin mux */
    switch (base) {
    case GPIO1_BASE_ADDR:
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

void gpioSet(uint32_t base, uint32_t pin)
{
    GPIO_SETDATAOUT(base) = (1 << pin);
}

void gpioClear(uint32_t base, uint32_t pin)
{
    GPIO_CLEARDATAOUT(base) = (1 << pin);
}

void gpioToggle(uint32_t base, uint32_t pin)
{
    if (GPIO_SETDATAOUT(base)   & (1 << pin))
        GPIO_CLEARDATAOUT(base) = (1 << pin);
    else
        GPIO_SETDATAOUT(base)   = (1 << pin);
}

