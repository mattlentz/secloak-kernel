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

#ifndef KERNEL_DT_H
#define KERNEL_DT_H

#include <compiler.h>
#include <stdint.h>
#include <types_ext.h>
#include <util.h>
#include <drivers/dt.h>

/*
 * Bitfield to reflect status and secure-status values ("ok", "disabled" or not
 * present)
 */
#define DT_STATUS_DISABLED 0
#define DT_STATUS_OK_NSEC  BIT(0)
#define DT_STATUS_OK_SEC   BIT(1)

/*
 * DT-aware drivers
 */

struct dt_device_match {
	const char *compatible;
	const void *data;
};

struct device;

struct dt_driver {
	const char *name;
	const struct dt_device_match *match_table; /* null-terminated */
	const void *driver;
	int (*probe)(const void *fdt, struct device *dev, const void *data);
};

#define __dt_driver __section(".rodata.dtdrv")

/*
 * Find a driver that is suitable for the given DT node, that is, with
 * a matching "compatible" property.
 *
 * @fdt: pointer to the device tree
 * @offs: node offset
 */
const struct dt_driver *dt_find_compatible_driver(const void *fdt, int offs);
int dt_probe_compatible_driver(const void *fdt, struct device *dev);

const struct dt_driver *__dt_driver_start(void);

const struct dt_driver *__dt_driver_end(void);

const void* dt_read_property(const void *fdt, int offs, const char *name);
int dt_read_property_u32(const void *fdt, int offs, const char *name, uint32_t *out);

/*
 * FDT manipulation functions, not provided by <libfdt.h>
 */

/*
 * Read the status and secure-status properties into a bitfield.
 * @status is set to DT_STATUS_DISABLED or a combination of DT_STATUS_OK_NSEC
 * and DT_STATUS_OK_SEC
 * Returns 0 on success or -1 in case of error.
 */
int _fdt_get_status(const void *fdt, int offs);

#define for_each_dt_driver(drv) \
	for (drv = __dt_driver_start(); drv < __dt_driver_end(); drv++)

#endif /* KERNEL_DT_H */
