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

#ifndef _CHCORE_H_
#define _CHCORE_H_

#define ARM_ENABLE_WFI_IDLE             FALSE

#define CH_ARCHITECTURE_ARM
#define CH_ARCHITECTURE_NAME            "CortexA"
#define CH_CORE_VARIANT_NAME            "CortexA8"
#define CH_PORT_INFO                    "Pure ARM"
#define CH_COMPILER_NAME                "GCC " __VERSION__

/*===========================================================================*/
/* Port implementation part (common).                                        */
/*===========================================================================*/

/**
 * @brief   32 bits stack and memory alignment enforcement.
 */
typedef uint32_t stkalign_t;

/**
 * @brief   Generic ARM register.
 */
typedef void *regarm_t;

/**
 * @brief   Interrupt saved context.
 * @details This structure represents the stack frame saved during a
 *          preemption-capable interrupt handler.
 */
struct extctx {
  regarm_t      spsr_irq;
  regarm_t      lr_irq;
  regarm_t      r0;
  regarm_t      r1;
  regarm_t      r2;
  regarm_t      r3;
  regarm_t      r12;
  regarm_t      lr_usr;
};

/**
 * @brief   System saved context.
 * @details This structure represents the inner stack frame during a context
 *          switching.
 */
struct intctx {
  regarm_t      r4;
  regarm_t      r5;
  regarm_t      r6;
  regarm_t      r7;
  regarm_t      r8;
  regarm_t      r9;
  regarm_t      r10;
  regarm_t      r11;
  regarm_t      lr;
};

/**
 * @brief   Platform dependent part of the @p Thread structure.
 * @details In this port the structure just holds a pointer to the @p intctx
 *          structure representing the stack pointer at context switch time.
 */
struct context {
  struct intctx *r13;
};

/**
 * @brief   Platform dependent part of the @p chThdCreateI() API.
 * @details This code usually setup the context switching frame represented
 *          by an @p intctx structure.
 */
#define SETUP_CONTEXT(workspace, wsize, pf, arg) {                          \
  tp->p_ctx.r13 = (struct intctx *)((uint8_t *)workspace +                  \
                                     wsize -                                \
                                     sizeof(struct intctx));                \
  tp->p_ctx.r13->r4 = pf;                                                   \
  tp->p_ctx.r13->r5 = arg;                                                  \
  tp->p_ctx.r13->lr = _port_thread_start;                                   \
}

/**
 * @brief   Stack size for the system idle thread.
 * @details This size depends on the idle thread implementation, usually
 *          the idle thread should take no more space than those reserved
 *          by @p PORT_INT_REQUIRED_STACK.
 * @note    In this port it is set to 4 because the idle thread does have
 *          a stack frame when compiling without optimizations.
 */
#define PORT_IDLE_THREAD_STACK_SIZE     16

/**
 * @brief   Per-thread stack overhead for interrupts servicing.
 * @details This constant is used in the calculation of the correct working
 *          area size.
 *          This value can be zero on those architecture where there is a
 *          separate interrupt stack and the stack space between @p intctx and
 *          @p extctx is known to be zero.
 * @note    In this port 0x10 is a safe value, it can be reduced after careful
 *          analysis of the generated code.
 */
#define PORT_INT_REQUIRED_STACK         0x10

/**
 * @brief   Enforces a correct alignment for a stack area size value.
 */
#define STACK_ALIGN(n) ((((n) - 1) | (sizeof(stkalign_t) - 1)) + 1)

/**
 * @brief   Computes the thread working area global size.
 */
#define THD_WA_SIZE(n) STACK_ALIGN(sizeof(Thread) +                         \
                                   sizeof(struct intctx) +                  \
                                   sizeof(struct extctx) +                  \
                                   (n) + (PORT_INT_REQUIRED_STACK))

/**
 * @brief   Static working area allocation.
 * @details This macro is used to allocate a static thread working area
 *          aligned as both position and size.
 */
#define WORKING_AREA(s, n) stkalign_t s[THD_WA_SIZE(n) / sizeof(stkalign_t)]

/**
 * @brief   IRQ prologue code.
 * @details This macro must be inserted at the start of all IRQ handlers
 *          enabled to invoke system APIs.
 */
