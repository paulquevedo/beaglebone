#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globalDefs.h"
#include "am335x.h"
#include "hardware.h"
#include "sdhc.h"

/* SDPHY_SPEC: Part_1 SD Physical Layer Simplified Specification v3.01 */
#define CLK_INPUT_FREQ (PER_CLKOUTM2 / 2)
#define CLK_INIT_FREQ  400000    /* SDPHY_SPEC: s4.2.1 */
#define CLK_SD_FREQ    24000000  /* 25MHz doesn't divide evenly. Use 24MHz */
#define CLK_SDHS_FREQ  48000000  /* 50MHz doesn't divide evenly. Use 48MHz */

static uint32_t inst2Base[] = {
    [SDHC_0] = MMC0_BASE_ADDR,
};

enum {
    CMDTYPE_NORMAL,
    CMDTYPE_SUSPEND,
    CMDTYPE_FUNCSEL,
    CMDTYPE_ABORT,
};

enum {
    RSPTYPE_NONE,
    RSPTYPE_48BIT,
    RSPTYPE_48BIT_BUSY,
    RSPTYPE_136BIT,
};

enum {
    XFER_FLAG_CICE,      /* Command Index in response field */
    XFER_FLAG_CCCE,      /* CRC7 in response field */
    XFER_FLAG_DATA_READ, /* Expect data on DAT line(s) */
};

enum {
    SD_VER_1_00,
    SD_VER_1_10,
    SD_VER_2_00,
};

typedef struct {
    uint8_t  cmdIdx;
    uint8_t  cmdType;
    uint8_t  rspType;
    uint8_t  xferFlags;
    uint32_t cmdArg;
    uint32_t nBlks;
    uint32_t blkSize;
    uint32_t resp[4];
} sdhcCmd_t;

/*****************************************************************************
 *****************************************************************************
 ************************ COMMAND DESCRIPTORS  *******************************
 *****************************************************************************
 ****************************************************************************/

static sdhcCmd_t cmd0 = {
    .cmdIdx    = 0,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_NONE,
    .xferFlags = 0,
    .cmdArg    = 0,
    .nBlks     = 0,
};
static sdhcCmd_t cmd2 = {
    .cmdIdx    = 2,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_136BIT,
    .xferFlags = 0,
    .cmdArg    = 0,
    .nBlks     = 0,
};
static sdhcCmd_t cmd3 = {
    .cmdIdx    = 3,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT,
    .xferFlags = XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0,
    .nBlks     = 0,
};
#if 0
static sdhcCmd_t cmd6 = {
    .cmdIdx    = 6,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT,
    .xferFlags = XFER_FLAG_DATA_READ | XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0x80FFFFF1, /* HS Mode SDPHY_SPEC s4.3.10 */
    .nBlks     = 1,
    .blkSize   = 512,
};
#endif
static sdhcCmd_t acmd6 = {
    .cmdIdx    = 6,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT_BUSY,
    .xferFlags = XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0,
    .nBlks     = 0,
};
static sdhcCmd_t cmd7 = {
    .cmdIdx    = 7,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT_BUSY,
    .xferFlags = XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0,
    .nBlks     = 0,
};
static sdhcCmd_t cmd8 = {
    .cmdIdx    = 8,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT,
    .xferFlags = XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0x1aa, /* SDPHY_SPEC: s4.3.13 */
    .nBlks     = 0,
};
static sdhcCmd_t cmd9 = {
    .cmdIdx    = 9,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_136BIT,
    .xferFlags = 0,
    .cmdArg    = 0,
    .nBlks     = 0,
};
static sdhcCmd_t cmd17 = {
    .cmdIdx    = 17,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT,
    .xferFlags = XFER_FLAG_DATA_READ | XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0,
    .nBlks     = 1,
    .blkSize   = 512,
};
static sdhcCmd_t acmd41 = {
    .cmdIdx    = 41,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT,
    .xferFlags = 0,
    .cmdArg    = 0,
    .nBlks     = 0,
};
static sdhcCmd_t acmd51 = {
    .cmdIdx    = 51,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT,
    .xferFlags = XFER_FLAG_DATA_READ | XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0,
    .nBlks     = 1,
    .blkSize   = 8,
};
static sdhcCmd_t cmd55 = {
    .cmdIdx    = 55,
    .cmdType   = CMDTYPE_NORMAL,
    .rspType   = RSPTYPE_48BIT,
    .xferFlags = XFER_FLAG_CICE | XFER_FLAG_CCCE,
    .cmdArg    = 0,
    .nBlks     = 0,
};

