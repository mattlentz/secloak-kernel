/*
 * Copyright (c) 2015, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arm.h>
#include <kernel/abort.h>
#include <kernel/linker.h>
#include <kernel/misc.h>
#include <kernel/panic.h>
#include <mm/core_mmu.h>
#include <trace.h>

#include "thread_private.h"

enum fault_type {
	FAULT_TYPE_USER_TA_PANIC,
	FAULT_TYPE_USER_TA_VFP,
	FAULT_TYPE_PAGEABLE,
	FAULT_TYPE_IGNORE,
};

static __maybe_unused const char *abort_type_to_str(uint32_t abort_type)
{
	if (abort_type == ABORT_TYPE_DATA)
		return "data";
	if (abort_type == ABORT_TYPE_PREFETCH)
		return "prefetch";
	return "undef";
}

static __maybe_unused const char *fault_to_str(uint32_t abort_type,
			uint32_t fault_descr)
{
	/* fault_descr is only valid for data or prefetch abort */
	if (abort_type != ABORT_TYPE_DATA && abort_type != ABORT_TYPE_PREFETCH)
		return "";

	switch (core_mmu_get_fault_type(fault_descr)) {
	case CORE_MMU_FAULT_ALIGNMENT:
		return " (alignment fault)";
	case CORE_MMU_FAULT_TRANSLATION:
		return " (translation fault)";
	case CORE_MMU_FAULT_READ_PERMISSION:
		return " (read permission fault)";
	case CORE_MMU_FAULT_WRITE_PERMISSION:
		return " (write permission fault)";
	default:
		return "";
	}
}

static __maybe_unused void __print_abort_info(
				struct abort_info *ai __maybe_unused,
				const char *ctx __maybe_unused)
{
	EMSG_RAW("\n");
	EMSG_RAW("%s %s-abort at address 0x%" PRIxVA "%s",
		ctx, abort_type_to_str(ai->abort_type), ai->va,
		fault_to_str(ai->abort_type, ai->fault_descr));
#ifdef ARM32
	EMSG_RAW(" fsr 0x%08x  ttbr0 0x%08x  ttbr1 0x%08x  cidr 0x%X",
		 ai->fault_descr, read_ttbr0(), read_ttbr1(),
		 read_contextidr());
	EMSG_RAW(" cpu #%zu          cpsr 0x%08x",
		 get_core_pos(), ai->regs->spsr);
	EMSG_RAW(" r0 0x%08x      r4 0x%08x    r8 0x%08x   r12 0x%08x",
		 ai->regs->r0, ai->regs->r4, ai->regs->r8, ai->regs->ip);
	EMSG_RAW(" r1 0x%08x      r5 0x%08x    r9 0x%08x    sp 0x%08x",
		 ai->regs->r1, ai->regs->r5, ai->regs->r9,
		 read_mode_sp(ai->regs->spsr & CPSR_MODE_MASK));
	EMSG_RAW(" r2 0x%08x      r6 0x%08x   r10 0x%08x    lr 0x%08x",
		 ai->regs->r2, ai->regs->r6, ai->regs->r10,
		 read_mode_lr(ai->regs->spsr & CPSR_MODE_MASK));
	EMSG_RAW(" r3 0x%08x      r7 0x%08x   r11 0x%08x    pc 0x%08x",
		 ai->regs->r3, ai->regs->r7, ai->regs->r11, ai->pc);
#endif /*ARM32*/
}

/*
 * Print abort info and (optionally) stack dump to the console
 * @ai user-mode or kernel-mode abort info. If user mode, the current session
 * must be the one of the TA that caused the abort.
 * @stack_dump true to show a stack trace
 */
static void __abort_print(struct abort_info *ai, bool stack_dump)
{
	bool paged_ta = false;

	__print_abort_info(ai, "Core");

	if (!stack_dump || paged_ta)
		return;
}

void abort_print(struct abort_info *ai)
{
	__abort_print(ai, false);
}

void abort_print_error(struct abort_info *ai)
{
	__abort_print(ai, true);
}

#ifdef ARM32
static void set_abort_info(uint32_t abort_type, struct thread_abort_regs *regs,
		struct abort_info *ai)
{
	switch (abort_type) {
	case ABORT_TYPE_DATA:
		ai->fault_descr = read_dfsr();
		ai->va = read_dfar();
		break;
	case ABORT_TYPE_PREFETCH:
		ai->fault_descr = read_ifsr();
		ai->va = read_ifar();
		break;
	default:
		ai->fault_descr = 0;
		ai->va = regs->elr;
		break;
	}
	ai->abort_type = abort_type;
	ai->pc = regs->elr;
	ai->regs = regs;
}
#endif /*ARM32*/

bool abort_is_user_exception(struct abort_info *ai __unused)
{
	return false;
}

#ifdef ARM32
/* Returns true if the exception originated from abort mode */
static bool is_abort_in_abort_handler(struct abort_info *ai)
{
	return (ai->regs->spsr & ARM32_CPSR_MODE_MASK) == ARM32_CPSR_MODE_ABT;
}
#endif /*ARM32*/

static enum fault_type get_fault_type(struct abort_info *ai)
{
	if (is_abort_in_abort_handler(ai)) {
		abort_print_error(ai);
		panic("[abort] abort in abort handler (trap CPU)");
	}

	if (ai->abort_type == ABORT_TYPE_UNDEF) {
		if (abort_is_user_exception(ai))
			return FAULT_TYPE_USER_TA_PANIC;
		abort_print_error(ai);
		panic("[abort] undefined abort (trap CPU)");
	}

	switch (core_mmu_get_fault_type(ai->fault_descr)) {
	case CORE_MMU_FAULT_ALIGNMENT:
		if (abort_is_user_exception(ai))
			return FAULT_TYPE_USER_TA_PANIC;
		abort_print_error(ai);
		panic("[abort] alignement fault!  (trap CPU)");
		break;

	case CORE_MMU_FAULT_ACCESS_BIT:
		if (abort_is_user_exception(ai))
			return FAULT_TYPE_USER_TA_PANIC;
		abort_print_error(ai);
		panic("[abort] access bit fault!  (trap CPU)");
		break;

	case CORE_MMU_FAULT_DEBUG_EVENT:
		abort_print(ai);
		DMSG("[abort] Ignoring debug event!");
		return FAULT_TYPE_IGNORE;

	case CORE_MMU_FAULT_TRANSLATION:
	case CORE_MMU_FAULT_WRITE_PERMISSION:
	case CORE_MMU_FAULT_READ_PERMISSION:
		return FAULT_TYPE_PAGEABLE;

	case CORE_MMU_FAULT_ASYNC_EXTERNAL:
		abort_print(ai);
		DMSG("[abort] Ignoring async external abort!");
		return FAULT_TYPE_IGNORE;

	case CORE_MMU_FAULT_OTHER:
	default:
		abort_print(ai);
		DMSG("[abort] Unhandled fault!");
		return FAULT_TYPE_IGNORE;
	}
}

void abort_handler(uint32_t abort_type, struct thread_abort_regs *regs)
{
	struct abort_info ai;

	set_abort_info(abort_type, regs, &ai);

	switch (get_fault_type(&ai)) {
	case FAULT_TYPE_IGNORE:
		break;
	case FAULT_TYPE_USER_TA_PANIC:
		DMSG("[abort] abort in User mode (TA will panic)");
		abort_print_error(&ai);
		break;
	case FAULT_TYPE_PAGEABLE:
	default:
		panic("unhandled pageable abort");
		break;
	}
}
