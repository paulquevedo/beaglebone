################################################################################
#
# Makefile for the TI AM355x/Beaglebone Bootloader
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

# Name of project/output file:

TARGET = app

# List your asm files here (minus the .s):

ASM_PIECES = start

# List your c files here (minus the .c):

C_PIECES  = hardware
C_PIECES += gpio uart syscalls
C_PIECES += sdhc ff diskio

# Define Hardware Platform
PROCESSOR  = AM335X
START_ADDR = 0x80000000 #Must match linkerscript origin addr
BOOT_MODE  = MMCSD

SOURCERY = /opt/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_EABI
STARTERWARE = ../StarterWare
FATFS = fatfs
PATH :=/opt/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_EABI/bin:${PATH}
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
GDB = arm-none-eabi-gdb
SIZE = arm-none-eabi-size
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy
TI_IMAGE = ${STARTERWARE}/tools/ti_image/tiimage

OBJDIR = ${TARGET}_obj
CHIBIOS_DIR 	 = ./ChibiOS
CHIBIOS_PORT_DIR = ./port_chibios

INCLUDES   = -I${FATFS}/ -I.
INCLUDES  += -I${CHIBIOS_DIR}/os/kernel/include
INCLUDES  += -I${CHIBIOS_PORT_DIR}

CPU_FLAGS  = -mcpu=cortex-a8 -mlong-calls -mno-thumb-interwork -marm
CPU_FLAGS += -ffunction-sections -falign-functions=16

ASM_FLAGS = -Wall -c -D${PROCESSOR} ${INCLUDES}
ASM_FILES = ${ASM_PIECES:%=%.S}
ASM_O_FILES = ${ASM_FILES:%.S=${OBJDIR}/%.o}

C_FLAGS = -Wall -Wno-format -c -D${PROCESSOR} ${INCLUDES}
ifeq ($(DEBUG), VERBOSE)
C_FLAGS += -g3 -O0 -DDEBUG=1
else
C_FLAGS += -g -O1
endif
C_FILES  = ${C_PIECES:%=%.c}
C_O_FILES = ${C_FILES:%.c=${OBJDIR}/%.o}

O_FILES = ${ASM_O_FILES} ${C_O_FILES}

LD_SCRIPT = linkerscript.ld

# nostartfiles prevents the toolchain from including startup routines.
LD_FLAGS = -nostartfiles -Map=${TARGET}.map

LIBS  = libChibi.a
LIBS += ${SOURCERY}/arm-none-eabi/lib/libc.a
LIBS += ${SOURCERY}/lib/gcc/arm-none-eabi/4.7.2/libgcc.a

all: ${TARGET}.axf
	@${OBJDUMP} -DS ${TARGET}.axf >| ${TARGET}.out.s
	@${OBJCOPY} -Obinary ${TARGET}.axf ${TARGET}.bin
	@${TI_IMAGE} ${START_ADDR} ${BOOT_MODE} ${TARGET}.bin app
	@ln -fs ${TARGET}.axf out.axf
	@echo
	@echo Executable: ${TARGET}.axf, sym-linked to out.axf
	@echo
	@echo Disassembly Listing: ${TARGET}.out.s, sym-linked to out.s
	@echo
	@${SIZE} ${TARGET}.axf
	@echo
	@${CC} --version

${TARGET}.axf: ${OBJDIR} ${O_FILES}
	@echo
	${LD} ${O_FILES} ${LIBS} -T ${LD_SCRIPT} ${LD_FLAGS} -o ${TARGET}.axf

${OBJDIR}/%.o: %.S
	${CC} ${ASM_FLAGS} ${CPU_FLAGS} -o $@ $<

${OBJDIR}/%.o: %.c
	${CC} ${C_FLAGS} ${CPU_FLAGS} -o $@ -c $<

${OBJDIR}/%.o: ${FATFS}/%.c
	${CC} ${C_FLAGS} ${CPU_FLAGS} -o $@ -c $<

${OBJDIR}:
	mkdir ${OBJDIR}

clean:
	@echo
	@echo Cleaning up...
	@echo
	rm -rf ${OBJDIR}
	rm -f ${TARGET}.axf
	rm -f ${TARGET}.out.s
	rm -f ${TARGET}.bin
	rm -f ${TARGET}.map
	rm -f out.axf
	rm -f app

openocd:
	@echo
	@echo Launching openOCD...
	@echo
	@openocd -s /usr/local/share/openocd/scripts -f board/ti_beaglebone.cfg

gdb:
	@echo
	@echo Launching GDB...
	@echo
	${GDB} --eval-command="target remote localhost:3333" out.axf

