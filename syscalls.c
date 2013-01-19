/*******************************************************************************
* syscalls.c
*******************************************************************************/
#include <sys/stat.h>
#include "globalDefs.h"
#include "hardware.h"

void *_sbrk_r(void *reent, int size)
{
    return (void *) -1;
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
