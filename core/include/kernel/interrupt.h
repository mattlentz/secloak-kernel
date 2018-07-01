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
#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include <types_ext.h>
#include <sys/queue.h>
#include <libfdt.h>

#define ITRF_TRIGGER_LEVEL	(1 << 0)

enum irq_return {
	ITRR_NONE,
	ITRR_HANDLED,
	ITRR_HANDLED_PASS,
	ITRR_HANDLED_DEFAULT
};

struct irq_handler {
	void *data;
	enum irq_return (*handle)(struct irq_handler *h);
};

struct irq_chip;
struct irq_chip_ops {
	int32_t (*map)(struct irq_chip *chip, const fdt32_t *dt_spec, size_t *irq, uint32_t *flags);
	int32_t (*add)(struct irq_chip *chip, size_t irq, uint32_t flags);
	int32_t (*remove)(struct irq_chip *chip, size_t irq);
	int32_t (*enable)(struct irq_chip *chip, size_t irq);
	int32_t (*disable)(struct irq_chip *chip, size_t irq);
	int32_t (*secure)(struct irq_chip *chip, size_t irq);
	int32_t (*unsecure)(struct irq_chip *chip, size_t irq);
	int32_t (*raise)(struct irq_chip *chip, size_t irq);
	int32_t (*raise_sgi)(struct irq_chip *chip, size_t irq, uint8_t cpu_mask);
	int32_t (*set_affinity)(struct irq_chip *chip, size_t irq, uint8_t cpu_mask);
};

struct irq_chip {
	struct device *dev;
	const struct irq_chip_ops *ops;
	size_t num_irqs;
	bool default_handler;
	struct irq_handler **handlers;
	void *data;
	SLIST_ENTRY(irq_chip) entry;
};

struct irq_desc {
	struct irq_chip *chip;
	size_t irq;
};

int32_t irq_construct_chip(struct irq_chip *chip, struct device *dev, const struct irq_chip_ops *ops, size_t num_irqs, void *data, bool default_handler);
void irq_destruct_chip(struct irq_chip *chip);
struct irq_chip *irq_find_chip(struct device *dev);

enum irq_return irq_handle(struct irq_chip *chip, size_t irq);

int32_t irq_map(struct irq_chip *chip, const fdt32_t *dt_spec, size_t *irq, uint32_t *flags);
int32_t irq_add(struct irq_desc *desc, uint32_t flags, struct irq_handler *handler);
int32_t irq_remove(struct irq_desc *desc);
int32_t irq_enable(struct irq_desc *desc);
int32_t irq_disable(struct irq_desc *desc);
int32_t irq_secure(struct irq_desc *desc);
int32_t irq_unsecure(struct irq_desc *desc);
int32_t irq_raise(struct irq_desc *desc);
int32_t irq_raise_sgi(struct irq_desc *desc, uint8_t cpu_mask);
int32_t irq_set_affinity(struct irq_desc *desc, uint8_t cpu_mask);

#endif /*__KERNEL_INTERRUPT_H*/
