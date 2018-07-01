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

#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <errno.h>
#include <malloc.h>
#include <trace.h>

static SLIST_HEAD(, irq_chip) chips = SLIST_HEAD_INITIALIZER(chips);

int32_t irq_construct_chip(struct irq_chip *chip, struct device *dev, const struct irq_chip_ops *ops, size_t num_irqs, void *data, bool default_handler) {
	chip->dev = dev;
	chip->ops = ops;
	chip->num_irqs = num_irqs;
	chip->data = data;
	chip->default_handler = default_handler;
	chip->handlers = malloc(num_irqs * sizeof(*chip->handlers));
	if (!chip->handlers) {
		return -ENOMEM;
	}

	memset(chip->handlers, 0, num_irqs * sizeof(*chip->handlers));
	SLIST_INSERT_HEAD(&chips, chip, entry);

	return 0;
}

void irq_destruct_chip(struct irq_chip *chip) {
	SLIST_REMOVE(&chips, chip, irq_chip, entry);
	free(chip->handlers);
}

struct irq_chip *irq_find_chip(struct device *dev) {
	struct irq_chip *chip;
	SLIST_FOREACH(chip, &chips, entry) {
		if (chip->dev == dev) {
			break;
		}
	}

	return chip;
}

static bool irq_check_valid(struct irq_chip *chip, size_t irq) {
	if (irq >= chip->num_irqs) {
		DMSG("[IRQ] Invalid IRQ %d", irq);
		panic();
		return false;
	}

	return true;
}

enum irq_return irq_handle(struct irq_chip *chip, size_t irq)
{
	if (!irq_check_valid(chip, irq)) {
		return ITRR_NONE;
	}

	struct irq_handler *handler = chip->handlers[irq];
	if (!handler) {
		if (chip->default_handler) {
			return ITRR_HANDLED_DEFAULT;
		} else {
			EMSG("[IRQ] Disabling unhandled interrupt %zu", irq);
			chip->ops->disable(chip, irq);
			return ITRR_NONE;
		}
	}

	enum irq_return ret = handler->handle(handler);
	if (ret == ITRR_NONE) {
		EMSG("[IRQ] Disabling interrupt %zu not handled by handler", irq);
		chip->ops->disable(chip, irq);
	}

	return ret;
}

int32_t irq_map(struct irq_chip *chip, const fdt32_t *dt_spec, size_t *irq, uint32_t *flags) {
	return chip->ops->map(chip, dt_spec, irq, flags);
}

int32_t irq_add(struct irq_desc *desc, uint32_t flags, struct irq_handler *handler) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	chip->handlers[irq] = handler;
	return chip->ops->add(chip, irq, flags);
}

int32_t irq_remove(struct irq_desc *desc) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	int32_t error = chip->ops->remove(chip, irq);
	if (!error) {
		chip->handlers[irq] = NULL;
	}

	return error;
}

int32_t irq_enable(struct irq_desc *desc) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	return chip->ops->enable(chip, irq);
}

int32_t irq_disable(struct irq_desc *desc) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	return chip->ops->disable(chip, irq);
}

int32_t irq_secure(struct irq_desc *desc) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	return chip->ops->secure(chip, irq);
}

int32_t irq_unsecure(struct irq_desc *desc) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	return chip->ops->unsecure(chip, irq);
}

int32_t irq_raise(struct irq_desc *desc) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	return chip->ops->raise(chip, irq);
}

int32_t irq_raise_sgi(struct irq_desc *desc, uint8_t cpu_mask) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	if (chip->ops->raise_sgi) {
		return -EINVAL;
	}

	return chip->ops->raise_sgi(chip, irq, cpu_mask);
}

int32_t irq_set_affinity(struct irq_desc *desc, uint8_t cpu_mask) {
	struct irq_chip *chip = desc->chip;
	size_t irq = desc->irq;
	if (!irq_check_valid(chip, irq)) {
		return -EINVAL;
	}

	if (chip->ops->set_affinity) {
		return -EINVAL;
	}

	return chip->ops->set_affinity(chip, irq, cpu_mask);
}
