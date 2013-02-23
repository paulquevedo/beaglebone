/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "chconf.h"

#define FALSE 0
#define TRUE 1

.set    MODE_USR, 0x10
.set    MODE_FIQ, 0x11
.set    MODE_IRQ, 0x12
.set    MODE_SVC, 0x13
.set    MODE_ABT, 0x17
.set    MODE_UND, 0x1B
.set    MODE_SYS, 0x1F

.equ    I_BIT, 0x80
.equ    F_BIT, 0x40

.text

/*
 * Interrupt enable/disable functions, only present if there is THUMB code in
 * the system because those are inlined in ARM code.
 */

.balign 16
.code 32
.global _port_switch_arm
_port_switch_arm:
#ifdef CH_CURRP_REGISTER_CACHE
        stmfd   sp!, {r4, r5, r6, r8, r9, r10, r11, lr}
        str     sp, [r1, #12]
        ldr     sp, [r0, #12]
        ldmfd   sp!, {r4, r5, r6, r8, r9, r10, r11, pc}
#else
        stmfd   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
        str     sp, [r1, #12]
        ldr     sp, [r0, #12]
        ldmfd   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
#endif

/*
 * Common exit point for all IRQ routines, it performs the rescheduling if
 * required.
 * System stack frame structure after a context switch in the
 * interrupt handler:
 *
 * High +------------+
 *      |   LR_USR   | -+
 *      |     R12    |  |
 *      |     R3     |  |
 *      |     R2     |  | External context: IRQ handler frame
 *      |     R1     |  |
 *      |     R0     |  |
 *      |     PC     |  |   (user code return address)
 *      |   PSR_USR  | -+   (user code status)
 *      |    ....    | <- chSchDoReschedule() stack frame, optimize it for space
 *      |     LR     | -+   (system code return address)
 *      |     R11    |  |
 *      |     R10    |  |
 *      |     R9     |  |
 *      |     R8     |  | Internal context: chSysSwitch() frame
 *      |    (R7)    |  |   (optional, see CH_CURRP_REGISTER_CACHE)
 *      |     R6     |  |
 *      |     R5     |  |
 * SP-> |     R4     | -+
 * Low  +------------+
 */
.balign 16
.code 32
.globl _port_irq_common
_port_irq_common:
        bl      chSchIsPreemptionRequired
        cmp     r0, #0                          // Simply returns if a
        ldmeqfd sp!, {r0-r3, r12, lr}           // reschedule is not
        subeqs  pc, lr, #4                      // required.

        // Saves the IRQ mode registers in the system stack.
        ldmfd   sp!, {r0-r3, r12, lr}           // IRQ stack now empty.
        msr     CPSR_c, #MODE_SYS | I_BIT
        stmfd   sp!, {r0-r3, r12, lr}           // Registers on System Stack.
        msr     CPSR_c, #MODE_IRQ | I_BIT
        mrs     r0, SPSR
        mov     r1, lr
        msr     CPSR_c, #MODE_SYS | I_BIT
        stmfd   sp!, {r0, r1}                   // Push R0=SPSR, R1=LR_IRQ.

        // Context switch.
#if CH_DBG_SYSTEM_STATE_CHECK
        bl      dbg_check_lock
#endif
        bl      chSchDoReschedule
#if CH_DBG_SYSTEM_STATE_CHECK
        bl      dbg_check_unlock
#endif

        // Re-establish the IRQ conditions again.
        ldmfd   sp!, {r0, r1}                   // Pop R0=SPSR, R1=LR_IRQ.
        msr     CPSR_c, #MODE_IRQ | I_BIT
        msr     SPSR_fsxc, r0
        mov     lr, r1
        msr     CPSR_c, #MODE_SYS | I_BIT
        ldmfd   sp!, {r0-r3, r12, lr}
        msr     CPSR_c, #MODE_IRQ | I_BIT
        subs    pc, lr, #4

/*
 * Threads trampoline code.
 * NOTE: The threads always start in ARM mode and then switches to the
 * thread-function mode.
 */
.balign 16
.code 32
.globl _port_thread_start
_port_thread_start:
#if CH_DBG_SYSTEM_STATE_CHECK
        mov     r0, #0
        ldr     r1, =dbg_lock_cnt
        str     r0, [r1]
#endif
        msr     CPSR_c, #MODE_SYS
        mov     r0, r5
        mov     lr, pc
        bx      r4
        bl      chThdExit
