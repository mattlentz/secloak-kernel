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
#ifndef OPTEE_SMC_H
#define OPTEE_SMC_H

/*
 * This file is exported by OP-TEE and is in kept in sync between secure
 * world and normal world kernel driver. We're following ARM SMC Calling
 * Convention as specified in
 * http://infocenter.arm.com/help/topic/com.arm.doc.den0028a/index.html
 *
 * This file depends on optee_msg.h being included to expand the SMC id
 * macros below.
 */

#define OPTEE_SMC_32			0
#define OPTEE_SMC_64			0x40000000
#define OPTEE_SMC_FAST_CALL		0x80000000
#define OPTEE_SMC_STD_CALL		0

#define OPTEE_SMC_OWNER_MASK		0x3F
#define OPTEE_SMC_OWNER_SHIFT		24

#define OPTEE_SMC_FUNC_MASK		0xFFFF

#define OPTEE_SMC_IS_FAST_CALL(smc_val)	((smc_val) & OPTEE_SMC_FAST_CALL)
#define OPTEE_SMC_IS_64(smc_val)	((smc_val) & OPTEE_SMC_64)
#define OPTEE_SMC_FUNC_NUM(smc_val)	((smc_val) & OPTEE_SMC_FUNC_MASK)
#define OPTEE_SMC_OWNER_NUM(smc_val) \
	(((smc_val) >> OPTEE_SMC_OWNER_SHIFT) & OPTEE_SMC_OWNER_MASK)

#define OPTEE_SMC_CALL_VAL(type, calling_convention, owner, func_num) \
			((type) | (calling_convention) | \
			(((owner) & OPTEE_SMC_OWNER_MASK) << \
				OPTEE_SMC_OWNER_SHIFT) |\
			((func_num) & OPTEE_SMC_FUNC_MASK))

#define OPTEE_SMC_STD_CALL_VAL(func_num) \
	OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_STD_CALL, \
			   OPTEE_SMC_OWNER_TRUSTED_OS, (func_num))
#define OPTEE_SMC_FAST_CALL_VAL(func_num) \
	OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_FAST_CALL, \
			   OPTEE_SMC_OWNER_TRUSTED_OS, (func_num))

#define OPTEE_SMC_OWNER_ARCH		0
#define OPTEE_SMC_OWNER_CPU		1
#define OPTEE_SMC_OWNER_SIP		2
#define OPTEE_SMC_OWNER_OEM		3
#define OPTEE_SMC_OWNER_STANDARD	4
#define OPTEE_SMC_OWNER_TRUSTED_APP	48
#define OPTEE_SMC_OWNER_TRUSTED_OS	50

#define OPTEE_SMC_OWNER_TRUSTED_OS_OPTEED 62
#define OPTEE_SMC_OWNER_TRUSTED_OS_API	63

/* Returned in a0 */
#define OPTEE_SMC_RETURN_UNKNOWN_FUNCTION 0xFFFFFFFF

/* Returned in a0 only from Trusted OS functions */
#define OPTEE_SMC_RETURN_OK		0x0
#define OPTEE_SMC_RETURN_ETHREAD_LIMIT	0x1
#define OPTEE_SMC_RETURN_EBUSY		0x2
#define OPTEE_SMC_RETURN_ERESUME	0x3
#define OPTEE_SMC_RETURN_EBADADDR	0x4
#define OPTEE_SMC_RETURN_EBADCMD	0x5
#define OPTEE_SMC_RETURN_ENOMEM		0x6
#define OPTEE_SMC_RETURN_ENOTAVAIL	0x7
#define OPTEE_SMC_RETURN_IS_RPC(ret) \
	(((ret) != OPTEE_SMC_RETURN_UNKNOWN_FUNCTION) && \
	((((ret) & OPTEE_SMC_RETURN_RPC_PREFIX_MASK) == \
		OPTEE_SMC_RETURN_RPC_PREFIX)))

#endif /* OPTEE_SMC_H */
