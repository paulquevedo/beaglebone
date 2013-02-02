/*******************************************************************************
 *
 * xmodem.h
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *******************************************************************************/
#ifndef __XMODEM_H__
#define __XMODEM_H__
#include "hardware.h"

typedef struct {
    int32_t   uartFd;
    int32_t   numRetries;
} xmodemCfg_t;

extern int32_t xmodemInit(xmodemCfg_t *cfg);
extern int32_t xmodemAbort(void);
extern int32_t xmodemRecv(uint8_t *outBuffer, uint32_t numBytes);
#endif
