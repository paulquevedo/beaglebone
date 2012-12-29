#include "globalDefs.h"
#include "am335x.h"

enum {
    GPIO_CFG_INPUT  = BIT_0,
    GPIO_CFG_OUTPUT = BIT_1,
};

void gpioConfig(uint32_t base, uint32_t pin, uint32_t opt)
{
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
    /* Wait for clock activity */
    while(!(CM_PER_L4LS_CLKSTCTRL & CLKACTIVITY_GPIO_1_GDBCLK))
        ;

    CTRLM_CONF_GPMC_A(7) = CTRLM_CONF_MUXMODE(7);
    gpioConfig(GPIO1_BASE_ADDR, 23, GPIO_CFG_OUTPUT);

    while (1) {
        gpioClear(GPIO1_BASE_ADDR, 23);
        delay(0x3FFFF);
        gpioSet(GPIO1_BASE_ADDR, 23);
        delay(0x3FFFF);
    }

    return 0;
}
