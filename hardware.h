/******************************************************************************
 * hardware.h
 *
 * Board level defines for the am335x beaglebone
 *
 * Paul Quevedo 2012
 *****************************************************************************/
#include "globalDefs.h"
#include "am335x.h"

#ifndef __HARDWARE_H__
#define __HARDWARE_H__

/**********************
 * Board Defines
 *********************/
#define MASTER_OSC 24000000 /* 24MHz crystal */

/* DDR PLL
 * CLK_M_OSC is 24MHz.
 * CLK_OUT   is CLK_M_OSC * M / ((N + 1) * M2)
 *           defined in s7.3.3.2 of am335x TRM */
#define DDR_OSC 266000000
#define DDR_PLL_M  266
#define DDR_PLL_N  23
#define DDR_PLL_M2 1

/* User LEDs */
#define HW_LED0_PORT GPIO1_BASE_ADDR
#define HW_LED0_PIN  21
#define HW_LED1_PORT GPIO1_BASE_ADDR
#define HW_LED1_PIN  22
#define HW_LED2_PORT GPIO1_BASE_ADDR
#define HW_LED2_PIN  23
#define HW_LED3_PORT GPIO1_BASE_ADDR
#define HW_LED3_PIN  24

/**********************
 * GPIO
 *********************/
enum {
    GPIO_CFG_INPUT  = BIT_0,
    GPIO_CFG_OUTPUT = BIT_1,
};

extern void gpioConfig(uint32_t base, uint32_t pin, uint32_t opt);
extern void gpioToggle(uint32_t base, uint32_t pin);
extern void gpioClear (uint32_t base, uint32_t pin);
extern void gpioSet   (uint32_t base, uint32_t pin);

#endif
