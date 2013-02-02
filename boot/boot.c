/******************************************************************************
 *
 * boot.c
 *
 * Bootloader for the am335x beaglebone
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "globalDefs.h"
#include "am335x.h"
#include "hardware.h"
#include "ff.h"
#include "xmodem.h"

static void delay(volatile uint32_t count)
{
    while (count--)
        ;
}

static void pllCoreInit(void)
{
    /* Core PLL Init as per s8.1.6.7.1 of am335x TRM */
    /* 1. Switch PLL to bypass mode */
    volatile uint32_t value;
    value  = CM_CLKMODE_DPLL_CORE   & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_MN_BYP_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_CORE = value;

    /* 2. Wait for PLL bypass */
    while (!(CM_IDLEST_DPLL_CORE & CM_DPLL_ST_MN_BYPASS))
        ;

    /* 3. Configure PLL mult and div */
    value  = CM_CLKSEL_DPLL_CORE & ~(CM_DPLL_MULT_MASK | CM_DPLL_DIV_MASK);
    value |= CORE_PLL_M << CM_DPLL_MULT_SHFT;
    value |= CORE_PLL_N << CM_DPLL_DIV_SHFT;
    CM_CLKSEL_DPLL_CORE = value;

    /* 4. Configure M4,5,6 Divider */
    value  = CM_DIV_M4_DPLL_CORE & ~CM_DPLL_HSDIVIDER_DIV_MASK;
    value |= CORE_PLL_M4 << CM_DPLL_HSDIVIDER_DIV_SHFT;
    CM_DIV_M4_DPLL_CORE  = value;

    value  = CM_DIV_M5_DPLL_CORE & ~CM_DPLL_HSDIVIDER_DIV_MASK;
    value |= CORE_PLL_M5 << CM_DPLL_HSDIVIDER_DIV_SHFT;
    CM_DIV_M5_DPLL_CORE  = value;

    value  = CM_DIV_M6_DPLL_CORE & ~CM_DPLL_HSDIVIDER_DIV_MASK;
    value |= CORE_PLL_M6 << CM_DPLL_HSDIVIDER_DIV_SHFT;
    CM_DIV_M6_DPLL_CORE  = value;

    /* 5. Switch PLL to lock mode */
    value  = CM_CLKMODE_DPLL_CORE & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_LOCK_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_CORE = value;

    /* 6. Wait for pll to lock */
    while (!(CM_IDLEST_DPLL_CORE & CM_DPLL_ST_DPLL_CLK))
        ;
}

static void pllPerInit(void)
{
    /* Peripheral PLL Init as per s8.1.6.8.1 of am335x TRM */
    /* 1. Switch PLL to bypass mode */
    volatile uint32_t value;
    value  = CM_CLKMODE_DPLL_PER    & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_MN_BYP_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_PER = value;

    /* 2. Wait for PLL bypass */
    while (!(CM_IDLEST_DPLL_PER & CM_DPLL_ST_MN_BYPASS))
        ;

    /* 3. Configure PLL mult and div */
    value  = CM_CLKSEL_DPLL_PERIPH & ~(CM_DPLL_MULT_MASK | CM_DPLL_DIV_MASK);
    value |= PER_PLL_M << CM_DPLL_MULT_SHFT;
    value |= PER_PLL_N << CM_DPLL_DIV_SHFT;
    CM_CLKSEL_DPLL_PERIPH = value;

    /* 4. Configure M2 Divider */
    value  = CM_DIV_M2_DPLL_PER & ~CM_DPLL_M2_CLKOUT_DIV_MASK;
    value |= PER_PLL_M2 << CM_DPLL_M2_CLKOUT_DIV_SHFT;
    CM_DIV_M2_DPLL_PER = value;

    /* 5. Switch PLL to lock mode */
    value  = CM_CLKMODE_DPLL_PER  & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_LOCK_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_PER = value;

    /* 6. Wait for pll to lock */
    while (!(CM_IDLEST_DPLL_PER & CM_DPLL_ST_DPLL_CLK))
        ;
}

