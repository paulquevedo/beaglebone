################################################################################
#
# Makefile for library file of ChibiOS running on an ARM Cortex-A8
#
# Copyright (C) 2013 Paul Quevedo
#
# This program is free software.  It comes without any warranty, to the extent
# permitted by applicable law.  You can redistribute it and/or modify it under
# the terms of the WTF Public License (WTFPL), Version 2, as published by
# Sam Hocevar.  See http://sam.zoy.org/wtfpl/COPYING for more details.
#
#
################################################################################

CHIBIOS_DIR = ./ChibiOS/os/kernel
CHIBIOS_PORT_DIR = ./port_chibios

# Name of project/output file:

TARGET = libChibi
OBJDIR = libChibi_obj

# Port Pieces

ASM_PIECES = chcoreasm

C_PIECES   = chcore

# Kernel Pieces

C_PIECES += chsys chdebug chlists chvt chschd chthreads chdynamic chregistry
C_PIECES += chsem chmtx chcond chevents chmsg chmboxes chqueues chmemcore
C_PIECES += chheap chmempools

# Define Hardware Platform

PROCESSOR = AM335X

SOURCERY = /opt/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_EABI
PATH := $(SOURCERY)/bin:${PATH}
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
GDB = arm-none-eabi-gdb
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size
AR = arm-none-eabi-ar

INCLUDES  = -I.
INCLUDES += -I${CHIBIOS_DIR}/include
INCLUDES += -I${CHIBIOS_PORT_DIR}

CPU_FLAGS  = -mcpu=cortex-a8
CPU_FLAGS += -ffunction-sections -fdata-sections
CPU_FLAGS += -fomit-frame-pointer -falign-functions=16
CPU_FLAGS += -mno-thumb-interwork -marm

ASM_FLAGS = -Wall -c -D${PROCESSOR} ${INCLUDES}
ASM_FILES = ${ASM_PIECES:%=%.S}
ASM_O_FILES = ${ASM_FILES:%.S=${OBJDIR}/%.o}

C_FLAGS = -Wall -c -g -O1 -D${PROCESSOR} ${INCLUDES}
C_FLAGS += -fno-common -Wextra -Wstrict-prototypes -mlong-calls
C_FILES = ${C_PIECES:%=%.c}
C_O_FILES = ${C_FILES:%.c=${OBJDIR}/%.o}

O_FILES = ${ASM_O_FILES} ${C_O_FILES}

LD_SCRIPT = linkerscript.ld

# nostartfiles prevents the toolchain from including startup routines.
LD_FLAGS = -nostartfiles -Map=${TARGET}.map

LIBS  = ${SOURCERY}/arm-none-eabi/lib/libc.a
LIBS += ${SOURCERY}/lib/gcc/arm-none-eabi/4.7.2/libgcc.a

all: ${TARGET}.a

${TARGET}.a: ${OBJDIR} ${O_FILES}
	@echo
	rm -f $@
	${AR} crs $@ ${O_FILES}

${OBJDIR}/%.o: ${CHIBIOS_PORT_DIR}/%.S
	${CC} ${ASM_FLAGS} ${CPU_FLAGS} -o $@ $<

${OBJDIR}/%.o: ${CHIBIOS_PORT_DIR}/%.c
	${CC} ${C_FLAGS} ${CPU_FLAGS} -o $@ $<

${OBJDIR}/%.o: ${CHIBIOS_DIR}/src/%.c
	${CC} ${C_FLAGS} ${CPU_FLAGS} -o $@ $<

${OBJDIR}:
	mkdir ${OBJDIR}

clean:
	@echo
	@echo Cleaning up...
	@echo
	rm -rf ${OBJDIR}
	rm -f ${TARGET}.a
