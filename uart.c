/******************************************************************************
 * uart.c
 *
 * UART Driver for the beaglebone/am335x processor
 *
 * Paul Quevedo 2012
 *****************************************************************************/
#include "globalDefs.h"
#include "am335x.h"
#include "hardware.h"

#define CLK_INPUT_FREQ (PER_CLKOUTM2 / 4)

static const uint32_t inst2Base[] = {
    [UART_0] = UART0_BASE_ADDR,
    [UART_1] = UART1_BASE_ADDR,
    [UART_2] = UART2_BASE_ADDR,
    [UART_3] = UART3_BASE_ADDR,
    [UART_4] = UART4_BASE_ADDR,
    [UART_5] = UART5_BASE_ADDR,
};

/* Divided from 48MHz CLK_INPUT_FREQ */
static const uint16_t baud2Div[] = {
    [BAUD_300]    = 10000,
    [BAUD_600]    = 5000,
    [BAUD_1200]   = 2500,
    [BAUD_2400]   = 1250,
    [BAUD_4800]   = 625,
    [BAUD_9600]   = 313,
    [BAUD_14400]  = 208,
    [BAUD_19200]  = 156,
    [BAUD_28800]  = 104,
    [BAUD_38400]  = 78,
    [BAUD_57600]  = 52,
    [BAUD_115200] = 26,
    [BAUD_230400] = 13,
};

static const uint32_t REG_CONFIG_MODE_A = 0x0080;
static const uint32_t REG_CONFIG_MODE_B = 0x00BF;

/*
 * uartFifoConfig()
 *
 * Configures the FIFO. s19.4.1.1.2 of the am335x TRM. This is absurd
 */
static void uartFifoConfig(uint32_t base, uartCfg_t *cfg)
{
    /* Restore values */
    uint32_t lcrValue;
    uint32_t efrValue;
    uint32_t mcrValue;

    /* FIFO Config Values */
    uint32_t fcrValue = 0;
    uint32_t tlrValue = 0;
    uint32_t scrValue = 0;

    if (cfg->fifo.enable) {
        fcrValue |= UART_FCR_FIFO_EN;
        /* 6bits, 1 byte granularity, [0:1] in FCR, [2:5] in TLR */
        fcrValue |= (cfg->fifo.rxTrig & 0x03) << UART_FCR_RX_FIFO_TRIG_SHFT;
        fcrValue |= (cfg->fifo.txTrig & 0x03) << UART_FCR_TX_FIFO_TRIG_SHFT;
        tlrValue |= (cfg->fifo.rxTrig >> 2)   << UART_TLR_RX_FIFO_TRIG_SHFT;
        tlrValue |= (cfg->fifo.txTrig >> 2)   << UART_TLR_TX_FIFO_TRIG_SHFT;
        scrValue |= UART_SCR_RX_TRIG_GRANU1;
        scrValue |= UART_SCR_TX_TRIG_GRANU1;
    }

    /* 1. Switch to configuration mode B to access EFR*/
    lcrValue = UART_LCR(base);
    UART_LCR(base) = REG_CONFIG_MODE_B;

    /* 2. Submode TCR_TLR part 1/2 to access TLR*/
    efrValue = UART_EFR(base);
    UART_EFR(base) |= UART_EFR_ENHANCEDEN;

    /* 3. Switch to configuration mode A to access MCR */
    UART_LCR(base) = REG_CONFIG_MODE_A;

    /* 4. Submode TCR_TLR part 2/2 to access TLR */
    mcrValue = UART_MCR(base);
    UART_MCR(base) |= UART_MCR_TCRTLR;

    /* 5. Write FCR */
    UART_FCR(base) = fcrValue;

    /* 6. Configuration mode B to access EFR */
    UART_LCR(base) = REG_CONFIG_MODE_B;

    /* 7. Write TLR */
    UART_TLR(base) = tlrValue;

    /* 8. Write SCR */
    UART_SCR(base) = scrValue;

    /* 9. Restore EFR */
    UART_EFR(base) = efrValue;

    /* 10. Switch to configuration mode A to access MCR */
    UART_LCR(base) = REG_CONFIG_MODE_A;

    /* 11. Restore MCR */
    UART_MCR(base) = mcrValue;

    /* 12. Restore LCR */
    UART_LCR(base) = lcrValue;
}