static void pllMpuInit(void)
{
    /* MPU (MicroProcessor) PLL Init as per s8.1.6.9.1 of am335x TRM */
    /* 1. Switch PLL to bypass mode */
    volatile uint32_t value;
    value  = CM_CLKMODE_DPLL_MPU    & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_MN_BYP_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_MPU = value;

    /* 2. Wait for PLL bypass */
    while (!(CM_IDLEST_DPLL_MPU & CM_DPLL_ST_MN_BYPASS))
        ;

    /* 3. Configure PLL mult and div */
    value  = CM_CLKSEL_DPLL_MPU & ~(CM_DPLL_MULT_MASK | CM_DPLL_DIV_MASK);
    value |= MPU_PLL_M << CM_DPLL_MULT_SHFT;
    value |= MPU_PLL_N << CM_DPLL_DIV_SHFT;
    CM_CLKSEL_DPLL_MPU = value;

    /* 4. Configure M2 Divider */
    value  = CM_DIV_M2_DPLL_MPU & ~CM_DPLL_M2_CLKOUT_DIV_MASK;
    value |= MPU_PLL_M2 << CM_DPLL_M2_CLKOUT_DIV_SHFT;
    CM_DIV_M2_DPLL_MPU = value;

    /* 5. Switch PLL to lock mode */
    value  = CM_CLKMODE_DPLL_MPU  & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_LOCK_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_MPU = value;

    /* 6. Wait for pll to lock */
    while (!(CM_IDLEST_DPLL_MPU & CM_DPLL_ST_DPLL_CLK))
        ;
}

static void pllDDRInit(void)
{
    /* DDR PLL Init as per s8.1.6.11.1 of the am335x TRM */
    /* 1. Switch PLL to bypass mode */
    volatile uint32_t value;
    value  = CM_CLKMODE_DPLL_DDR    & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_MN_BYP_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_DDR = value;

    /* 2. Wait for PLL to be in bypass */
    while (!(CM_IDLEST_DPLL_DDR & CM_DPLL_ST_MN_BYPASS))
        ;

    /* 3. Configure PLL mult and div */
    value  = CM_CLKSEL_DPLL_DDR & ~(CM_DPLL_MULT_MASK | CM_DPLL_DIV_MASK);
    value |= DDR_PLL_M << CM_DPLL_MULT_SHFT;
    value |= DDR_PLL_N << CM_DPLL_DIV_SHFT;
    CM_CLKSEL_DPLL_DDR = value;

    /* 4. Configure M2 Divider */
    value  = CM_DIV_M2_DPLL_DDR & ~CM_DPLL_M2_CLKOUT_DIV_MASK;
    value |= DDR_PLL_M2 << CM_DPLL_M2_CLKOUT_DIV_SHFT;
    CM_DIV_M2_DPLL_DDR = value;

    /* 5. Switch PLL to lock mode */
    value  = CM_CLKMODE_DPLL_DDR  & ~CM_DPLL_EN_MASK;
    value |= CM_DPLL_EN_LOCK_MODE << CM_DPLL_EN_SHFT;
    CM_CLKMODE_DPLL_DDR = value;

    /* 6. Wait for PLL to lock */
    while (!(CM_IDLEST_DPLL_DDR & CM_DPLL_ST_DPLL_CLK))
        ;
}

static void emifInit(void)
{
    uint32_t mask;

    CM_MODULEMODE_ENABLE(CM_PER_EMIF_FW_CLKCTRL);
    CM_MODULEMODE_ENABLE(CM_PER_EMIF_CLKCTRL);

    mask = CM_CLKSTCTRL_CLKACTIVITY_EMIF_GCLK
         | CM_CLKSTCTRL_CLKACTIVITY_L3_GCLK;
    while ((CM_PER_L3_CLKSTCTRL & mask) != mask)
        ;
}

/* ddr2Init()
 * ddr2PhyInit()
 *
 * This is just taken from the DDR2Init function from TI Starterware.
 * There's a whole page and spreadsheets to come up with the parameters
 * but I really don't care.
 * http://processors.wiki.ti.com/index.php/AM335x_EMIF_Configuration_tips */
