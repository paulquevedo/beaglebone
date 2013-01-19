/* CPSR Modes */
#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_ABT 0x17
#define MODE_UND 0x1B
#define MODE_SYS 0x1F

/* CPSR Control Bits */
#define I_BIT    0x80 /* IRQ Disable */
#define F_BIT    0x40 /* FIQ Disable */

                        /* Linker script defines */
    .global _vector_start_addr
    .global _stack_bottom
    .global _stack_irq_top
    .global _stack_fiq_top
    .global _stack_und_top
    .global _stack_abt_top
    .global _stack_svc_top
    .global _stack_top
    .global _bss_start
    .global _bss_end
    .global _heap_start
    .global _heap_end

    .text
    .section .vector_table,"ax",%progbits
    .code 32

_vector_table:
    /* Initial Vector Table */
    b   _reset              /* Reset */
    b   _undefined_handler  /* Undefined Instruction */
    b   _undefined_handler  /* Software Interrupt */
    b   _undefined_handler  /* Prefetch Abort */
    b   _undefined_handler  /* Data Abort */
    b   .                   /* Reserved */
    b   _undefined_handler  /* IRQ */
    b   _undefined_handler  /* FIQ */

    .text
    .section .default_handlers,"ax",%progbits
    .align 4

_reset:
    /* Initialize stacks for each mode
     * Note that cpsr_c is an ARM intrinsic for the control bits of the cpsr */
    msr cpsr_c, #(MODE_UND | I_BIT | F_BIT)
    ldr r0, =_stack_und_top
    mov sp, r0

    msr cpsr_c, #(MODE_ABT | I_BIT | F_BIT)
    ldr r0, =_stack_abt_top
    mov sp, r0

    msr cpsr_c, #(MODE_FIQ | I_BIT | F_BIT)
    ldr r0, =_stack_fiq_top
    mov sp, r0

    msr cpsr_c, #(MODE_IRQ | I_BIT | F_BIT)
    ldr r0, =_stack_irq_top
    mov sp, r0

    msr cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
    ldr r0, =_stack_svc_top
    mov sp, r0

    msr cpsr_c, #(MODE_SYS | I_BIT | F_BIT)
    ldr r0, =_stack_top
    mov sp, r0

    /* Watermark the stacks */
    ldr r0, =_stack_bottom
    ldr r1, =_stack_top
    ldr r2, =0x5a5a5a5a
    /* Descending stack, fill from bottom up */
stack_loop:
    str r2, [r0], #4
    cmp r0, r1
    blt stack_loop

    /* Watermark the heap */
    ldr r0, =_heap_start
    ldr r1, =_heap_end
    ldr r2, =0xefefefef
heap_loop:
    str r2, [r0], #4
    cmp r0, r1
    blt heap_loop

    /* Zero out .bss section */
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
bss_loop:
    str r2, [r0], #4
    cmp r0, r1
    blt bss_loop

call_main:
    ldr r10,=main   /* Addres of main() */
    mov lr,pc       /* Dummy return */
    bx  r10         /* Bye-bye */

dead_loop:
    b dead_loop;

    .align 4
_undefined_handler:
    b _undefined_handler

    .end


