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
