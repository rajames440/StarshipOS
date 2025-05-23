/* -*- asm -*- */

#include "globalconfig.h"
#include "config_tcbsize.h"
#include "tcboffset.h"
#include "asm_entry.h"


/**************************************************************************
 * Enter kernel mode (i.e. switch from any exception mode to the 
 * kernel mode and transfer the exception state).
 */

/**
 * Fix instruction pointer for entries from code on the kernel_lib page.
 *
 * \param insn         Register containing the instruction pointer for the
 *                     entry.
 * \param do_store_pc  Store the adjusted instruction pointer into the entry
 *                     stack frame (sp must point to the return frame minus
 *                     `sp_offset`.)
 * \param sp_offset    Offset of `sp` below the start of the return frame
 *                     (used with do_store_pc = true)
 * \pre If do_store_pc is set then `sp` must point `sp_offset` bytes below the
 *      start of the return frame (defined by `RF_SIZE`).
 * \post The value in register `insn` is either untouched, or adjusted to the
 *       start of an interrupted kernel_lib function.
 * \post If do_store_pc is set the adjusted value of the program counter
 *       is stored into the PC value within the return frame, referenced by
 *       `sp`.
 */
.macro atomic_fixup insn do_store_pc sp_offset=20
#ifndef CONFIG_ARM_V6PLUS
	@ Adjust PC if it is in the special atomic insns area
        @ Zero-flag set after this fixup code
        cmp     \insn, #0xffffe000
	bls	5477f
	cmp	\insn, #0xfffff000
	bhs     5477f
	tst	\insn, #0x10
	biceq	\insn, \insn, #0x0ff
