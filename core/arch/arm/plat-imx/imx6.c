/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 * All rights reserved.
 * Copyright (c) 2016, Wind River Systems.
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

#include <drivers/gic.h>
#include <io.h>
#include <kernel/generic_boot.h>
#include <kernel/misc.h>
#include <kernel/tz_ssvce_pl310.h>
#include <mm/core_mmu.h>
#include <platform_config.h>
#include <drivers/tzc380.h>
#include <kernel/panic.h>

void plat_cpu_reset_late(void)
{
	if (!get_core_pos()) {
		/* primary core */
#if defined(CFG_BOOT_SYNC_CPU)
		/* set secondary entry address and release core */
		write32(CFG_TEE_LOAD_ADDR, SRC_BASE + SRC_GPR1 + 8);
		write32(CFG_TEE_LOAD_ADDR, SRC_BASE + SRC_GPR1 + 16);
		write32(CFG_TEE_LOAD_ADDR, SRC_BASE + SRC_GPR1 + 24);

		write32(SRC_SCR_CPU_ENABLE_ALL, SRC_BASE + SRC_SCR);
#endif

		/* SCU config */
		write32(SCU_INV_CTRL_INIT, SCU_BASE + SCU_INV_SEC);
		write32(SCU_SAC_CTRL_INIT, SCU_BASE + SCU_SAC);
		write32(SCU_NSAC_CTRL_INIT, SCU_BASE + SCU_NSAC);

		/* SCU enable */
		write32(read32(SCU_BASE + SCU_CTRL) | 0x1,
			SCU_BASE + SCU_CTRL);

		/* Protect region of the secure kernel with the TZASC */
		tzc_init((vaddr_t)TZC380_BASE);
		tzc_configure_region(0, 0x00000000, TZC_ATTR_SP_ALL);
		tzc_configure_region(1, CFG_TEE_RAM_START, TZC_ATTR_SP_S_RW | TZC_ATTR_REGION_SIZE(TZC_REGION_SIZE_64M) | TZC_ATTR_REGION_ENABLE);
		tzc_configure_region(2, CFG_FBMEM_START, TZC_ATTR_SP_S_RW_NS_R | TZC_ATTR_REGION_SIZE(TZC_REGION_SIZE_8M) | TZC_ATTR_REGION_ENABLE);
		tzc_set_action(TZC_ACTION_ERR);
	}
}

