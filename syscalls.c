/*******************************************************************************
 *
 * syscalls.c
 *
 * Refer to http://neptune.billgatliff.com/newlib.html as a reference on
 * porting newlib
 *
 * Copyright (C) 2013 Paul Quevedo
 *
 * This program is free software.  It comes without any warranty, to the extent
 * permitted by applicable law.  You can redistribute it and/or modify it under
 * the terms of the WTF Public License (WTFPL), Version 2, as published by
 * Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 *******************************************************************************/
#include <sys/stat.h>
#include "globalDefs.h"
#include "hardware.h"

void *_sbrk_r(void *reent, int size)
{
    extern char _heap_start;  /* from linkerscript */
    extern char _heap_end;
    static char *brk = &_heap_start;
    char *prevBrk;

    prevBrk = brk;
    if ((brk + size) > (char *) &_heap_end) {
        uartPuts("Heap Overflow");
        return (void *) -1;  /* out of memory */
    }
    brk += size;
    return (void *) prevBrk;
}


int _fstat_r(void *reent, int fd, struct stat *st)
{
    /* Character device */
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty_r(void *reent, int fd)
{
    return 1;
}

int _lseek_r(void *reent, int fd, int ptr, int dir)
{
    return 0;
}

int _write_r(void *reent, int fd, char *ptr, int len)
{
    return (uartWrite(UART_CONSOLE, (uint8_t *)ptr, (uint32_t) len));
}

int _close_r(int fd)
{
    return -1;
}

int _read_r(int fd, char *ptr, int len)
{
    return (uartRead(UART_CONSOLE, (uint8_t *)ptr, (uint32_t)len));
}