.if \do_store_pc
	str	\insn, [sp, #RF(PC,  \sp_offset)]
.endif
5477:
	@ ---------------------------------------------------
#endif
.endm

.macro disable_irqs
	msr	cpsr_c, #0xd3
.endm

.macro mpu_kernel_enter offset
#ifdef CONFIG_MPU
	ldr	r0, [sp, #RF(PSR, \offset)]	@ get spsr from stack
	tst	r0, #0x0f			@ Mask all but relevant mode bits
	bne	111f				@ Kernel re-entry
	bl	__mpu_kernel_enter
111:
#endif
.endm

/**
 * Reconfigure MPU for user space.
 *
 * Clobbers registers r1-r3, r12. Spares r0 because it's relevant for the vCPU
 * kernel mode exit path.
 *
 * \param check   Test for target mode. Can be 0 if known to return to user
 *                space. Otherwise 1 if the eret may return to kernel mode.
 * \param offset  Kernel stack offset from entry frame.
 */
.macro mpu_kernel_leave check, offset
#ifdef CONFIG_MPU
.if \check != 0
	ldr	r1, [sp, #RF(PSR, \offset)]	@ get spsr from stack
	tst	r1, #0x0f			@ Mask all but relevant mode bits
	bne	112f				@ Kernel re-entry
.endif
	bl	__mpu_kernel_leave
112:
#endif
.endm

/**
 * switch to kernel mode and
 *
 * \param reg           scratch register (used if not_svc on ARMv5)
 * \param adjust        adjustment offset of the program counter
 * \param atomic_fixup  fixup program counter for atomic instructions if set
 * \param not_svc       set if code is running not in SVC mode (used
 *                      for ARMv5).
 * \pre `lr` of the current mode contains the exception program counter
 * \post IRQs/FIQs are disabled.
 * \post adjusted and fixed-up exception PC, SPSR, and SVC mode LR are
 *       stored on the SVC mode stack.
 * \post `sp` points to the SVC_LR field in the return frame (8 bytes into
 *       the return frame).
 *
 */
.macro  __switch_to_kernel reg adjust atomic_fixup not_svc
.if \adjust != 0
	sub	lr, lr, #\adjust
.endif
#ifdef CONFIG_ARM_V6PLUS
#ifdef CONFIG_ARM_1136
	// todo: do clrex with strex for CPUs without clrex
#else
	clrex
#endif
#endif
.if \atomic_fixup
        atomic_fixup lr 0
.endif
#if defined(CONFIG_ARM_V6PLUS)
	srsdb   #0x13!
	disable_irqs
	sub	sp, sp, #(RF_SIZE - 8)
	str	lr, [sp, #RF(SVC_LR, 0)]
#else
.if \not_svc
	str	lr, s_lr
	mrs	\reg, spsr
	str	\reg, s_spsr
	disable_irqs
.endif  @ syscall (already in svc mode)
	sub	sp, sp, #RF_SIZE
	str	lr, [sp, #RF(SVC_LR, 0)]
.if \not_svc
	ldr	lr, s_spsr
	str	lr, [sp, #RF(PSR, 0)]
	ldr	lr, s_lr
	str	lr, [sp, #RF(PC, 0)]
.else
	str	lr, [sp, #RF(PC, 0)]
	mrs	lr, spsr
	str	lr, [sp, #RF(PSR, 0)]
.endif
#endif
.endm
.macro  switch_to_kernel adjust atomic_fixup not_svc
	__switch_to_kernel r14 \adjust \atomic_fixup \not_svc
.endm

/*************************************************************************
 * return from an exception
 * \pre interrupts must be disabled
 * \pre `sp` must point to the return frame
 * \post `sp` points to the top of the stack (pops the return frame)
 */
.macro 	return_from_exception
	ldr	lr, [sp, #RF(PSR,0)]		@ Unstack SPSR
	tst	lr, #0x0f			@ Mask all but relevant mode bits
	add	sp, sp, #RF_SIZE		@ SP to top of stack
#if defined(CONFIG_ARM_V6PLUS)
	ldrne   lr, [sp, #RF(SVC_LR, -RF_SIZE)]	@ load old kernel lr
	rfedb	sp
#else
	msr	spsr_cfsx, lr			@ Load SPSR from kernel_lr
	ldr	lr, [sp, #RF(PC, -RF_SIZE)]	@ copy PC on psr field for
	str	lr, [sp, #RF(PSR, -RF_SIZE)]	@   final ldmdb and proper ksp
        ldrne   lr, [sp, #RF(SVC_LR, -RF_SIZE)]	@ load old kernel lr
	ldmdb	sp, {pc}^			@ go back to interrupted insn 
						@ (load psr)
#endif
.endm

/***********************************************************************
 * Enter the kernel slowtrap handler
 *
 * Stack the state and call 'slowtrap_entry' with sp and error code
 * \pre 'sp' point to the start of the reurn frame
 * \post R0-R12, errorcode, and pfa-dummy are stored below the return
 *       frame and `sp` point to the pfa ...
 */
.macro 	enter_slowtrap errorcode
	stmdb 	sp!, {r0 - r12}
	mpu_kernel_enter (13*4)
	enter_slowtrap_w_stack \errorcode
.endm

/*	
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr'  |  (pc after syscall)
 *             +-------+
 *   old sp -> | spsr  |
 *	       +-------+
 *             | km_lr |
 *             +-------+
 *             |  lr^  |  (user lr)
 *             +-------+
 *       sp -> |  sp^  |  (user sp)
 *             +-------+
 *             |       |
 *
 */


/***************************************************************************
 * Generate stack for system call entries
 *
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr^  |  (pc after syscall)
 *             +-------+
 *   old sp -> | spsr  |
 *	       +-------+
 *             | km_lr |
 *             +-------+
 *             |  xx   |
 *             +-------+
 *       sp -> |  sp^  |  (user sp)
 *             +-------+
 *             |       |
 *
 *
 * lr: must contain fault addr (from switch_to_kernel)
 */
.macro  enter_sys_call no_sys_call
	cmp	r5, #1
	bhi	\no_sys_call

	stmdb	sp!, {r0 - r12}
	mpu_kernel_enter (13*4)
	/* stack frame: RF + r12-r0, sp points to r0 */
	CONTEXT_OF	r1, sp
	RESET_THREAD_CANCEL_AT r1       @ sets r0 to state 
	tst	r0, #VAL__Thread_alien_or_vcpu_user
	bne	alien_syscall
	adr	r0, sys_call_table
	adr	lr, 2f
1:	ldr	pc, [r0, +r5, LSL #2]
.global fast_ret_from_irq
.type fast_ret_from_irq, #function
fast_ret_from_irq:
2:	disable_irqs
	mpu_kernel_leave 1, (13*4)
	ldmia	sp!, {r0 - r12}
	add	sp, sp, #(RF_SIZE)

        // XXX: use rfe here
	/* Return */
	ldr	lr, [sp, #RF(PSR,-RF_SIZE)]
	msr	spsr_cfsx, lr
	ldr	lr, [sp, #RF(PC, -RF_SIZE)]
	movs	pc, lr
.endm

/**************************************************************************
 * The Exception vector table.
 */
#ifdef CONFIG_MMU
// MMU kernels map the exception vectors in a separate page to the high vectors.
.section	.vect,"a"
#else
// MPU kernel can embed sufficently aligned exception vectors in the image.
.p2align 5
.section	.text.ivt,"xa"
#endif
.globl	exception_vector
exception_vector:
#ifndef CONFIG_ARM_EM_TZ
	nop				/* RESET	*/
	b	undef_entry		/* UNDEF	*/
	b	swi_entry		/* SWI		*/
	b	inst_abort_entry	/* IABORT	*/
	b	data_abort_entry	/* DABORT	*/
	nop				/* reserved	*/
	b	irq_entry		/* IRQ		*/
	b	fiq_entry		/* FIQ		*/
#else
	nop				/* RESET	*/
	adr     pc, (fiq_b_vector + 8)	/* UNDEF	*/
	adr     pc, (fiq_b_vector + 16)	/* SWI		*/
	adr     pc, (fiq_b_vector + 24) /* IABORT	*/
	adr     pc, (fiq_b_vector + 32) /* DABORT	*/
	nop				/* reserved	*/
	b       irq_entry               /* IRQ   	*/
	b	fiq_entry		/* FIQ		*/

fiq_b_vector:
	nop                             /* RESET	*/
        nop
	cpsid f
        b	undef_entry		/* UNDEF	*/
	cpsid f
	b	swi_entry		/* SWI		*/
	cpsid f
	b	inst_abort_entry	/* IABORT	*/
	cpsid f
	b	data_abort_entry	/* DABORT	*/
#endif

/* locations to pass lr and spsr from one mode to the other
   these are globally shared !!! */
#ifdef CONFIG_MMU
.section	.excp.text,"xa"
#endif
#if !defined(CONFIG_ARM_V6PLUS)
s_lr:	.word	0
s_spsr:	.word	0
#endif

/***************************************************************************
**
** Exception entry points.
**
*/

/***************************************************************************
 * Exception undef ()
 *
 *    Exception is an undefined instruction.
 *
 *    We do the fixup ourself because it depends on the PE state.
 */
undef_entry:
	switch_to_kernel 0 0 1
	ldr     lr, [sp, #RF(PSR,    0)]  @ read orig PSR
	tst     lr, #0x20                 @ Thumb mode?
	ldr	lr, [sp, #RF(PC, 0)]      @ read orig exception PC
	subeq	lr, lr, #4                @ adjust pc A32 mode
	subne	lr, lr, #2                @ adjust pc T32 mode
	str	lr, [sp, #RF(PC, 0)]      @ store to return address
	enter_slowtrap GET_HSR(0x00)

/**************************************************************************
 * Exception swi ()
 *
 *    Exception is a software interrupt (typically a syscall in normal
 *    OSes).
 *
 */
swi_entry:
	switch_to_kernel 0 0 0
	enter_sys_call no_sys_call
no_sys_call:
	ldr     lr, [sp, #RF(PSR,    0)]  @ read orig PSR
	tst     lr, #0x20                 @ Thumb mode?
	ldr	lr, [sp, #RF(SVC_LR, 0)]  @ read orig exception PC
	subeq	lr, lr, #4                @ adjust pc to be on swi insn (A32)
	subne	lr, lr, #2                @ adjust pc to be on swi insn (T32)
	str	lr, [sp, #RF(PC,     0)]  @ store to return address
	str	lr, [sp, #RF(SVC_LR, 0)]  @ store to old kernel lr
	enter_slowtrap GET_HSR(0x11)

.align 4

/***************************************************************************
 * Exception inst_abort ()
 *
 *    Exception is a prefetch (instruction) abort.  This exception is also
 *    used for L4 syscalls.  If the exception address is in the range 0x00
 *    to 0x24 (in the exception vector page), this is interpreted as a
 *    syscall number.  Some platforms allow the exception vector to be
 *    relocated to the beginning of the last 64K of memory.  For these
 *    platforms, we use a negative (i.e. end of address space) value to
 *    indicate the syscall number.  If exception is not within the syscall
 *    range, generate a pager IPC (or panic if within the kernel).
 *
 */
inst_abort_entry:

	switch_to_kernel 4 0 1
/**************************************************************************/
/*prefetch_abort: @ A real prefetch abort occurred --- handled as a page fault */
	stmdb	sp!, {r0 - r3, r12} 	@ Stack rest of user state
	mpu_kernel_enter (5*4)
	ldr	lr, [sp, #RF(PSR, 5*4)]	@ get spsr from stack
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
	bne	kernel_prefetch_abort	@ Kernel abort?
	/* user prefetch abort */
	mrc	p15, 0, r1, c5, c0, 1	@ Load IFSR into r1
	bic	r1, r1, #GET_HSR(0x3f)
	orr	r1, r1, #GET_HSR(0x20)  @ Set read bit and prefetch abort
#if defined(CONFIG_ARM_V6PLUS) && !defined(CONFIG_ARM_1136) && !defined(CONFIG_ARM_MPCORE)
# if defined(CONFIG_ARM_V8PLUS)
	tst	r1, #(1<<16)            @ IFSR.FnV
	mrceq	p15, 0, r0, c6, c0, 2   @ Read PFA if IFSR.FnV==0, for T2: pfa != pc
	movne	r0, #0xffffffff         @ Use PFA=0xffffffff if IFSR.FnV==1
# else
	mrc	p15, 0, r0, c6, c0, 2   @ Read fault address, for T2: pfa != pc
# endif
#else
	ldr	r0, [sp, #RF(PC, 5*4)]  @ Get PC from RF and use as pfa
#endif
	mov	r2, r0
	add 	r3, sp, #(5*4)
	stmdb	sp!, {r0, r1}
	align_and_save_sp r12           @ use r12 as scratch
	adr	lr, pagefault_return
	ldr	pc,  .LCpagefault_entry	@ Jump to C code

kernel_prefetch_abort:			@ Kernel generated IAbort
					@ Should not get IAborts in kernel
					@ Kernel Panic
	adr	r0, kernel_prefetch_abort_label
	b	kern_kdebug_cstr_entry


kernel_prefetch_abort_label: .string "Kernel prefetch abort"

/*****************************************************************************/
/* The syscall table stuff                                                   */
/*****************************************************************************/
GEN_SYSCALL_TABLE
GEN_LEAVE_BY_TRIGGER_EXCEPTION
GEN_LEAVE_AND_KILL_MYSELF


/****************************************************************************
 * Exception data_abort ()
 *
 *    Exception is a data abort.  If exception happened in user mode,
 *    generate pager IPC.  If exception happened in kernel mode, it is
 *    probably due to a non-mapped TCB (or else we panic).
 *
 *
 * Stack layout:
 *
 *   old sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | spsr  | +64
 *	       +-------+
 *             | km_lr | +60
 *             +-------+
 *             |  ulr  | +56
 *             +-------+
 *             |  usp  | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *       sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 *
 */

.macro check_ldrd_insn jmp_to_if_ldrd
	tst	r3, #0x0e000000
	bne	1f
	and	r12, r3, #0x000000f0
	cmp	r12, #0x000000d0
	bne	1f
	tst	r3, #(1<<20)
	beq	\jmp_to_if_ldrd
1:
.endm

.p2align 2
data_abort_entry:
	switch_to_kernel 8 0 1
	stmdb   sp!, {r0 - r3, r12}          @ Stack rest of user state
	mpu_kernel_enter (5*4)

	/* user data abort */
#ifdef CONFIG_ARM_V6PLUS
	mrc     p15, 0, r1, c5, c0, 0	@ Load DFSR into r1
        and     r2, r1, #(1 << 11)      @ FSR WnR
        bic     r1, r1, #((1 << 6) | (1 << 11)) @ clear HSR & FSR WnR
        orr     r1, r1, r2, lsr #5      @ make HSR.Wnr from FSR.WnR
	bic	r1, r1, #GET_HSR(0x3f)
# ifdef CONFIG_ARM_V8PLUS
	tst	r1, #(1<<16)		@ DFSR.FnV
	mrceq	p15, 0, r0, c6, c0, 0	@ Load DFAR into r0 if DFSR.FnV==0
	movne	r0, #0xffffffff		@ Set r0=0xffffffff if DFSR.FnV==1
# else
	mrc     p15, 0, r0, c6, c0, 0	@ Load DFAR into r0
# endif

	ldr	r2, [sp, #RF(PC,  5*4)] @ Load PC into r2
	ldr	lr, [sp, #RF(PSR, 5*4)]	@ load spsr, from stack 

	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	add 	r3, sp, #(5*4)
	orrne   r1, r1, #GET_HSR(0x01)
	orr	r1, r1, #GET_HSR(0x24)  @ Set error code to data abort

	stmdb	sp!, {r0, r1}
	align_and_save_sp r12           @ use r12 as scratch
	adr	lr, pagefault_return	@ set return address

	ldr	pc, .LCpagefault_entry	@ page fault	
#else
	mrc     p15, 0, r1, c5, c0, 0	@ Load FSR into r1
	bic	r1, r1, #(1 << 6)	@ clear bit 6 (write indicator)
	bic	r1, r1, #GET_HSR(0x3f)
	mrc     p15, 0, r0, c6, c0, 0	@ Load FAR into r0
	ldr	r2, [sp, #RF(PC,  5*4)] @ Load PC into r2
	ldr	lr, [sp, #RF(PSR, 5*4)]	@ load spsr, from stack 
	tst	lr, #0x20		@ comes from thumb mode?
	bne	.LChandle_thumb
	@ arm insns
	ldr	r3, [r2]		@ Load faulting insn
	check_ldrd_insn .LCwas_ldrd
	tst	r3, #(1<<20)
	orreq	r1, r1, #(1 << 6)	@ Set FSR write bit
.LCret_handle_thumb:
.LCwas_ldrd:
	atomic_fixup r2 1
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	add 	r3, sp, #(5*4)
	orrne	r1, r1, #GET_HSR(0x01)
	orr	r1, r1, #GET_HSR(0x24)  @ Set error code to data abort
	stmdb	sp!, {r0, r1}
	align_and_save_sp r12           @ use r12 as scratch
	adr	lr, pagefault_return	@ set return address

	ldr	pc, .LCpagefault_entry	@ page fault

.LChandle_thumb:
	@ thumb insns
	ldrh	r3, [r2]
	and     r3, r3, #0xfe00
	teq     r3, #0x5600
	beq     .LCret_handle_thumb
	tst     r3, #(1<<11)
	orreq   r1, r1, #(1 << 6)	@ Set FSR write bit
	b .LCret_handle_thumb
#endif

.LCpagefault_entry:	.word	pagefault_entry
.LCslowtrap_entry:	.word	slowtrap_entry


/***************************************************************************
 * Generic return code for restoring the thread state after exceptions.
 *
 * Stack layout:
 *
 *	 sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | spsr  | +64
 *             +-------+
 *             | km_lr | +60
 *             +-------+
 *             |  ulr  | +56
 *             +-------+
 *             |  usp  | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *   old sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 */	
pagefault_return:
	ldr	sp, [sp]
	cmp 	r0, #0
	ldmia	sp!, {r12, lr}
	beq	slowtrap_from_pagefault
	
	msr	cpsr_c, #0xd3 // disable IRQs
	mpu_kernel_leave 1, (5*4)
	ldmia	sp!, {r0 - r3, r12}		@ Restore user state
	return_from_exception

slowtrap_from_pagefault:
	disable_irqs
	ldmia 	sp!, {r0 - r3}
	stmdb 	sp!, {r0 - r11}
	stmdb 	sp!, {r12, lr}
	align_and_save_sp r0
	adr   	lr, exception_return
	ldr	pc, .LCslowtrap_entry	@ slow trap

GEN_EXCEPTION_RETURN
GEN_IRET

/***************************************************************************
 * Exception irq ()
 *
 *    Exception is an interrupt.  Generate interrupt IPC.
 *
 */
irq_entry:
	switch_to_kernel 4 1 1

	stmdb   sp!, {r0 - r3, r12}   	@ Stack rest of user state
	mpu_kernel_enter (5*4)
	align_and_save_sp r12
	@ add r0, sp, #(5*4) @ debug
        mov     lr, pc
        ldr     pc, 1f
	ldr	sp, [sp]
	disable_irqs
	mpu_kernel_leave 1, (5*4)
	ldmia	sp!, {r0 - r3, r12}		@ Restore user state
	return_from_exception

	.global __irq_handler_irq
__irq_handler_irq:
1:	.word	irq_handler

/******************************************************************************
 * Exception fiq ()
 *
 *    Exception is a fast interrupt.
 *
 */

#ifdef CONFIG_ARM_EM_TZ
.pc_adjust_map_arm:
	.long 0    /* RESET	*/
	.long 4    /* UNDEF	*/
	.long 4	   /* SWI	*/
	.long 4    /* IABORT	*/
	.long 8    /* DABORT	*/
.pc_adjust_map_thumb:
	.long 0    /* RESET	*/
	.long 2    /* UNDEF	*/
	.long 4	   /* SWI	*/
	.long 4    /* IABORT	*/
	.long 8    /* DABORT	*/
#endif // ARM_EM_TZ

fiq_entry:
#ifdef CONFIG_ARM_EM_TZ
	mrs	r8, spsr
	and	r9, r8, #0x1f
	cmp	r9, #0x10
	beq	9f

	mov	r10, r0

	cmp	r9, #0x13
	bne	8f

	cps	#0x13
	mov	r0, sp
	cps	#0x11
	mov	r11, r0
	mov	r0, r10 // restore

	tst	r11, #0x0ff
	tsteq	r11, #0xf00
	bne	9f

8:
	sub	r12, lr, #4
#ifdef CONFIG_ARM_1176
	mov	r11, #(fiq_b_vector - exception_vector)
	orr	r11, #0xff000000
	orr	r11, #0x00ff0000
#else
	movw	r11, #(fiq_b_vector - exception_vector)
	movt	r11, #0xffff
#endif
	cmp	r12, r11
	subge	r12, r12, r11
	lsrge	r12, #1
	and	r12, r12, #0x1f

	tst	r8, #0x20
	adreq	r0, .pc_adjust_map_arm
	adrne	r0, .pc_adjust_map_thumb
	ldr	r0, [r0, r12]

	orr	r9, #0xc0
	msr	cpsr_c, r9

	sub	lr, lr, r0
	srsdb	#0x13!
	cps	#0x13
	sub	sp, sp, #(RF_SIZE - 8)
	str	lr, [sp, #RF(SVC_LR, 0)]

	cps	#0x11
	mov	r0, r10
	cps	#0x13
	b	7f
9:
#endif // ARM_EM_TZ
	switch_to_kernel 4 1 1
7:
	stmdb	sp!, {r0 - r3, r12}   	@ Stack rest of user state
	mpu_kernel_enter (5*4)
	align_and_save_sp r12
	mov	lr, pc
	ldr	pc, 1f
	ldr	sp, [sp]
	disable_irqs
	mpu_kernel_leave 1, (5*4)
	ldmia	sp!, {r0 - r3, r12}		@ Restore user state
	return_from_exception

	.global __irq_handler_fiq
__irq_handler_fiq:
1:	.word	irq_handler


/**************************************************************************/
/* The alien stuff is below                                               */
/**************************************************************************/
/**
 * \pre sp  points to r0-r12 + RF
 */
alien_syscall: @ Do it for an alien ---------------------------------------
	tst	r0, #VAL__Thread_dis_alien
	bicne	r0, r0, #VAL__Thread_dis_alien
	bne	1f
	@ Trap alien before system call -----------------------------------
	ldr     r0, [sp, #RF(PSR,    13*4)] @ read orig PSR
	tst     r0, #0x20                   @ Thumb mode?
	ldr	lr, [sp, #RF(PC,     13*4)] @ read orig exception PC
	subeq	lr, lr, #4                  @ A32: adjust pc to be on SVC insn
	subne	lr, lr, #2                  @ T32: adjust pc to be on SVC insn
	str	lr, [sp, #RF(PC,     13*4)] @ store to entry_stack_PC
	enter_slowtrap_w_stack GET_HSR(0x11)
	@ Never reach this -- end up in user land after exception reply

1:	@ Resume the alien system call ------------------------------------
	str	r0, [r1, #(OFS__THREAD__STATE)]
	adr	r0, sys_call_table
	adr	lr, 2f
	ldr	pc, [r0, +r5, LSL #2]
2:	nop @ The return point after the resumed alien system call --------
	disable_irqs
	@ Trap after the resumed alien system call ------------------------
	@ The trap occurs at the insn where the system call returns to.
	@ Set the bit 0x00000040 to indicate a trap after the resumed 
	@ system call.
	enter_slowtrap_w_stack GET_HSR(0x11), 0x40

	
GEN_VCPU_UPCALL OFS__THREAD__USER_VCPU

missed_excpt_ret_label:      .string "ERROR in exception return"
fiq_label:	             .string "FIQ entry"

/**************************************************************************
 * MPU kernel entry and exit handling.
 */

#ifdef CONFIG_MPU
__mpu_kernel_enter:
	CONTEXT_OF r0, sp

	// restore regular heap region
	ldr	r1, [r0, #(OFS__THREAD__MPU_PRBAR2)]
	ldr	r2, [r0, #(OFS__THREAD__MPU_PRLAR2)]
	mcr	p15, 0, r1, c6, c9, 0	// write PRBAR2
	mcr	p15, 0, r2, c6, c9, 1	// write PRLAR2

	// Disable all user regions. The missing PRENR register makes this
	// really a pain...
	ldr	r1, [r0, #(OFS__THREAD__KU_MEM_REGIONS)]

	mov	r3, #1
1:	clz	r2, r1			// find highest bit set
	rsbs	r2, r2, #31
	bmi	2f			// exit loop if all zero

	mcr	p15, 0, r2, c6, c2, 1	// PRSELR
	isb
	bics	r1, r1, r3, LSL r2	// clear bit in mask
	mrc	p15, 0, r2, c6, c3, 1	// read PRLAR
	bic	r2, r2, #1		// clear EN
	mcr	p15, 0, r2, c6, c3, 1	// write back PRLAR

	bne	1b			// next iteration if still sth set
2:
	isb				// Make sure MPU updates have settled
	bx	lr

__mpu_kernel_leave:
	CONTEXT_OF r1, sp

	// enable all user regions
	ldr	r2, [r1, #(OFS__THREAD__KU_MEM_REGIONS)]

	mov	r12, #1
1:	clz	r3, r2			// find highest bit set
	rsbs	r3, r3, #31
	bmi	2f			// exit loop if all zero

	mcr	p15, 0, r3, c6, c2, 1	// PRSELR
	isb
	bics	r2, r2, r12, LSL r3	// clear bit in mask
	mrc	p15, 0, r3, c6, c3, 1	// read PRLAR
	orr	r3, r3, #1		// set EN
	mcr	p15, 0, r3, c6, c3, 1	// write back PRLAR

	bne	1b			// next iteration if still sth set
2:
	// constrain heap region to stack
	mrc	p15, 0, r2, c6, c9, 0	// read PRBAR2
	mrc	p15, 0, r3, c6, c9, 1	// read PRLAR2

	add	r12, r1, #((THREAD_BLOCK_SIZE-1) & ~0x3f)
	and	r2, r2, #0x3f
	and	r3, r3, #0x3f
	orr	r2, r2, r1
	orr	r3, r3, r12

	mcr	p15, 0, r2, c6, c9, 0	// write PRBAR2
	mcr	p15, 0, r3, c6, c9, 1	// write PRLAR2
	bx	lr
#endif

/**********************************************************************
	kdebug entry
 **********************************************************************/
GEN_DEBUGGER_ENTRIES


#ifdef CONFIG_ARM_EM_TZ

.macro ISB_OP reg
#ifdef CONFIG_ARM_V7PLUS
	isb
#else
	mov \reg, #0
	mcr p15, 0, \reg, c7, c5, 4	@ cp15isb
#endif
.endm

/**********************************************************************
 * Secure and Nonsecure switching stuff
 *
 *********************************************************************/
.macro SAVE_NONSECURE_STATE off

	// save exit reason temporarily on stack
	str	lr, [sp,#-12]
	
	// switch to secure world
	SWITCH_TO_SECURE_MODE lr

	// save gen-regs
	ldr	lr, [sp, #\off]
	//add	lr, lr, #8
	stmia	lr!, {r0 - r12}

	mov	r0, lr

	// usr
	stmia	r0, {sp, lr}^
	add	r0, r0, #8

	// irq
	cps	#0x12
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}

	// fiq
	cps	#0x11
	stmia	r0!, {r8 - r12, sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}

	// abt
	cps	#0x17
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}

	// und
	cps	#0x1b
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}

	// svc
	cps	#0x13
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}

	cps	#0x16

	// copy return pc/cpsr from stack
	sub	lr, sp, #8
	ldmia	lr, {r1, r2}
	stmia	r0!, {r1, r2}
	
	// save pending virtual interrupt state
	mrc	p15, 0, r1, c12, c1, 0
	stmia	r0!, {r1}

        mrc     p15, 0, r1, c1, c0, 2 @ cpacr
        str     r1, [r0], #4

#ifdef CONFIG_FPU
        mov     r1, #0x500000 @ enable CP10/11
        mcr     p15, 0, r1, c1, c0, 2 @ cpacr
#endif

	// switch to non-secure world
	SWITCH_TO_NONSECURE_MODE r1

#ifdef CONFIG_FPU
	mrc	p10, 7, r1, cr8, cr0, 0 @ fpexc
	stmia	r0!, {r1}
#else
	add	r0, #4
#endif

	// switch to secure world
        SWITCH_TO_SECURE_MODE r1
	
	mrc	p15, 0, r1, c5, c0, 0	@ read CP15_DFSR
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c6, c0, 0	@ read CP15_DFAR
	stmia	r0!, {r1}

	// copy the exit reason from stack
	ldr	r1, [sp, #-12]
	stmia	r0!,{r1}
.endm

.macro RESTORE_AND_SWITCH_TO_NONSECURE_STATE off

	ldr	r0, [sp, #\off]

	// jump over general purpose register
	add	r0, r0, #13*4

	// usr
	ldmia	r0, {sp, lr}^
	add	r0, r0, #8

	// irq
	cps	#0x12
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	msr	spsr_cfsx, r1

	// fiq
	cps	#0x11
	ldmia	r0!, {r8 - r12, sp, lr}
	ldmia	r0!, {r1}
	msr	spsr_cfsx, r1

	// abt
	cps	#0x17
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	msr	spsr_cfsx, r1

	// und
	cps	#0x1b
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	msr	spsr_cfsx, r1
	
	// svc
	cps	#0x13
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	msr	spsr_cfsx, r1

	cps	#0x16

	// copy return pc/cpsr on stack
	ldmia	r0!, {r1, r2}
	stmdb	sp, {r1, r2}

	// skip pending events field
	add	r0, #4

        ldr     r1, [r0], #4
        mrc     p15, 0, r1, c1, c0, 2 @ cpacr

	// switch to non-secure world
	SWITCH_TO_NONSECURE_MODE r1

#ifdef CONFIG_FPU
	ldmia	r0!, {r1}
	mcr	p10, 7, r1, cr8, cr0, 0     @ fpexc
#else
	add	r0, #4
#endif

	// load gen-regs
	ldr	lr, [sp, #\off]
	ldmia	lr!, {r0 - r12}
.endm

/**********************************************************************
 * Save secure state on top of the stack.
 *
 * We save also the user-level registers here, because we need to
 * restore some on FIQ.
 *
 */
.macro SAVE_SECURE_STATE

        mrc p15, 0, r5, c1, c0, 2
        str     r5, [sp, #-4]!
	stmdb	sp!, {r3, r4}	@ save supervisor return values
	stmdb	sp, {sp, lr}^	@ save user-level return values
	sub	sp, sp, #8
.endm

/**********************************************************************
 * Restore secure state when guest returns with monitor call.
 *
 * This removes the secure state from the top of the stack.
 */
.macro RESTORE_SECURE_STATE

	mov	r0, sp		@ restore stack pointer from supervisor mode
	cps	#0x13
	mov	sp, r0
	cps	#0x16
	ldmia	sp, {sp, lr}^	@ restore user-level return values
	add	sp, sp, #8
	ldmia	sp!, {r3, r4}	@ restore supervisor return values
        ldr     r0, [sp], #4
        mcr p15, 0, r0, c1, c0, 2
.endm

.macro SWITCH_TO_NONSECURE_MODE reg
#ifdef CONFIG_ARM_1176
	mov	\reg, #0x100
	orr	\reg, #0x02d
#else
	movw	\reg, #0x12d
#endif
	mcr	p15, 0, \reg, c1, c1, 0
	ISB_OP	\reg
.endm

.macro SWITCH_TO_SECURE_MODE reg
	mov	\reg, #0x100
	mcr	p15, 0, \reg, c1, c1, 0
	ISB_OP	\reg
.endm


/*****************************************************************************/
/* The monitor entry table stuff                                             */
/*****************************************************************************/
.p2align 5
.globl monitor_vector_base
monitor_vector_base:
	nop				/* RESET	*/
	b	mon_undef_entry		/* UNDEF	*/
	b	mon_swi_entry		/* SWI		*/
	b	mon_inst_abort_entry	/* IABORT	*/
	b	mon_data_abort_entry	/* DABORT	*/
	nop				/* reserved	*/
	b	mon_irq_entry		/* IRQ		*/
	b	mon_fiq_entry		/* FIQ		*/

.macro mon_enter_secure name, ec, sub_lr
\name:
.if \sub_lr
        sub     lr, lr, #\sub_lr
.endif
        srsdb   sp, #0x16
        mov     lr, #\ec
        b       go_secure
.endm

mon_enter_secure mon_undef_entry,      6, 4
mon_enter_secure mon_swi_entry,        1, 0
mon_enter_secure mon_inst_abort_entry, 2, 4
mon_enter_secure mon_data_abort_entry, 3, 8
mon_enter_secure mon_irq_entry,        4, 4
mon_enter_secure mon_fiq_entry,        5, 4

//	cps	#0x12			@ switch to irq mode
//	adr	lr, go_nonsecure_after_fiq + 4			@ set lr_irq
//	msr	spsr, #0xd3		@ set spsr_irq
//	b	fiq_entry

/**********************************************************************
 * Go to secure world
 *
 */
go_secure:
	SAVE_NONSECURE_STATE 20
	RESTORE_SECURE_STATE
	
	mov	lr, r3
	msr	spsr_cfsx, r4
	movs	pc, lr
	
/**********************************************************************
 * Go to nonsecure world
 *
 * When the guest was interrupted by an FIQ, we don't need to save
 * secure state again, because it is still on top of the stack.
 *
 */
//go_nonsecure_after_fiq:
//	mov	r2, sp			@ copy sp_svc to sv_mon
//	cps	#0x16
//	mov	sp, r2
//	cps	#0x13
//	b go_nonsecure_after_fiq_2

.globl go_nonsecure
go_nonsecure:
	SAVE_SECURE_STATE
	RESTORE_AND_SWITCH_TO_NONSECURE_STATE 20

	ldr	lr, [sp, #-4]
	msr	spsr_cfsx, lr		@ set spsr_mon with unsecure spsr
	ldr	lr, [sp, #-8]		@ set lr_mon with unsecure ip
	movs	pc, lr
#endif /* TRUSTZONE */

/* -------------------------------------- TEXT ---------------------------*/

.text
	.global	vcpu_resume
	.type vcpu_resume, #function
vcpu_resume:
#ifdef CONFIG_MPU
	// We cannot update sp before calling mpu_kernel_leave because
	// CONTEXT_OF does not work with a fully emtpy stack!
	mov	r4, r1
	mpu_kernel_leave 0, 0
	add	sp, r4, #RF_SIZE
#else
	add	sp, r1, #RF_SIZE
#endif
	add	lr, r0, #8			@ pfa, err
#if !defined(CONFIG_ARM_V6PLUS)
	ldr	r1, [lr, #RF(PSR, 13*4)]	@ Unstack SPSR
	msr	spsr_cfsx, r1			@ Load SPSR from kernel_lr
#endif
	ldmia	lr!, {r0 - r12}
	ldmia	lr, {sp,lr}^			@ restore user sp and lr (now lazy)
#if defined(CONFIG_ARM_V6PLUS)
	add	lr, lr, #RF_SIZE		@ Read return address
	rfedb	lr
#else
	add	lr, lr, #(RF_SIZE - 4)		@ Read return address
	ldmdb	lr, {pc}^			@ go back to interrupted insn 
#endif
