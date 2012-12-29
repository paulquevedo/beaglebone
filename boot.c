#include "globalDefs.h"
#include "am335x.h"
#include "hardware.h"

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

    gpioConfig(HW_LED0_PORT, HW_LED0_PIN, GPIO_CFG_OUTPUT);

    while (1) {
        gpioToggle(HW_LED0_PORT, HW_LED0_PIN);
        delay(0x3FFFF);
    }

    return 0;
}
