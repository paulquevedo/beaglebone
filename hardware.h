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
 * Clock Frequencies
 *********************/
#define MASTER_OSC 24000000 /* 24MHz crystal */

/* All clocks are generated from the master ocsillator.
 * All peripherls/modules run off one of the following clock
 * outputs. There may be an additioanl fixed divider into
 * a perpheral/module */

/* CORE PLL via ADPLL
 * CLK_M_OSC is 24MHz.
 * CLKDCOLDO is 2000MHZ
 * CLK_OUTM4 is 200MHz
 * CLK_OUTM5 is 250MHz
 * CLK_OUTM6 is 500MHz
 *           2 * [M / (N + 1)] * CLK_M_OSC
 *           defined in s8.1.6.3.1 of am335x TRM */
#define CORE_CLKDCOLDO 2000000000
#define CORE_CLKOUTM4  200000000
#define CORE_CLKOUTM5  250000000
#define CORE_CLKOUTM6  500000000
#define CORE_PLL_M  1000
#define CORE_PLL_N  23
#define CORE_PLL_M4 10
#define CORE_PLL_M5 8
#define CORE_PLL_M6 4

/* PER PLL via ADPLLLJ (Low Jitter)
 * CLK_M_OSC is 24MHz.
 * CLK_OUT   is 960MHZ
 * CLK_OUTM2 is 192MHz
 *           [M / (N + 1)] * CLK_M_OSC * [1/M2]
 *           defined in s8.1.6.4.1 of am335x TRM */
#define PER_CLKOUT   960000000
#define PER_CLKOUTM2 192000000
#define PER_PLL_M  960
#define PER_PLL_N  23
#define PER_PLL_M2 5

/* MPU (MicroProcessor) PLL via ADPLL
 * CLK_M_OSC is 24MHz
 * CLK_OUT   is 720MHz
 *           [M / (N + 1)] * CLK_M_OSC * [1/M2]
 *           defined in s8.1.6.3.1 of am335x TRM */
#define MPU_CLKOUT 720000000
#define MPU_PLL_M  720
#define MPU_PLL_N  23
#define MPU_PLL_M2 1

/* DDR PLL via ADPLL
 * CLK_M_OSC is 24MHz.
 * CLK_OUT   is 266MHz
 *           [M / (N + 1)] * CLK_M_OSC * [1/M2]
 *           defined in s8.1.6.3.1 of am335x TRM */
#define DDR_CLKOUT 266000000
#define DDR_PLL_M  266
#define DDR_PLL_N  23
#define DDR_PLL_M2 1


/**********************
 * Board Defines
 *********************/
/* User LEDs */
#define HW_LED0_PORT GPIO_1
#define HW_LED0_PIN  21
#define HW_LED1_PORT GPIO_1
#define HW_LED1_PIN  22
#define HW_LED2_PORT GPIO_1
#define HW_LED2_PIN  23
#define HW_LED3_PORT GPIO_1
#define HW_LED3_PIN  24

/* System Console */
#define UART_CONSOLE UART_0

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

extern int  uartConfig(uint32_t inst, uartCfg_t *cfg);
extern int  uartWrite (uint32_t inst, uint8_t *data, uint32_t len);
extern int  uartRead  (uint32_t inst, uint8_t *data, uint32_t len);
extern void uartPuts  (char *str);

enum {
    MMCSD_0,
    MMCSD_1,
    MMCSD_2,
};
extern int mmcsdInit(uint32_t inst);
#endif