static void ddr2PhyInit(void)
{
    CTRLM_VTP_CTRL |=  CTRLM_VTP_CTRL_ENABLE;
    CTRLM_VTP_CTRL &= ~CTRLM_VTP_CTRL_CLRZ;
    CTRLM_VTP_CTRL |=  CTRLM_VTP_CTRL_CLRZ;
    while (!(CTRLM_VTP_CTRL & CTRLM_VTP_CTRL_READY))
        ;

    /* DDR PHY CMD0 Register configuration */
    DDRPHY_CMD0_SLAVE_RATIO_0   = 0x80;
    DDRPHY_CMD0_SLAVE_FORCE_0   = 0x0;
    DDRPHY_CMD0_SLAVE_DELAY_0   = 0x0;
    DDRPHY_CMD0_LOCK_DIFF_0     = 0x4;
    DDRPHY_CMD0_INVERT_CLKOUT_0 = 0x0;

    /* DDR PHY CMD1 Register configuration */
    DDRPHY_CMD1_SLAVE_RATIO_0   = 0x80;
    DDRPHY_CMD1_SLAVE_FORCE_0   = 0x0;
    DDRPHY_CMD1_SLAVE_DELAY_0   = 0x0;
    DDRPHY_CMD1_LOCK_DIFF_0     = 0x4;
    DDRPHY_CMD1_INVERT_CLKOUT_0 = 0x0;

    /* DDR PHY CMD2 Register configuration */
    DDRPHY_CMD2_SLAVE_RATIO_0   = 0x80;
    DDRPHY_CMD2_SLAVE_FORCE_0   = 0x0;
    DDRPHY_CMD2_SLAVE_DELAY_0   = 0x0;
    DDRPHY_CMD2_LOCK_DIFF_0     = 0x4;
    DDRPHY_CMD2_INVERT_CLKOUT_0 = 0x0;

    /* DATA macro configuration */
    DDRPHY_DATA0_RD_DQS_SLAVE_RATIO_0  = 0x04010040;
    DDRPHY_DATA0_RD_DQS_SLAVE_RATIO_1  = 0x00000010;
    DDRPHY_DATA0_WR_DQS_SLAVE_RATIO_0  = 0x0;
    DDRPHY_DATA0_WR_DQS_SLAVE_RATIO_1  = 0x0;
    DDRPHY_DATA0_WRLVL_INIT_RATIO_0    = 0x0;
    DDRPHY_DATA0_WRLVL_INIT_RATIO_1    = 0x0;
    DDRPHY_DATA0_GATELVL_INIT_RATIO_0  = 0x0;
    DDRPHY_DATA0_GATELVL_INIT_RATIO_1  = 0x0;
    DDRPHY_DATA0_FIFO_WE_SLAVE_RATIO_0 = 0x85615856;
    DDRPHY_DATA0_FIFO_WE_SLAVE_RATIO_1 = 0x00000015;
    DDRPHY_DATA0_WR_DATA_SLAVE_RATIO_0 = 0x04010040;
    DDRPHY_DATA0_WR_DATA_SLAVE_RATIO_1 = 0x00000010;
    DDRPHY_DATA0_LOCK_DIFF_0           = 0x0;

    DDRPHY_DATA1_RD_DQS_SLAVE_RATIO_0  = 0x04010040;
    DDRPHY_DATA1_RD_DQS_SLAVE_RATIO_1  = 0x00000010;
    DDRPHY_DATA1_WR_DQS_SLAVE_RATIO_0  = 0x0;
    DDRPHY_DATA1_WR_DQS_SLAVE_RATIO_1  = 0x0;
    DDRPHY_DATA1_WRLVL_INIT_RATIO_0    = 0x0;
    DDRPHY_DATA1_WRLVL_INIT_RATIO_1    = 0x0;
    DDRPHY_DATA1_GATELVL_INIT_RATIO_0  = 0x0;
    DDRPHY_DATA1_GATELVL_INIT_RATIO_1  = 0x0;
    DDRPHY_DATA1_FIFO_WE_SLAVE_RATIO_0 = 0x85615856;
    DDRPHY_DATA1_FIFO_WE_SLAVE_RATIO_1 = 0x00000015;
    DDRPHY_DATA1_WR_DATA_SLAVE_RATIO_0 = 0x04010040;
    DDRPHY_DATA1_WR_DATA_SLAVE_RATIO_1 = 0x00000010;
    DDRPHY_DATA1_LOCK_DIFF_0           = 0x0;

    DDRPHY_DATA0_USE_RANK0_DELAYS_0    = 1;
    DDRPHY_DATA1_USE_RANK0_DELAYS_0    = 1;
}

