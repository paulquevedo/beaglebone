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
 * GPIO
 *********************/
enum {
    GPIO_0,
    GPIO_1,
    GPIO_2,
    GPIO_3,
};

enum {
    GPIO_CFG_INPUT  = BIT_0,
    GPIO_CFG_OUTPUT = BIT_1,
};

extern void gpioConfig(uint32_t inst, uint32_t pin, uint32_t opt);
extern void gpioToggle(uint32_t inst, uint32_t pin);
extern void gpioClear (uint32_t inst, uint32_t pin);
extern void gpioSet   (uint32_t inst, uint32_t pin);

/**********************
 * UART
 *********************/
enum {
    UART_0,
    UART_1,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
};

enum {
    BAUD_300,
    BAUD_600,
    BAUD_1200,
    BAUD_2400,
    BAUD_4800,
    BAUD_9600,
    BAUD_14400,
    BAUD_19200,
    BAUD_28800,
    BAUD_38400,
    BAUD_57600,
    BAUD_115200,
    BAUD_230400,
};

typedef struct {
    uint32_t baud;
    struct {
        bool32_t enable;
        uint32_t rxTrig;
        uint32_t txTrig;
    } fifo;
} uartCfg_t;

extern void uartConfig(uint32_t inst, uartCfg_t *cfg);

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
#define HW_LED0_PORT GPIO_1
#define HW_LED0_PIN  21
#define HW_LED1_PORT GPIO_1
#define HW_LED1_PIN  22
#define HW_LED2_PORT GPIO_1
#define HW_LED2_PIN  23
#define HW_LED3_PORT GPIO_1
#define HW_LED3_PIN  24

#endif
