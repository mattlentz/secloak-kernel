/*
 * Copyright (c) 2016, Linaro Limited
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

#include <kernel/thread.h>
#include <sm/sm.h>
#include <types_ext.h>
#include "thread_private.h"

#define DEFINES void __defines(void); void __defines(void)

#define DEFINE(def, val) \
	asm volatile("\n==>" #def " %0 " #val : : "i" (val))

DEFINES
{
#ifdef ARM32
	DEFINE(SM_NSEC_CTX_R0, offsetof(struct sm_nsec_ctx, r0));
	DEFINE(SM_NSEC_CTX_R8, offsetof(struct sm_nsec_ctx, r8));
	DEFINE(SM_NSEC_CTX_LR, offsetof(struct sm_nsec_ctx, lr));
	DEFINE(SM_SEC_CTX_R0, offsetof(struct sm_sec_ctx, r0));
	DEFINE(SM_SEC_CTX_R8, offsetof(struct sm_sec_ctx, r8));
	DEFINE(SM_SEC_CTX_MON_LR, offsetof(struct sm_sec_ctx, mon_lr));
	DEFINE(SM_CTX_SIZE, sizeof(struct sm_ctx));
	DEFINE(SM_CTX_NSEC, offsetof(struct sm_ctx, nsec));
	DEFINE(SM_CTX_SEC, offsetof(struct sm_ctx, sec));

	DEFINE(THREAD_VECTOR_TABLE_FIQ_ENTRY,
	       offsetof(struct thread_vector_table, fiq_entry));

	DEFINE(THREAD_SVC_REG_R0, offsetof(struct thread_svc_regs, r0));
	DEFINE(THREAD_SVC_REG_R5, offsetof(struct thread_svc_regs, r5));
	DEFINE(THREAD_SVC_REG_R6, offsetof(struct thread_svc_regs, r6));
#endif /*ARM32*/
}