/*
 * uartBaudConfig()
 *
 * Ridiclous steps to set baud rate and line control.
 * s19.4.1.1.3 of am355x_TRM
 */
static void uartBaudConfig(uint32_t base, uartCfg_t *cfg)
{
    uint32_t baudDiv = baud2Div[cfg->baud];
    uint32_t efrValue;

    UART_MDR1(base) = UART_MDR1_MODESELECT(UART_MDR1_MODE_DISABLE);

    /* Crap to access DLL/DLH registers */
    UART_LCR(base) = REG_CONFIG_MODE_B;
    efrValue = UART_EFR(base);
    UART_EFR(base) |= UART_EFR_ENHANCEDEN;
    UART_LCR(base) = 0x0;
    UART_IER(base) = 0x0;
    UART_LCR(base) = REG_CONFIG_MODE_B;

    /* Set Baud Divisors */
    UART_DLL(base) = UART_DLL_CLOCK_LSB(baudDiv);
    UART_DLH(base) = UART_DLH_CLOCK_MSB(baudDiv);

    /* Crap to access IER */
    UART_LCR(base) = 0x0;

    /* No Interrupts */
    UART_IER(base) = 0x0;

    /* Crap to program LCR */
    UART_LCR(base) = REG_CONFIG_MODE_B;
    UART_EFR(base) = efrValue;

    /* 8-bit words, no parity, 1 stop bit */
    UART_LCR(base) = UART_LCR_CHAR_LENGTH(0x3);

    UART_MDR1(base) = UART_MDR1_MODESELECT(UART_MDR1_MODE_16X);
}

int uartConfig(uint32_t inst, uartCfg_t *cfg)
{
    uint32_t base = inst2Base[inst];

    LIMIT_HI_VAL(cfg->baud, BAUD_230400);

    /* Enable UART Clock */
    switch (inst) {
    case UART_0:
        CM_MODULEMODE_ENABLE (CM_WKUP_UART0_CLKCTRL);
        while (!(CM_WKUP_CLKSTCTRL & CM_CLKACTIVITY_UART0_GFCLK))
            ;
        CM_MODULE_IDLEST_FUNC(CM_WKUP_UART0_CLKCTRL);
        break;
    default:
        return ERROR;
    }

    /* Pin Multiplexing */
    switch (inst) {
    case UART_0:
        CTRLM_CONF_UART0_RXD = CTRLM_CONF_MUXMODE(0)
                             | CTRLM_CONF_RXACTIVE;
        CTRLM_CONF_UART0_TXD = CTRLM_CONF_MUXMODE(0);
        break;
    case UART_1:
        CTRLM_CONF_UART1_RXD = CTRLM_CONF_MUXMODE(0)
                             | CTRLM_CONF_RXACTIVE;
        CTRLM_CONF_UART1_TXD = CTRLM_CONF_MUXMODE(0);
        break;
    default:
        return ERROR;
    }

    uartFifoConfig(base, cfg);
    uartBaudConfig(base, cfg);

    return OK;
}

int uartWrite(uint32_t inst, uint8_t *data, uint32_t len)
{
    uint32_t base = inst2Base[inst];
    uint32_t txLen;

    for (txLen = 0; txLen < len; txLen++) {
        uint32_t retry = 100;
        /* Wait for TXFIFO to be empty */
        while (!(UART_LSR(base) & UART_LSR_TXSRE) && retry--)
            ;

        if (retry)
            UART_THR(base) = *data++;
        else
            break;
    }

    return txLen;
}

int uartRead(uint32_t inst, uint8_t *data, uint32_t len)
{
    uint32_t base = inst2Base[inst];
    uint32_t rxLen;

    for (rxLen = 0; rxLen < len; rxLen++) {
        uint32_t retry = 100;
        /* Wait for RXFIFO to have data */
        while (!(UART_LSR(base) & UART_LSR_RXFIFOE) && retry--)
            ;

        if (retry)
            *data++ = UART_RHR(base);
        else
            break;
    }

    return rxLen;
}

void uartPuts(char *str)
{
    char *cr = "\n\r";
    int len;
    for (len = 0; len < 80; len++) {
        if (str[len] == '\0')
            break;
    }
    uartWrite(UART_CONSOLE, (uint8_t *)str, len);
    uartWrite(UART_CONSOLE, (uint8_t *)cr, 2);
}

