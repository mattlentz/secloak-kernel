/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell                                                                                                       
 * Copyright 2010, 2011 David Jander <david@protonic.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

// Retained the above copyright/licensing notices from the GPIO keypad driver in
// the Linux kernel, which this driver is based on. Relevant files:
//   drivers/input/keyboard/gpio_keys.c

#include <drivers/imx_gpio_keys.h>

#include <drivers/dt.h>
#include <drivers/imx_gpio.h>
#include <errno.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <io.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <malloc.h>
#include <stdlib.h>
#include <util.h>

struct gpio_keys_button {
	unsigned int code;
	const char *desc;
	struct irq_desc gpio_irq;
	uint32_t gpio_irq_flags;
	struct irq_handler gpio_irq_handler;
};

struct gpio_keys_platform_data {
	struct gpio_keys_button *buttons;
	int nbuttons;
	int (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	SLIST_HEAD(, button_handler) handlers;
	const char *name;
};

static struct gpio_keys_platform_data *pdata = NULL;

static enum irq_return gpio_keys_handler(struct irq_handler *handler) {
	struct gpio_keys_button *button = handler->data;	

	bool consume = false;
	const struct button_handler *bh;
	SLIST_FOREACH(bh, &pdata->handlers, entry) {
		consume |= bh->on_press(button->code);
	}

	return consume ? ITRR_HANDLED : ITRR_HANDLED_PASS;
}

bool gpio_keys_acquire(struct button_handler *handler) {
	if (SLIST_EMPTY(&pdata->handlers)) {
		for (int b = 0; b < pdata->nbuttons; b++) {
			struct gpio_keys_button *button = &pdata->buttons[b];

			// Apparently, the flags in the DT should be ignored, and the
			// interrupt should be triggered on both edges instead (0x3)
			irq_add(&button->gpio_irq, 0x3, &button->gpio_irq_handler);
			irq_enable(&button->gpio_irq);
		}
	}

	SLIST_INSERT_HEAD(&pdata->handlers, handler, entry);

	return true;
}

void gpio_keys_release(struct button_handler *handler) {
	const struct button_handler *h;
	SLIST_FOREACH(h, &pdata->handlers, entry) {
		if (h == handler) {
			SLIST_REMOVE(&pdata->handlers, h, button_handler, entry);
		}
	}

	if (SLIST_EMPTY(&pdata->handlers)) {
		for (int b = 0; b < pdata->nbuttons; b++) {
			struct gpio_keys_button *button = &pdata->buttons[b];
			irq_disable(&button->gpio_irq);
			irq_remove(&button->gpio_irq);
		}
	}
}

enum secpwr_state {
	SECPWR_STATE0,
	SECPWR_STATE1,
	SECPWR_STATE2,
	SECPWR_STATE3
};

static enum secpwr_state secpwr_state;

static bool secpwr_button_press_handler(int code) {
	enum secpwr_state next_state = SECPWR_STATE0;
	switch(secpwr_state) {
		case SECPWR_STATE0:
			if (code == KEY_HOMEPAGE) {
				next_state = SECPWR_STATE1;
			}
			break;
		case SECPWR_STATE1:
			if (code == KEY_BACK) {
				next_state = SECPWR_STATE2;
			} else if (code == KEY_HOMEPAGE) {
				next_state = SECPWR_STATE1;
			}
			break;
		case SECPWR_STATE2:
			if (code == KEY_VOLUMEUP) {
				next_state = SECPWR_STATE3;
			} else if (code == KEY_BACK) {
				next_state = SECPWR_STATE2;
			}
			break;
		case SECPWR_STATE3:
			if (code == KEY_VOLUMEDOWN) {
				DMSG("[SecPower] Button sequence was pressed... Shutting down!");
				// TODO: Shutdown via PSCI function
			} else if (code == KEY_VOLUMEUP) {
				next_state = SECPWR_STATE3;
			}
			break;
	}

	secpwr_state = next_state;
	return false;
}

static struct button_handler secpwr_button_handler = {
	.on_press = secpwr_button_press_handler,
};

static int gpio_keys_probe(const void *fdt, struct device *dev, const void *data __unused)
{
	int node;

	int nbuttons = 0;
	fdt_for_each_subnode(node, fdt, dev->node) {
		nbuttons++;
	}

	if (nbuttons == 0) {
		return -ENODEV;
	}

	if (!(pdata = malloc(sizeof(*pdata)))) {
		EMSG("[GPIOKeys] Could not allocate memory for driver structure for device %s\n", dev->name);
		return -ENOMEM;
	}
	memset(pdata, 0, sizeof(*pdata));

	pdata->nbuttons = nbuttons;
	pdata->buttons = malloc(sizeof(*pdata->buttons) * nbuttons);
	if (!pdata->buttons) {
		EMSG("[GPIOKeys] Could not allocate memory for driver structure for device %s\n", dev->name);
		return -ENOMEM;
	}
	memset(pdata->buttons, 0, sizeof(*pdata->buttons) * nbuttons);
	SLIST_INIT(&pdata->handlers);

	int n = 0;
	fdt_for_each_subnode(node, fdt, dev->node) {
		struct gpio_keys_button *button = &pdata->buttons[n];

		if (dt_read_property_u32(fdt, node, "linux,code", &button->code)) {
			EMSG("[GPIOKeys] Device '%s' has a button without a keycode", dev->name);
			return -EINVAL;
		}

		button->desc = dt_read_property(fdt, node, "label");

		const fdt32_t *gpio_dt_spec = dt_read_property(fdt, node, "gpios");
		struct device *gpio_dev = dt_lookup_device(fdt, gpio_dt_spec[0]);
		struct irq_chip *gpio_chip = irq_find_chip(gpio_dev);
		if (gpio_chip == NULL) {
			EMSG("[GPIOKeys] Device '%s' has button '%s' without a valid GPIO phandle", dev->name, button->desc);
			return -EINVAL;
		}

		button->gpio_irq.chip = gpio_chip;
		irq_map(gpio_chip, &gpio_dt_spec[1], &button->gpio_irq.irq, &button->gpio_irq_flags);
		button->gpio_irq_handler.handle = gpio_keys_handler;
		button->gpio_irq_handler.data = (void *)button;

		n++;
	}
	
	IMSG("Registered GPIOKeys with buttons:");
	for (int b = 0; b < nbuttons; b++) {
		IMSG("\t %s", pdata->buttons[b].desc);
	}
	
	if (!gpio_keys_acquire(&secpwr_button_handler)) {
		EMSG("[GPIOKeys] Could not acquire keys for secure power down sequence");
		return -EINVAL;
	}

	return 0;
}

static const struct dt_device_match gpio_keys_match_table[] = {
	{ .compatible = "gpio-keys", },
	{ 0 }
};

const struct dt_driver gpio_keys_dt_driver __dt_driver = {
	.name = "gpio-keys",
	.match_table = gpio_keys_match_table,
	.probe = gpio_keys_probe,
};

