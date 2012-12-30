/*******************************************************************************
*
* globalDefs.h
*
* Paul Quevedo 2012
*
*******************************************************************************/
#ifndef __GLOBALDEFS_H__
#define __GLOBALDEFS_H__

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;

typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed long    int32_t;

typedef unsigned char  bool8_t;
typedef unsigned short bool16_t;
typedef unsigned int   bool32_t;

#define ARRAY_SIZE (array) (sizeof(array) / sizeof(array[0]))
#define STR(s)  #s
#define XSTR(s) STR(s)

#ifdef  TRUE
#undef  TRUE
#endif
#define TRUE 1

#ifdef  FALSE
#undef  FALSE
#endif
#define FALSE 0

#ifdef  ERROR
#undef  ERROR
#endif
#define ERROR -1

#ifdef  OK
#undef  OK
#endif
#define OK 0

#define BIT_0  0x00000001
#define BIT_1  0x00000002
#define BIT_2  0x00000004
#define BIT_3  0x00000008
#define BIT_4  0x00000010
#define BIT_5  0x00000020
#define BIT_6  0x00000040
#define BIT_7  0x00000080
#define BIT_8  0x00000100
#define BIT_9  0x00000200
#define BIT_10 0x00000400
#define BIT_11 0x00000800
#define BIT_12 0x00001000
#define BIT_13 0x00002000
#define BIT_14 0x00004000
#define BIT_15 0x00008000
#define BIT_16 0x00010000
#define BIT_17 0x00020000
#define BIT_18 0x00040000
#define BIT_19 0x00080000
#define BIT_20 0x00100000
#define BIT_21 0x00200000
#define BIT_22 0x00400000
#define BIT_23 0x00800000
#define BIT_24 0x01000000
#define BIT_25 0x02000000
#define BIT_26 0x04000000
#define BIT_27 0x08000000
#define BIT_28 0x10000000
#define BIT_29 0x20000000
#define BIT_30 0x40000000
#define BIT_31 0x80000000

#endif