static void ddr2Init(void)
{
    ddr2PhyInit();

    CTRLM_DDR_CMD0_IOCTRL  = 0x18B;
    CTRLM_DDR_CMD1_IOCTRL  = 0x18B;
    CTRLM_DDR_CMD2_IOCTRL  = 0x18B;
    CTRLM_DDR_DATA0_IOCTRL = 0x18B;
    CTRLM_DDR_DATA1_IOCTRL = 0x18B;

    CTRLM_DDR_IO_CTRL  &= 0x0FFFFFFF;
    CTRLM_DDR_CKE_CTRL |= BIT_0;    /* Normal operation */

    EMIF_DDR_PHY_CTRL_1      = 0x05;
    EMIF_DDR_PHY_CTRL_1_SHDW = 0x05;
    EMIF_DDR_PHY_CTRL_2      = 0x05;

    EMIF_SDRAM_TIM_1         = 0x0666B3D6;
    EMIF_SDRAM_TIM_1_SHDW    = 0x0666B3D6;
    EMIF_SDRAM_TIM_2         = 0x143731DA;
    EMIF_SDRAM_TIM_2_SHDW    = 0x143731DA;
    EMIF_SDRAM_TIM_3         = 0x00000347;
    EMIF_SDRAM_TIM_3_SHDW    = 0x00000347;
    EMIF_SDRAM_CONFIG        = 0x41805332;
    EMIF_SDRAM_REF_CTRL      = 0x00004650;
    EMIF_SDRAM_REF_CTRL_SHDW = 0x00004650;

    delay(5000);

    EMIF_SDRAM_REF_CTRL      = 0x0000081A;
    EMIF_SDRAM_REF_CTRL_SHDW = 0x0000081A;
    EMIF_SDRAM_CONFIG        = 0x41805332;

    CTRLM_CONTROL_EMIF_SDRAM_CONFIG = 0x41805332;
}

static int ddrtest(void)
{
    volatile uint32_t *dstPtr = (volatile uint32_t *)0x80000000;
    while ((uint32_t)dstPtr < 0x8FA00000) {
        *dstPtr = 0x12345678;
        if (*dstPtr != 0x12345678) {
            return ERROR;
        }
        dstPtr += 0x40;
    }
    return OK;
}


#define BAD_ADDRESS 0xffffffff
static uint32_t imageCopy(void)
{
    FIL fp;
    UINT bytesRead;
    FRESULT result;
    uint32_t imageSize;
    uint32_t loadAddr;
    uint32_t *dstPtr;
    static uint32_t buffer[512];

    memset(&fp, 0, sizeof(fp));

    result = f_open(&fp, "/app", FA_READ);
    if (result != FR_OK) {
        result = f_open(&fp, "/APP", FA_READ);
        if (result != FR_OK) {
            uartPuts("Failed to open application file");
            return BAD_ADDRESS;
        }
    }

    if (f_read(&fp, &imageSize, 4, &bytesRead) != FR_OK || bytesRead != 4) {
        uartPuts("Failed to Read application File");
        f_close(&fp);
        return BAD_ADDRESS;
    }

    if (f_read(&fp, &loadAddr, 4, &bytesRead) != FR_OK || bytesRead != 4) {
        uartPuts("Failed to Read application File");
        f_close(&fp);
        return BAD_ADDRESS;
    }

    imageSize -= 8; /* Remove header info */
#if DEBUG
    iprintf("Loading to addr %08x size %x\n\r", loadAddr, imageSize);
#else
    uartPuts("Image loading...");
#endif
    dstPtr = (uint32_t *)loadAddr;

    while (imageSize) {
        uint32_t chunk = (imageSize > 512) ? 512 : imageSize;
        if (f_read(&fp, buffer, chunk, &bytesRead) != FR_OK
                                     || bytesRead  != chunk) {
            uartPuts("Failed to read a chunk");
            f_close(&fp);
            return BAD_ADDRESS;
        }
        memcpy(dstPtr, buffer, chunk);
        dstPtr    += (chunk / 4);
        imageSize -= chunk;
    }

    f_close(&fp);

    return loadAddr;
}

