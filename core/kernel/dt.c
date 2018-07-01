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
#include <kernel/dt.h>
#include <drivers/dt.h>
#include <kernel/linker.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <string.h>
#include <trace.h>

const struct dt_driver *dt_find_compatible_driver(const void *fdt, int offs)
{
	const struct dt_device_match *dm;
	const struct dt_driver *drv;

	for_each_dt_driver(drv)
		for (dm = drv->match_table; dm; dm++)
			if (!fdt_node_check_compatible(fdt, offs,
						       dm->compatible))
				return drv;

	return NULL;
}

int dt_probe_compatible_driver(const void *fdt, struct device *dev)
{
	const struct dt_driver *driver;

	for_each_dt_driver(driver) {
		for (const struct dt_device_match *match = driver->match_table; match->compatible != NULL; match++) {
			if (!fdt_node_check_compatible(fdt, dev->node, match->compatible)) {
				return driver->probe(fdt, dev, match->data);
			}
		}
	}

	return 0;
}

const struct dt_driver *__dt_driver_start(void)
{
	return &__rodata_dtdrv_start;
}

const struct dt_driver *__dt_driver_end(void)
{
	return &__rodata_dtdrv_end;
}

const void* dt_read_property(const void *fdt, int offs, const char *name) {
	const void *prop;
	prop = fdt_getprop(fdt, offs, name, NULL);

	return prop;
}

int dt_read_property_u32(const void *fdt, int offs, const char *name, uint32_t *out) {
	const void *prop;
	int prop_size;

	prop = fdt_getprop(fdt, offs, name, &prop_size);
	if (!prop || (prop_size != sizeof(uint32_t))) {
		return -1;
	}

	*out = fdt32_to_cpu(*((uint32_t*)prop));
	return 0;
}

static bool is_okay(const char *st, int len)
{
	return !strncmp(st, "ok", len) || !strncmp(st, "okay", len);
}

int _fdt_get_status(const void *fdt, int offs)
{
	const char *prop;
	int st = 0;
	int len;

	prop = fdt_getprop(fdt, offs, "status", &len);
	if (!prop || is_okay(prop, len)) {
		st |= DT_STATUS_OK_NSEC | DT_STATUS_OK_SEC;
	}

	return st;
}
