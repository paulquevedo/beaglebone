/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs                                   */
/*-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "globalDefs.h"
#include "am335x.h"
#include "sdhc.h"

#include "diskio.h"		/* FatFs lower layer API */

enum {
    DRIVE_SDHC_0,

    MAX_DRIVES,
};
typedef struct {
    uint32_t hwInst;
    bool32_t initialized;
    void *devCtx;
} fatdev_t;
static fatdev_t fatdev[MAX_DRIVES] = {
    [DRIVE_SDHC_0] = { .hwInst = SDHC_0 },
};

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE drv)
{
    DSTATUS status = 0;

    if (drv >= MAX_DRIVES)
        return STA_NODISK;

    switch (drv) {
    case DRIVE_SDHC_0:
        if (fatdev[drv].initialized)
            break;

        if (fatdev[drv].devCtx == 0)
            fatdev[drv].devCtx  = calloc(1, sizeof(sdhcCard_t *));

        if (fatdev[drv].devCtx) {
            sdhcCard_t *card = fatdev[drv].devCtx;

            card->inst = fatdev[drv].hwInst;
            if (sdhcInit(card->inst) == ERROR)
                status = STA_NOINIT;
            if (sdhcOpen(card) == ERROR)
                status = STA_NOINIT;

            fatdev[drv].initialized = TRUE;
        } else {
            status = STA_NOINIT;
        }
        break;
    default:
        status = STA_NOINIT;
        break;
    }

    return status;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE drv)
{
    DSTATUS status = 0;

    switch (drv) {
    case DRIVE_SDHC_0:
        if (!fatdev[drv].initialized)
            status = STA_NOINIT;
        else if (!sdhcCardPresent(fatdev[drv].hwInst))
            status = STA_NOINIT;
        break;
    default:
        status = STA_NOINIT;
        break;
    }

    return status;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, BYTE count)
{
    DRESULT result = RES_OK;

    switch (drv) {
    case DRIVE_SDHC_0:
        if (fatdev[drv].initialized) {
            sdhcCard_t *card = fatdev[drv].devCtx;

            while (count--) {
                if (sdhcReadBlock(card, sector, (uint32_t *)buff) == ERROR) {
                    result = RES_ERROR;
                    break;
                }
                buff += card->blkLen;
            }
        }
        else {
            result = RES_ERROR;
        }
        break;
    default:
        result = RES_ERROR;
        break;
    }

    return result;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, BYTE count)
{
    return RES_ERROR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
#if _USE_IOCTL
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
    return RES_OK;
}
#endif