#define PORT_IRQ_PROLOGUE() {}

/**
 * @brief   IRQ epilogue code.
 * @details This macro must be inserted at the end of all IRQ handlers
 *          enabled to invoke system APIs.
 */
#define PORT_IRQ_EPILOGUE() {}

/**
 * @brief   IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#define PORT_IRQ_HANDLER(id) void id(void)

/**
 * @brief   Fast IRQ handler function declaration.
 * @note    @p id can be a function name or a vector number depending on the
 *          port implementation.
 */
#define PORT_FAST_IRQ_HANDLER(id)                                           \
                             __attribute__((interrupt("FIQ"))) void id(void)

/**
 * @brief   Port-related initialization code.
 * @note    This function is empty in this port.
 */
#define port_init()

/**
 * @brief   Kernel-lock action.
 * @details Usually this function just disables interrupts but may perform
 *          more actions.
 * @note    In this port it disables the IRQ sources and keeps FIQ sources
 *          enabled.
 */
#define port_lock() asm volatile ("msr     CPSR_c, #0x9F" : : : "memory")

/**
 * @brief   Kernel-unlock action.
 * @details Usually this function just enables interrupts but may perform
 *          more actions.
 * @note    In this port it enables both the IRQ and FIQ sources.
 */
#define port_unlock() asm volatile ("msr     CPSR_c, #0x1F" : : : "memory")

/**
 * @brief   Kernel-lock action from an interrupt handler.
 * @details This function is invoked before invoking I-class APIs from
 *          interrupt handlers. The implementation is architecture dependent,
 *          in its simplest form it is void.
 * @note    Empty in this port.
 */
#define port_lock_from_isr()

/**
 * @brief   Kernel-unlock action from an interrupt handler.
 * @details This function is invoked after invoking I-class APIs from interrupt
 *          handlers. The implementation is architecture dependent, in its
 *          simplest form it is void.
 * @note    Empty in this port.
 */
#define port_unlock_from_isr()

/**
 * @brief   Disables all the interrupt sources.
 * @note    Of course non-maskable interrupt sources are not included.
 * @note    In this port it disables both the IRQ and FIQ sources.
 * @note    Implements a workaround for spurious interrupts taken from the NXP
 *          LPC214x datasheet.
 */
#define port_disable() {                                                    \
  asm volatile ("mrs     r3, CPSR                       \n\t"               \
                "orr     r3, #0x80                      \n\t"               \
                "msr     CPSR_c, r3                     \n\t"               \
                "orr     r3, #0x40                      \n\t"               \
                "msr     CPSR_c, r3" : : : "r3", "memory");                 \
}

/**
 * @brief   Disables the interrupt sources below kernel-level priority.
 * @note    Interrupt sources above kernel level remains enabled.
 * @note    In this port it disables the IRQ sources and enables the
 *          FIQ sources.
 */
#define port_suspend() asm volatile ("msr     CPSR_c, #0x9F" : : : "memory")

/**
 * @brief   Enables all the interrupt sources.
 * @note    In this port it enables both the IRQ and FIQ sources.
 */
#define port_enable() asm volatile ("msr     CPSR_c, #0x1F" : : : "memory")

/**
 * @brief   Performs a context switch between two threads.
 * @details This is the most critical code in any port, this function
 *          is responsible for the context switch between 2 threads.
 * @note    The implementation of this code affects <b>directly</b> the context
 *          switch performance so optimize here as much as you can.
 * @note    Implemented as inlined code for performance reasons.
 *
 * @param[in] ntp       the thread to be switched in
 * @param[in] otp       the thread to be switched out
 */
#if CH_DBG_ENABLE_STACK_CHECK
#define port_switch(ntp, otp) {                                             \
  register struct intctx *r13 asm ("r13");                                  \
  if ((stkalign_t *)(r13 - 1) < otp->p_stklimit)                            \
    chDbgPanic("stack overflow");                                           \
  _port_switch_arm(ntp, otp);                                               \
}
#else
#define port_switch(ntp, otp) _port_switch_arm(ntp, otp)
#endif

#define port_wait_for_interrupt() { }

  void port_halt(void);
  void _port_switch_arm(Thread *ntp, Thread *otp);
  void _port_thread_start(void);

#endif