/*****************************************************************************
 *****************************************************************************
 ************************ HELPER FUNCTIONS ***********************************
 *****************************************************************************
 ****************************************************************************/

/*****************************************************************************
 * parseCsd()
 *
 *  Parses the Card-Specific Data register
 *
 *****************************************************************************/
static void parseCsd(sdhcCard_t *card)
{
    uint32_t value;
    /* SDPHY_SPEC s5.3.2,3 */

    value = card->csd[3] & 0x7; /* Transfer unit rate */
    switch (value) {
    default: card->transSpeed = 0;        break;
    case 3:  card->transSpeed = 10000000; break;
    case 2:  card->transSpeed = 1000000;  break;
    case 1:  card->transSpeed = 100000;   break;
    case 0:  card->transSpeed = 10000;    break;
    }

    value = (card->csd[3] >> 3) & 0xf; /* Time Value */
    switch (value) {
    case 0x1: card->transSpeed *= 10; break;
    case 0x2: card->transSpeed *= 12; break;
    case 0x3: card->transSpeed *= 13; break;
    case 0x4: card->transSpeed *= 15; break;
    case 0x5: card->transSpeed *= 20; break;
    case 0x6: card->transSpeed *= 25; break;
    case 0x7: card->transSpeed *= 30; break;
    case 0x8: card->transSpeed *= 35; break;
    case 0x9: card->transSpeed *= 40; break;
    case 0xa: card->transSpeed *= 45; break;
    case 0xb: card->transSpeed *= 50; break;
    case 0xc: card->transSpeed *= 55; break;
    case 0xd: card->transSpeed *= 60; break;
    case 0xe: card->transSpeed *= 70; break;
    case 0xf: card->transSpeed *= 80; break;
    }

    card->blkLen = 1 << ((card->csd[2] >> 16) & 0xf);

    /* CSD Version 2.0 */
    if ((card->csd[3] >> 30) == 0x1) {
        value  = (card->csd[2] & 0x3f) << 16;
        value |= (card->csd[1] >> 16);
        card->size = (value + 1) * 512 * 1024;
        card->numBlks = card->size / 512;
    }
    /* CSD Version 1.0 */
    else {
        uint32_t mult = (card->csd[1] >> 15) & 0x3; /* C_SIZE_MULT */
        mult   = 1 << (mult + 2);
        value  = ((card->csd[1] >> 30) & 0x3);
        value |= ((card->csd[2] & 0x3ff) << 2);
        card->numBlks = (value + 1) * mult;
        card->size = card->numBlks * card->blkLen;
    }
#if DEBUG
    iprintf("SDHC Trans Speed: %lu\n\r", card->transSpeed);
    iprintf("SDHC Block Size:  %lu\n\r", card->blkLen);
    iprintf("SDHC Num Blocks:  %lu\n\r", card->numBlks);
    iprintf("SDHC Card Size:   %lu\n\r", card->size);
#endif
}

/*****************************************************************************
 * parseScr()
 *
 *  Parses the SD Configuration Register
 *
 *****************************************************************************/
static void parseScr(sdhcCard_t *card)
{
    uint32_t value  = (card->scr[0] >> 16) & 0xf;
    card->busWidth  = (value & BIT_2) ? 4 : 1;
    card->sdVersion = (card->scr[0] >> 24) & 0xf;

#if DEBUG
    iprintf("SDHC SD Version: %d\n\r", card->sdVersion);
    iprintf("SDHC Bus width:  %d bits\n\r", card->busWidth);
#endif
}

