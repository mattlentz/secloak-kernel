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

#include <assert.h>
#include <kernel/tee_misc.h>
#include <mm/core_mmu.h>
#include <mm/pgt_cache.h>
#include <stdlib.h>
#include <trace.h>
#include <string.h>
#include <util.h>

/*
 * With pager enabled we allocate page table from the pager.
 *
 * For LPAE each page table is a complete page which is allocated and freed
 * using the interface provided by the pager.
 *
 * For compat v7 page tables there's room for four page table in one page
 * so we need to keep track of how much of an allocated page is used. When
 * a page is completely unused it's returned to the pager.
 *
 * With pager disabled we have a static allocation of page tables instead.
 *
 * In all cases we limit the number of active page tables to
 * PGT_CACHE_SIZE.  This pool of page tables are shared between all
 * threads. In case a thread can't allocate the needed number of pager
 * tables it will release all its current tables and wait for some more to
 * be freed. A threads allocated tables are freed each time a TA is
 * unmapped so each thread should be able to allocate the needed tables in
 * turn if needed.
 */

static struct pgt_cache pgt_free_list = SLIST_HEAD_INITIALIZER(pgt_free_list);
static struct pgt pgt_entries[PGT_CACHE_SIZE];

void pgt_init(void)
{
	/*
	 * We're putting this in .nozi.* instead of .bss because .nozi.* already
	 * has a large alignment, while .bss has a small alignment. The current
	 * link script is optimized for small alignment in .bss
	 */
	static uint8_t pgt_tables[PGT_CACHE_SIZE][PGT_SIZE]
			__aligned(PGT_SIZE) __section(".nozi.pgt_cache");
	size_t n;

	for (n = 0; n < ARRAY_SIZE(pgt_tables); n++) {
		struct pgt *p = pgt_entries + n;

		p->tbl = pgt_tables[n];
		SLIST_INSERT_HEAD(&pgt_free_list, p, link);
	}
}

