/*******************************************************************************
 *
 * sdhc.h
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *******************************************************************************/
#ifndef __SDHC_H__
#define __SDHC_H__
#include "globalDefs.h"

enum {
    SDHC_0,
    SDHC_1,
    SDHC_2,
};

typedef struct {
    uint32_t inst;

    uint32_t rca;   /* Relative card address */
    uint16_t sdVersion;
    uint16_t busWidth;
    uint32_t transSpeed;
    uint32_t blkLen;
    uint32_t numBlks;
    uint32_t size;
    uint32_t cid[4];
    uint32_t csd[4];
    uint32_t scr[2];
} sdhcCard_t;

extern int32_t sdhcInit(uint32_t inst);
extern bool32_t sdhcCardPresent(uint32_t inst);
extern int32_t sdhcOpen(sdhcCard_t *card);
extern int32_t sdhcReadBlock (sdhcCard_t *card, uint32_t block,
                                                uint32_t *buffer);
extern int32_t sdhcWriteBlock(sdhcCard_t *card, uint32_t block,
                                                const uint32_t *buffer);
#endif