/*****************************************************************************
 * sdhcSendCmd()
 *
 *  Triggers a command transmission
 *
 *****************************************************************************/
static int sdhcSendCmd(uint32_t inst, sdhcCmd_t *cmd)
{
    uint32_t cmdReg = SD_CMD_INDX(cmd->cmdIdx);
    uint32_t base = inst2Base[inst];

    switch (cmd->cmdType) {
    case CMDTYPE_SUSPEND: cmdReg |= SD_CMD_CMD_TYPE(1); break;
    case CMDTYPE_FUNCSEL: cmdReg |= SD_CMD_CMD_TYPE(2); break;
    case CMDTYPE_ABORT:   cmdReg |= SD_CMD_CMD_TYPE(3); break;
    }

    switch (cmd->rspType) {
    case RSPTYPE_136BIT:     cmdReg |= SD_CMD_RSP_TYPE(1); break;
    case RSPTYPE_48BIT:      cmdReg |= SD_CMD_RSP_TYPE(2); break;
    case RSPTYPE_48BIT_BUSY: cmdReg |= SD_CMD_RSP_TYPE(3); break;
    }

    if (cmd->xferFlags & XFER_FLAG_CICE)
        cmdReg |= SD_CMD_CICE;
    if (cmd->xferFlags & XFER_FLAG_CCCE)
        cmdReg |= SD_CMD_CCCE;
    if (cmd->xferFlags & XFER_FLAG_DATA_READ)
        cmdReg |= SD_CMD_DDIR;
    if (cmd->nBlks)
        cmdReg |= SD_CMD_DP;

    SD_STAT(base) = 0xffffffff; /* Clear all status bits */

    SD_BLK(base) = SD_BLK_NBLK(cmd->nBlks);
    if (cmd->nBlks) {
        SD_BLK(base) |= SD_BLK_BLEN(cmd->blkSize);
        SD_SYSCTL(base) |= SD_SYSCTL_DTO(0xe); /* max timeout value */
    }

    SD_ARG(base) = cmd->cmdArg;
    SD_CMD(base) = cmdReg;

    while (!(SD_STAT(base) & (SD_STAT_ERRI | SD_STAT_CC)))
        ;

    if (SD_STAT(base) & SD_STAT_ERRI) {
        uartPuts("SDHC Cmd Error");
#if DEBUG
        iprintf("   Cmd %d Err %x\n\r", cmd->cmdIdx, SD_STAT(base));
#endif
        SD_STAT(base) |= SD_STAT_ERROR_BITS;
        memset(cmd->resp, 0, 16);
        return ERROR;
    }
    else {
        SD_STAT(base) |= SD_STAT_CC;

        cmd->resp[0] = SD_RSP10(base);
        cmd->resp[1] = SD_RSP32(base);
        cmd->resp[2] = SD_RSP54(base);
        cmd->resp[3] = SD_RSP76(base);
    }

#if DEBUG
    iprintf("SDHC Cmd %d Rsp %08x %08x %08x %08x\n\r", cmd->cmdIdx,
                        cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
#endif

    return OK;
}

/*****************************************************************************
 *****************************************************************************
 ********************* INTERFACE FUNCTIONS ***********************************
 *****************************************************************************
 ****************************************************************************/

/*****************************************************************************
 * sdhcCardPresent()
 *
 *  Returns whether an SD Card is current inserted
 *
 *****************************************************************************/
bool32_t sdhcCardPresent(uint32_t inst)
{
    uint32_t base = inst2Base[inst];
    bool32_t present;

    if (SD_PSTATE(base) & SD_PSTATE_CINS)
        present = TRUE;
    else
        present = FALSE;

#if DEBUG
    iprintf("SDHC Card present? %d\n\r", present);
#endif
    return present;
}

/*****************************************************************************
 * sdhcInit()
 *
 *  Sets up the MMC/SD Hardware controller as per s18.4 of the am335x TRM
 *
 *****************************************************************************/
int32_t sdhcInit(uint32_t inst)
{
    uint32_t base = inst2Base[inst];

    /* Enable clock */
    switch (inst) {
    case SDHC_0:
        CM_MODULEMODE_ENABLE(CM_PER_MMC0_CLKCTRL);
        CM_MODULE_IDLEST_FUNC(CM_PER_MMC0_CLKCTRL);
        break;
    default:
        return ERROR;
    }

    /* Pin Mux, All TX/RX w Pull Ups */
    switch (inst) {
    case SDHC_0:
        CTRLM_CONF_MMC0_CLK  = CTRLM_CONF_MUXMODE(0) | CTRLM_CONF_RXACTIVE
                                                     | CTRLM_CONF_PUTYPESEL;
        CTRLM_CONF_MMC0_CMD  = CTRLM_CONF_MUXMODE(0) | CTRLM_CONF_RXACTIVE
                                                     | CTRLM_CONF_PUTYPESEL;
        CTRLM_CONF_MMC0_DAT0 = CTRLM_CONF_MUXMODE(0) | CTRLM_CONF_RXACTIVE
                                                     | CTRLM_CONF_PUTYPESEL;
        CTRLM_CONF_MMC0_DAT1 = CTRLM_CONF_MUXMODE(0) | CTRLM_CONF_RXACTIVE
                                                     | CTRLM_CONF_PUTYPESEL;
        CTRLM_CONF_MMC0_DAT2 = CTRLM_CONF_MUXMODE(0) | CTRLM_CONF_RXACTIVE
                                                     | CTRLM_CONF_PUTYPESEL;
        CTRLM_CONF_MMC0_DAT3 = CTRLM_CONF_MUXMODE(0) | CTRLM_CONF_RXACTIVE
                                                     | CTRLM_CONF_PUTYPESEL;
        /* MMC0_SDCD Card Detect Line */
        CTRLM_CONF_SPI0_CS1  = CTRLM_CONF_MUXMODE(5) | CTRLM_CONF_RXACTIVE
                                                     | CTRLM_CONF_PUTYPESEL;
        break;
    default:
        return ERROR;
    }

    if (!sdhcCardPresent(inst)) {
#if DEBUG
        iprintf("SDHC No Card Detected\n\r");
#endif
        return ERROR;
    }

    /* s18.4.2.2 Soft Reset */
    SD_SYSCONFIG(base) |= SD_SYSCONFIG_SOFTRESET;
    while(!(SD_SYSSTATUS(base) & SD_SYSSTATUS_RESETDONE))
        ;

    /* Reset Data Lines */
    SD_SYSCTL(base) |= SD_SYSCTL_SRA;
    while (SD_SYSCTL(base) & SD_SYSCTL_SRA)
        ;
    /* s18.4.2.3 Set SD Default Capabilities */
    SD_CAPA(base) = SD_CAPA_VS18 | SD_CAPA_VS33; /* Support 1V8 and 3V3 */

    SD_SYSCONFIG(base) = SD_SYSCONFIG_STANDBYMODE(0x1)
                       | SD_SYSCONFIG_CLOCKACTIVITY(0x1)
                       | SD_SYSCONFIG_AUTOIDLE;

    /* s18.4.2.5 */
    /* 1 Bit interface */
    SD_CON(base)  &= ~SD_CON_DW8;
    SD_HCTL(base) &= ~SD_HCTL_DTW;

    /* Bus voltage. Limitations on SDHC1/2 defined in trm register map */
    switch (inst) {
    case SDHC_0:
        SD_HCTL(base) |= SD_HCTL_SDVS(0x7); /* 3.3V */
        break;
    case SDHC_1:
    case SDHC_2:
        SD_HCTL(base) &= ~SD_HCTL_SDVS(0x7);
        SD_HCTL(base) |=  SD_HCTL_SDVS(0x5); /* 1.8V */
        break;
    }
    /* Power on */
    SD_HCTL(base) |= SD_HCTL_SDBP;
    if (!(SD_HCTL(base) & SD_HCTL_SDBP)) {
        uartPuts("SDHC Bus Power Enable Failed");
        return ERROR;
    }
    /* Internal Clock Enable */
    SD_SYSCTL(base) |= SD_SYSCTL_ICE;

    /* Set clock to init frequency, wait for stable */
    SD_SYSCTL(base) |= SD_SYSCTL_CLKD(CLK_INPUT_FREQ / CLK_INIT_FREQ);
    while (!(SD_SYSCTL(base) & SD_SYSCTL_ICS))
        ;

    /* Enable clock to the card */
    SD_SYSCTL(base) |= SD_SYSCTL_CEN;

    /* Enable + clear the CC status bit, (Note this doesn't enable IRQ) */
    SD_IE(base)    = SD_IE_CC;
    SD_STAT(base) |= SD_STAT_CC;

    /* Send the 80 pulse init stream to the card */
    SD_CON(base)  |= SD_CON_INIT;
    SD_CMD(base)   = 0x0;
    while (!(SD_STAT(base) & SD_STAT_CC))
        ;
    SD_CON(base) &= ~SD_CON_INIT;
    SD_STAT(base) = 0xffffffff; /* Clear all status bits */

    /* Enable required status bits + All Error bits */
    SD_IE(base) = SD_IE_CC  | SD_IE_TC  | SD_IE_BRR | SD_IE_BWR
                | SD_IE_ERROR_BITS;

    return OK;
}


/*****************************************************************************
 * sdhcOpen()
 *
 *  Sets up a card in the transfer state
 *
 *****************************************************************************/
int32_t sdhcOpen(sdhcCard_t *card)
{
    uint32_t base = inst2Base[card->inst];
    int retry = 10;

    /* SDPHY_SPEC: s5.1 */
    enum {
        OCR_VDD_2V7_3V6    = (0x1ff << 15),
        OCR_HIGH_CAPACITY  = BIT_30,
        OCR_CARD_READY     = BIT_31,
    };

    /* Sanity check to ensure SD card, must support cmd 55 */
    sdhcSendCmd(card->inst, &cmd0);
    if (sdhcSendCmd(card->inst, &cmd55) == ERROR) {
        uartPuts("SDHC Init: Failed on cmd 55, Not an SD Card");
        return ERROR;
    }

    /* SDPHY_SPEC s4.2.3: Card identification mode */
    sdhcSendCmd(card->inst, &cmd0);
    if (sdhcSendCmd(card->inst, &cmd8) == ERROR) {
        uartPuts("SDHC Init: Failed on cmd 8, Voltage/Ver1.X not supported");
        return ERROR;
    }

    cmd55.cmdArg  = 0;
    acmd41.cmdArg = OCR_HIGH_CAPACITY | OCR_VDD_2V7_3V6;
    do {
        sdhcSendCmd(card->inst, &cmd55);
        sdhcSendCmd(card->inst, &acmd41);
    } while (!(acmd41.resp[0] & OCR_CARD_READY) && --retry);

    if (!retry) {
        uartPuts("SDHC Init: Cmd 41 Failed");
        return ERROR;
    }

    if (!(acmd41.resp[0] & OCR_HIGH_CAPACITY)) {
        uartPuts("SDHC Init: Standard Capacity not supported");
        return ERROR;
    }

    sdhcSendCmd(card->inst, &cmd2);
    memcpy(card->cid, cmd2.resp, 16);

    sdhcSendCmd(card->inst, &cmd3);
    card->rca = cmd3.resp[0] & 0xffff0000; /* SDPHY_SPEC s4.9.5 */

    /* SDPHY_SPEC s4.3: Now in data transfer mode, standby state */
    /* Retrieve card-specific data register CSD */
    cmd9.cmdArg = card->rca;
    sdhcSendCmd(card->inst, &cmd9);
    memcpy(card->csd, cmd9.resp, 16);

    /* Put card into the transfer state */
    cmd7.cmdArg = card->rca;
    sdhcSendCmd(card->inst, &cmd7);

    /* Retrieve SD Configuration register SCR - Data transfer */
    cmd55.cmdArg = card->rca;
    sdhcSendCmd(card->inst, &cmd55);
    acmd51.cmdArg = card->rca;
    sdhcSendCmd(card->inst, &acmd51);

    while(!(SD_STAT(base) & SD_STAT_BRR))
        ;
    SD_STAT(base) |= SD_STAT_BRR;

    card->scr[0] = SD_DATA(base);
    card->scr[1] = SD_DATA(base);

    swap32(card->scr[0]); /* DATA register is little endian */
    swap32(card->scr[1]);

    parseCsd(card);
    parseScr(card);

    if (card->busWidth == 4) {
        cmd55.cmdArg = card->rca;
        sdhcSendCmd(card->inst, &cmd55);
        acmd6.cmdArg = 0x2;
        sdhcSendCmd(card->inst, &acmd6);

        SD_HCTL(base) |=  SD_HCTL_DTW;  /* Enable 4-bit width */
    }

    /* TODO: Somethings not working here */
#if 0
    if (card->sdVersion >= SD_VER_1_10) { /* SDPHY_SPEC s4.3.10 */
        uint32_t value;
        uint32_t freq;

        sdhcSendCmd(card->inst, &cmd6);
        while (!(SD_STAT(base) & SD_STAT_BRR))
            ;
        SD_STAT(base) |= SD_STAT_BRR;

        int i;
        for (i = 0; i < cmd6.blkSize / 4; i++) {
            value = SD_DATA(base);
            if (i == 3) {
                swap32(value);
                break;
            }
        }

        if (1 || ((value >> 16) & 0xf) == 0x1) {  /* HS Supported */
            freq = CLK_SDHS_FREQ;
            SD_HCTL(base) |=  SD_HCTL_HSPE;
        }
        else {
            freq = CLK_SD_FREQ;
            SD_HCTL(base) &= ~SD_HCTL_HSPE;
        }

        value  = SD_SYSCTL(base) & ~SD_SYSCTL_CLKD_MASK;
        value |= SD_SYSCTL_CLKD(CLK_INPUT_FREQ / freq);
        SD_SYSCTL(base) = value;
        while (!(SD_SYSCTL(base) & SD_SYSCTL_ICS))
            ;
#if DEBUG
        iprintf("SDHC running at %dMBits\n\r", freq / 1000000);
#endif
    }
#else
    {
        uint32_t freq = CLK_SD_FREQ;
        uint32_t value;
        value  = SD_SYSCTL(base) & ~SD_SYSCTL_CLKD_MASK;
        value |= SD_SYSCTL_CLKD(CLK_INPUT_FREQ / freq);
        SD_SYSCTL(base) = value;
        SD_HCTL(base) &= ~SD_HCTL_HSPE;
        while (!(SD_SYSCTL(base) & SD_SYSCTL_ICS))
            ;
#if DEBUG
        iprintf("SDHC running at %dMBits\n\r", freq / 1000000);
#endif
    }
#endif

    return OK;
}

/*****************************************************************************
 * sdhcReadBlock()
 *
 *  Reads the given block
 *
 *****************************************************************************/
int32_t sdhcReadBlock(sdhcCard_t *card, uint32_t block, uint32_t *buffer)
{
    uint32_t base = inst2Base[card->inst];

    cmd17.cmdArg = block;
    sdhcSendCmd(card->inst, &cmd17);

#if 0
    iprintf("SDHC Read Block %d\n\r", block);
#endif

    while (1) {
        uint32_t status = SD_STAT(base);
        uint32_t value;
        int i;

        if (status & SD_STAT_BRR) {
            SD_STAT(base) |= SD_STAT_BRR;
            for (i = 0; i < cmd17.blkSize / 4; i++) {
                value = SD_DATA(base);
                *buffer++ = value;
#if 0
                iprintf("%08x ", value);
                if ((i + 1) % 4 == 0)
                    iprintf("\n\r");
#endif
            }
        }
        if (status & SD_STAT_TC) {
            SD_STAT(base) |= SD_STAT_TC;
            break;
        }
    }
    return OK;
}
