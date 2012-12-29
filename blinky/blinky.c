/*****************************************************************************
 * blinky.c
 *
 * Bare-metal bring up to blink the LED. Code is loaded by the ROM bootloader
 * to the on-chip SRAM. This code does not setup the DDR
 *
 * Copyright Paul Quevedo 2012
 *
 *****************************************************************************/
#include "../globalDefs.h"
#include "../am335x.h"

/* USER_LED0 defined in section 6.7.3 of the beaglebone SRM */
#define USER_LED_PORT GPIO1_BASE_ADDR
#define USER_LED_PIN  21
#define USER_LED_GPMC 5

static void delay(uint32_t count)
{
    while (count--)
        ;
}

int main(void)
{
    /* Disable watchdog */
    WDT_WSPR = 0xAAAA;
    while (WDT_WWPS & WDT_WWPS_W_PEND_WSPR)
        ;
    WDT_WSPR = 0x5555;
    while (WDT_WWPS & WDT_WWPS_W_PEND_WSPR)
        ;

    /* L3 & L4 Interconnect clocks */
    /* Enable module/instance level clocks */
    CM_MODULEMODE_ENABLE(CM_PER_L3_CLKCTRL);
    CM_MODULEMODE_ENABLE(CM_PER_L4LS_CLKCTRL);
    CM_MODULEMODE_ENABLE(CM_PER_L4FW_CLKCTRL);
    CM_MODULEMODE_ENABLE(CM_PER_L4HS_CLKCTRL);
    CM_MODULEMODE_ENABLE(CM_PER_L3_INSTR_CLKCTRL);
    CM_MODULEMODE_ENABLE(CM_WKUP_L4WKUP_CLKCTRL);

    /* Enable domain level clocks */
    CM_PER_L3_CLKSTCTRL   = CM_CLKTRCTRL_SW_WKUP;
    CM_PER_L4LS_CLKSTCTRL = CM_CLKTRCTRL_SW_WKUP;
    CM_WKUP_CLKSTCTRL     = CM_CLKTRCTRL_SW_WKUP;
    CM_PER_L4FW_CLKSTCTRL = CM_CLKTRCTRL_SW_WKUP;
    CM_PER_L3S_CLKSTCTRL  = CM_CLKTRCTRL_SW_WKUP;

    /* Enable control module clock */
    CM_MODULEMODE_ENABLE(CM_WKUP_CONTROL_CLKCTRL);

    /* Enable GPIO1 Clock */
    CM_MODULEMODE_ENABLE(CM_PER_GPIO1_CLKCTRL);
    /* Enable Debounce Functional Clk */
    CM_PER_GPIO1_CLKCTRL |= BIT_18;
    /* Wait for gpio clock activity */
    while(!(CM_PER_L4LS_CLKSTCTRL & CLKACTIVITY_GPIO_1_GDBCLK))
        ;

    /* Set pin mux to GPIO. Modes are defined in s2.2 of am335x datasheet */
    CTRLM_CONF_GPMC_A(USER_LED_GPMC) = CTRLM_CONF_MUXMODE(7);

    /* Configure pin as output */
    GPIO_OE(USER_LED_PORT) &= ~(1 << USER_LED_PIN);

    while (1) {
        GPIO_CLEARDATAOUT(USER_LED_PORT) = (1 << USER_LED_PIN);
        delay(0x3FFFF);
        GPIO_SETDATAOUT(USER_LED_PORT)   = (1 << USER_LED_PIN);
        delay(0x3FFFF);
    }

    return 0;
}