static bool32_t isImagePresent(void)
{
    FIL fp;
    FRESULT result;

    memset(&fp, 0, sizeof(fp));

    result = f_open(&fp, "/app", FA_READ);
    if (result == FR_OK)
        f_close(&fp);

    return (result == FR_OK);
}

static int32_t loadNewImage(void)
{
    static uint8_t rxBuffer[1024];
    xmodemCfg_t xmodemCfg = {
        .numRetries = 0x2000,
        .uartFd = UART_CONSOLE,
    };

    FIL fp;
    UINT bytesWritten;
    int32_t retVal = OK;
    int xferStarted = FALSE;

    xmodemInit(&xmodemCfg);

    if (f_open(&fp, "/app", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        uartPuts("Failed to create file");
        return ERROR;
    }
    f_lseek(&fp, 0);

    uartPuts("Waiting for XMODEM Transfer to begin");

    while (1) {
        int len = xmodemRecv(rxBuffer, sizeof(rxBuffer));

        if (len == 0) {
            uartPuts("Programming succesfull!");
            break;
        }
        else if (len > 0) {
            xferStarted = TRUE;
            if (f_write(&fp, rxBuffer, len, &bytesWritten) != FR_OK
                                          || bytesWritten  != len) {
                xmodemAbort();
                uartPuts("Failed to write chunk");
                retVal = ERROR;
                break;
            }
        }
        else if (xferStarted) {
            xmodemAbort();
            uartPuts("Error in transfer");
            retVal = ERROR;
            break;
        }

        gpioToggle(HW_LED0_PORT, HW_LED0_PIN);
    }
    f_close(&fp);

    return retVal;
}


int main(void)
{
    uartCfg_t uartCfg = {
        .baud = BAUD_115200,
        .fifo = { .enable = TRUE,
                  .rxTrig = 1,
                  .txTrig = 1, },
    };
    bool32_t imagePresent;
    FATFS fatfs;

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

    pllCoreInit();
    pllPerInit();
    pllMpuInit();
    pllDDRInit();
    emifInit();
    ddr2Init();
    uartConfig(UART_CONSOLE, &uartCfg);

    gpioConfig(HW_LED0_PORT, HW_LED0_PIN, GPIO_CFG_OUTPUT);
    gpioConfig(HW_LED1_PORT, HW_LED1_PIN, GPIO_CFG_OUTPUT);

    uartPuts("AM355x BeagleBone Bootloader");
    uartPuts("Executing DDR Test...");

    gpioSet(HW_LED1_PORT, HW_LED1_PIN);
    if (ddrtest() != ERROR) {
        uartPuts("DDR OK");
        gpioClear(HW_LED1_PORT, HW_LED1_PIN);
    } else {
        uartPuts("DDR ERROR");
        while (1) {
            gpioToggle(HW_LED1_PORT, HW_LED1_PIN);
            delay(0x4FFFF);
        }
    }

    memset(&fatfs, 0, sizeof(fatfs));
    if (f_mount(0, &fatfs) != FR_OK) {
        uartPuts("Failed to mount SD Card");
        while (1) {
            gpioToggle(HW_LED1_PORT, HW_LED1_PIN);
            delay(0x4FFFF);
        }
    }

    if (isImagePresent()) {
        uint8_t c;
        int i;
        uartPuts("Press any key to transfer new image...");

        for (i = 4; i >= 0; i--) {
            if (i)
                uartPuts("Tick...");
            else
                uartPuts("Tock!");
            delay(0x1FFFFF);
            if (uartRead(UART_CONSOLE, &c, 1) == 1) {
                loadNewImage();
                break;
            }
        }
    }

    imagePresent = isImagePresent();
    while(1) {
        if (!imagePresent) {
            uartPuts("No image detected. Waiting for file transfer...");

            imagePresent = loadNewImage() != ERROR;
        }

        if (imagePresent) {
            void (*imgPtr)() = (void *)imageCopy();

            if ((uint32_t)imgPtr != BAD_ADDRESS) {
                uartPuts("Jumping to Application");
                (*imgPtr)();
            }
            uartPuts("Failed to load image");
            imagePresent = FALSE;
        }

        gpioToggle(HW_LED0_PORT, HW_LED0_PIN);
        delay(0x6FFFF);
    }

    return 0;
}
