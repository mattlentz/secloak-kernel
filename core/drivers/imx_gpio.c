/*                                                                                                                                    
 * MXC GPIO support. (c) 2008 Daniel Mack <daniel@caiaq.de>
 * Copyright 2008 Juergen Beisert, kernel@pengutronix.de
 *
 * Based on code from Freescale,
 * Copyright (C) 2004-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Retained the above copyright/licensing notices from the GPIO driver in
// the Linux kernel, which this driver is based on. Relevant files:
//   drivers/gpio/gpio-mxc.c

#include <drivers/imx_gpio.h>

#include <drivers/imx_csu.h>
#include <drivers/dt.h>
#include <errno.h>
#include <io.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <malloc.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <secloak/emulation.h>
#include <stdlib.h>
#include <util.h>

enum mxc_gpio_hwtype {
	IMX1_GPIO = 1,  /* runs on i.mx1 */
	IMX21_GPIO, /* runs on i.mx21 and i.mx27 */
	IMX31_GPIO, /* runs on i.mx31 */
	IMX35_GPIO, /* runs on all other i.mx */
};

struct mxc_gpio_hwdata {
	unsigned dr_reg;
	unsigned gdir_reg;
	unsigned psr_reg;
	unsigned icr1_reg;
	unsigned icr2_reg;
	unsigned imr_reg;
	unsigned isr_reg;
	int edge_sel_reg;
	unsigned low_level;
	unsigned high_level;
	unsigned rise_edge;
	unsigned fall_edge;
};

static int32_t irq_op_map(struct irq_chip *chip, const fdt32_t *dt_spec, size_t *irq, uint32_t *flags);
static int32_t irq_op_add(struct irq_chip *chip, size_t it, uint32_t flags);
static int32_t irq_op_remove(struct irq_chip *chip, size_t irq);
static int32_t irq_op_enable(struct irq_chip *chip, size_t it);
static int32_t irq_op_disable(struct irq_chip *chip, size_t it);
static int32_t irq_op_secure(struct irq_chip *chip, size_t it);
static int32_t irq_op_unsecure(struct irq_chip *chip, size_t it);
static int32_t irq_op_raise(struct irq_chip *chip, size_t it);

static const struct irq_chip_ops irq_ops = {
	.map = irq_op_map,
	.add = irq_op_add,
	.remove = irq_op_remove,
	.enable = irq_op_enable,
	.disable = irq_op_disable,
	.secure = irq_op_secure,
	.unsecure = irq_op_unsecure,
	.raise = irq_op_raise,
};

struct mxc_gpio_port {
	SLIST_ENTRY(mxc_gpio_port) node;
	struct device *dev;
	paddr_t pbase;
	vaddr_t base;
	struct irq_desc *irq;
	struct irq_handler handler;
	struct irq_desc *irq_high;
	struct irq_handler handler_high;
	struct irq_desc *irq_pass;
	struct irq_chip irq_chip;
	uint32_t both_edges;
	uint32_t imr_previous;
	uint32_t secure_mask;
	uint32_t secure_irq_mask;
	uint32_t passed_irq_mask;
};

static struct mxc_gpio_hwdata imx1_imx21_gpio_hwdata = {
	.dr_reg   = 0x1c,
	.gdir_reg = 0x00,
	.psr_reg  = 0x24,
	.icr1_reg = 0x28,
	.icr2_reg = 0x2c,
	.imr_reg  = 0x30,
	.isr_reg  = 0x34,
	.edge_sel_reg = -EINVAL,
	.low_level  = 0x03,
	.high_level = 0x02,
	.rise_edge  = 0x00,
	.fall_edge  = 0x01,
};

static struct mxc_gpio_hwdata imx31_gpio_hwdata = {
	.dr_reg   = 0x00,
	.gdir_reg = 0x04,
	.psr_reg  = 0x08,
	.icr1_reg = 0x0c,
	.icr2_reg = 0x10,
	.imr_reg  = 0x14,
	.isr_reg  = 0x18,
	.edge_sel_reg = -EINVAL,
	.low_level  = 0x00,
	.high_level = 0x01,
	.rise_edge  = 0x02,
	.fall_edge  = 0x03,
};

static struct mxc_gpio_hwdata imx35_gpio_hwdata = {
	.dr_reg   = 0x00,
	.gdir_reg = 0x04,
	.psr_reg  = 0x08,
	.icr1_reg = 0x0c,
	.icr2_reg = 0x10,
	.imr_reg  = 0x14,
	.isr_reg  = 0x18,
	.edge_sel_reg = 0x1c,
	.low_level  = 0x00,
	.high_level = 0x01,
	.rise_edge  = 0x02,
	.fall_edge  = 0x03,
};

static enum mxc_gpio_hwtype mxc_gpio_hwtype;
static struct mxc_gpio_hwdata *mxc_gpio_hwdata = NULL;
static SLIST_HEAD(ports, mxc_gpio_port) mxc_gpio_ports = SLIST_HEAD_INITIALIZER(mxc_gpio_ports);

#define GPIO_DR (mxc_gpio_hwdata->dr_reg)
#define GPIO_GDIR (mxc_gpio_hwdata->gdir_reg)
#define GPIO_PSR (mxc_gpio_hwdata->psr_reg)
#define GPIO_ICR1 (mxc_gpio_hwdata->icr1_reg)
#define GPIO_ICR2 (mxc_gpio_hwdata->icr2_reg)
#define GPIO_IMR (mxc_gpio_hwdata->imr_reg)
#define GPIO_ISR (mxc_gpio_hwdata->isr_reg)
#define GPIO_EDGE_SEL (mxc_gpio_hwdata->edge_sel_reg)

#define GPIO_INT_LOW_LEV (mxc_gpio_hwdata->low_level)
#define GPIO_INT_HIGH_LEV (mxc_gpio_hwdata->high_level)
#define GPIO_INT_RISE_EDGE (mxc_gpio_hwdata->rise_edge)
#define GPIO_INT_FALL_EDGE (mxc_gpio_hwdata->fall_edge)
#define GPIO_INT_BOTH_EDGES 0x4

#define GPIO_DIR_INPUT 0
#define GPIO_DIR_OUTPUT 1

enum {
	IRQ_TYPE_NONE		= 0x00000000,
	IRQ_TYPE_EDGE_RISING	= 0x00000001,
	IRQ_TYPE_EDGE_FALLING	= 0x00000002,
	IRQ_TYPE_EDGE_BOTH	= (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
	IRQ_TYPE_LEVEL_HIGH	= 0x00000004,
	IRQ_TYPE_LEVEL_LOW	= 0x00000008,
};

static int gpio_get_value(struct mxc_gpio_port *port, int index)
{
	uint32_t psr = read32(port->base + GPIO_PSR);
	return psr & (1 << index);
}

static void gpio_set_dir(struct mxc_gpio_port *port, int index, int direction) {
	uint32_t gdir = read32(port->base + GPIO_GDIR);
	gdir = (gdir & ~(1 << index)) | (direction << index);
	write32(gdir, port->base + GPIO_GDIR);
}

static void gpio_irq_mask(struct mxc_gpio_port *port, int index, bool mask) {
	uint32_t imr = read32(port->base + GPIO_IMR);
	if (mask) {
		write32(imr & ~(1 << index), port->base + GPIO_IMR);
	} else {
		write32(imr | (1 << index), port->base + GPIO_IMR);
	}
}

static void gpio_irq_ack(struct mxc_gpio_port *port, int index) {
	write32(1 << index, port->base + GPIO_ISR);
}

static int gpio_set_irq_type(struct mxc_gpio_port *port, int index, uint32_t type)
{
	uint32_t bit, val;
	int edge;

	port->both_edges &= ~(1 << index);
	switch (type) {
		case IRQ_TYPE_EDGE_RISING:
			edge = GPIO_INT_RISE_EDGE;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			edge = GPIO_INT_FALL_EDGE;
			break;
		case IRQ_TYPE_EDGE_BOTH:
			if (GPIO_EDGE_SEL >= 0) {
				edge = GPIO_INT_BOTH_EDGES;
			} else {
				val = gpio_get_value(port, index);
				if (val) {
					edge = GPIO_INT_LOW_LEV;
				} else {
					edge = GPIO_INT_HIGH_LEV;
				}
				port->both_edges |= 1 << index;
			}
			break;
		case IRQ_TYPE_LEVEL_LOW:
			edge = GPIO_INT_LOW_LEV;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			edge = GPIO_INT_HIGH_LEV;
			break;
		default:
			return -EINVAL;
	}

	if (GPIO_EDGE_SEL >= 0) {
		val = read32(port->base + GPIO_EDGE_SEL);
		if (edge == GPIO_INT_BOTH_EDGES)
			write32(val | (1 << index), port->base + GPIO_EDGE_SEL);
		else
			write32(val & ~(1 << index), port->base + GPIO_EDGE_SEL);
	}

	if (edge != GPIO_INT_BOTH_EDGES) {
		vaddr_t reg = port->base + GPIO_ICR1 + ((index & 0x10) >> 2);
		bit = index & 0xf;
		val = read32(reg) & ~(0x3 << (bit << 1));
		write32(val | (edge << (bit << 1)), reg);
	}

	write32(1 << index, port->base + GPIO_ISR);

	return 0;
}

static struct mxc_gpio_port *gpio_port_from_phandle(uint32_t phandle) {
	struct mxc_gpio_port *port;
	SLIST_FOREACH(port, &mxc_gpio_ports, node) {
		if (port->dev->phandle == phandle) {
			break;
		}
	}

	return port;
}

struct mxc_gpio_port *gpio_port_from_address(paddr_t base) {
	struct mxc_gpio_port *port;
	SLIST_FOREACH(port, &mxc_gpio_ports, node) {
		if (port->pbase == base) {
			break;
		}
	}

	return port;
}

static bool gpio_emu_check(struct region *region, paddr_t address, enum emu_state state, uint32_t *value) {
	struct mxc_gpio_port *port = gpio_port_from_address(region->base);
	paddr_t offset = address - region->base;

	bool allowed = true;
	if (offset == GPIO_DR) {
		switch(state) {
			case EMU_STATE_WRITE:
				if ((read32(port->base + GPIO_DR) & port->secure_mask) != (*value & port->secure_mask)) {
					allowed = false;
				}
				break;
			case EMU_STATE_READ_AFTER:
				*value &= ~port->secure_mask;
				break;
			default:
				break;
		}
	} else if (offset == GPIO_GDIR) {
		switch (state) {
			case EMU_STATE_WRITE:
				if ((read32(port->base + GPIO_GDIR) & port->secure_mask) != (*value & port->secure_mask)) {
					allowed = false;
				}
				break;
			case EMU_STATE_READ_AFTER:
				*value &= ~port->secure_mask;
				break;
			default:
				break;
		}
	} else if (offset == GPIO_ISR) {
		switch (state) {
			case EMU_STATE_WRITE:
				if ((*value & port->secure_mask) != 0) {
					if ((*value & port->passed_irq_mask) == *value) {
						port->passed_irq_mask &= ~(*value);
						write32(read32(port->base + GPIO_IMR) | *value, port->base + GPIO_IMR);
					} else {
						allowed = false;
					}
				}
				break;
			case EMU_STATE_READ_AFTER:
				*value = (*value & ~port->secure_mask) | port->passed_irq_mask;
				break;
			default:
				break;
		}
	} else if (offset == GPIO_IMR) {
		switch (state) {
			case EMU_STATE_WRITE:
				if ((~(*value) & port->secure_mask) != 0) {
					allowed = false;
				}
				break;
			case EMU_STATE_READ_AFTER:
				*value = (*value & ~port->secure_mask) | port->passed_irq_mask;
				break;
			default:
				break;
		}
	} else if (offset == GPIO_PSR) {
		// FIXME: Linux does strange things with this after acking via ISR. Allowing it for now...
		/*
		if (state == EMU_STATE_READ_AFTER) {
			*value &= ~port->secure_mask | port->passed_irq_mask;
		}
		*/
	} else if (offset == (uint32_t)GPIO_EDGE_SEL) {
		switch (state) {
			case EMU_STATE_WRITE:
				if (((*value) & port->secure_mask) != (read32(port->base + GPIO_EDGE_SEL) & port->secure_mask)) {
					allowed = false;
				}
				break;
			default:
				break;
		}
	} else if (offset == GPIO_ICR1 || offset == GPIO_ICR2) {
		uint32_t secure_icr_mask = 0;
		for (int i = 0; i < 16; i++) {
			if (port->secure_mask & (1 << (i + (offset == GPIO_ICR2 ? 16 : 0)))) {
				secure_icr_mask |= (0x3 << (2 * i));
			}
		}

		switch (state) {
			case EMU_STATE_WRITE:
				if (((*value) & secure_icr_mask) != (read32(port->base + offset) & secure_icr_mask)) {
					allowed = false;
				}
				break;
			default:
				break;
		}
	}

	return allowed;
}

static bool gpio_parse_dt(const uint32_t *gpio_info, struct mxc_gpio_port **port, int *index, int *flags) {
	uint32_t phandle = fdt32_to_cpu(gpio_info[0]);
	*port = gpio_port_from_phandle(phandle);
	if (*port == NULL) {
		EMSG("[GPIO] Could not find gpio_port corresponding to phandle %u", gpio_info[0]);
		return false;
	}

	*index = fdt32_to_cpu(gpio_info[1]);
	*flags = fdt32_to_cpu(gpio_info[2]);

	return true;
}

static bool __gpio_acquire(struct mxc_gpio_port *port, int index) {
	uint32_t mask = 1 << index;
	if (port->secure_mask & mask) {
		EMSG("[GPIO] Index %d is already acquired", index);
		return false;
	}
	port->secure_mask |= mask;

	IMSG("[GPIO] Acquired pin %d on port 0x%lX", index, port->pbase);

	return true;
}

bool gpio_acquire(struct mxc_gpio_port *port, int index, bool is_output) {
	if (!__gpio_acquire(port, index)) {
		return false;
	}
	
	gpio_set_dir(port, index, is_output);
	gpio_irq_mask(port, index, true);
	
	return true;
}

bool gpio_acquire_dt(const uint32_t *gpio_info, bool is_output) {
	struct mxc_gpio_port *port;
	int index, flags;
	if (!gpio_parse_dt(gpio_info, &port, &index, &flags)) {
		return false;
	}
	
	return gpio_acquire(port, index, is_output);
}

bool gpio_release(struct mxc_gpio_port *port, int index) {
	uint32_t mask = 1 << index;
	if (!(port->secure_mask & mask)) {
		EMSG("[GPIO] Index %d was not previously acquired", index);
		return false;
	}
	port->secure_mask &= ~mask;

	IMSG("[GPIO] Released pin %d on port 0x%lX", index, port->pbase);

	return true;
}

void gpio_release_dt(const uint32_t *gpio_info) {
	struct mxc_gpio_port *port;
	int index, flags;
	if (gpio_parse_dt(gpio_info, &port, &index, &flags)) {
		gpio_release(port, index);
	}
}

bool gpio_read(struct mxc_gpio_port *port, int index) {
	return !!(read32(port->base + GPIO_DR) & (1 << index));
}

void gpio_write(struct mxc_gpio_port *port, int index, bool value) {
	uint32_t mask = 1 << index;
	uint32_t dr = read32(port->base + GPIO_DR);
	if (value) {
		dr |= mask;
	} else {
		dr &= ~mask;
	}
	write32(dr, port->base + GPIO_DR);
}

static void mxc_flip_edge(struct mxc_gpio_port *port, uint32_t gpio)
{
	vaddr_t reg = port->base;
	uint32_t bit, val;
	unsigned edge;

	reg += GPIO_ICR1 + ((gpio & 0x10) >> 2); /* lower or upper register */
	bit = gpio & 0xf;
	val = read32(reg);
	edge = (val >> (bit << 1)) & 3;
	val &= ~(0x3 << (bit << 1));
	if (edge == GPIO_INT_HIGH_LEV) {
		edge = GPIO_INT_LOW_LEV;
	} else if (edge == GPIO_INT_LOW_LEV) {
		edge = GPIO_INT_HIGH_LEV;
	} else {
		EMSG("[GPIO] Invalid configuration for pin %d on port 0x%lX: %x\n", gpio, port->pbase, edge);
		return;
	}

	write32(val | (edge << (bit << 1)), reg);
}

static inline unsigned int __clz(unsigned int x) {
	unsigned int ret;
	asm("clz\t%0, %1" : "=r" (ret) : "r" (x));
	return ret;
}

static inline int fls(int x) {
	return 32 - __clz(x);
}

/* handle 32 interrupts in one status register */
static void mxc_gpio_irq_handler(struct mxc_gpio_port *port, uint32_t irq_stat)
{
	while (irq_stat != 0) {
		int irqoffset = fls(irq_stat) - 1;

		if (port->both_edges & (1 << irqoffset)) {
			mxc_flip_edge(port, irqoffset);
		}

		enum irq_return handled = irq_handle(&port->irq_chip, irqoffset);
		switch (handled) {
			case ITRR_HANDLED:
				gpio_irq_ack(port, irqoffset);
				break;
			case ITRR_HANDLED_PASS:
				port->passed_irq_mask |= (1 << irqoffset);
				gpio_irq_mask(port, irqoffset, true);
				break;
			default:
				if (!(port->secure_mask & (1 << irqoffset))) {
					port->passed_irq_mask |= (1 << irqoffset);
					gpio_irq_mask(port, irqoffset, true);
				}
				break;
		}

		irq_stat &= ~(1 << irqoffset);
	}

	if (port->passed_irq_mask) {
		irq_raise(port->irq_pass);
	}
}

/* MX1 and MX3 has one interrupt *per* gpio port */
static enum irq_return mx3_gpio_irq_handler(struct irq_handler *handler)
{
	uint32_t irq_stat;
	struct mxc_gpio_port *port = handler->data;

	irq_stat = read32(port->base + GPIO_ISR) & read32(port->base + GPIO_IMR);
	mxc_gpio_irq_handler(port, irq_stat);

	return ITRR_HANDLED;
}

/* MX2 has one interrupt *for all* gpio ports */
static enum irq_return mx2_gpio_irq_handler(struct irq_handler *handler)
{
	uint32_t irq_msk, irq_stat;
	struct mxc_gpio_port *port;
	(void)handler;

	/* walk through all interrupt status registers */
	SLIST_FOREACH(port, &mxc_gpio_ports, node) {
		irq_msk = read32(port->base + GPIO_IMR);
		if (!irq_msk)
			continue;

		irq_stat = read32(port->base + GPIO_ISR) & irq_msk;
		if (irq_stat)
			mxc_gpio_irq_handler(port, irq_stat);
	}

	return ITRR_HANDLED;
}

static void mxc_gpio_get_hw(const void *data)
{
	enum mxc_gpio_hwtype hwtype = (enum mxc_gpio_hwtype)data;

	if (mxc_gpio_hwtype && hwtype != mxc_gpio_hwtype) {
		panic();	
		return;
	}

	if (hwtype == IMX35_GPIO)
		mxc_gpio_hwdata = &imx35_gpio_hwdata;
	else if (hwtype == IMX31_GPIO)
		mxc_gpio_hwdata = &imx31_gpio_hwdata;
	else
		mxc_gpio_hwdata = &imx1_imx21_gpio_hwdata;

	mxc_gpio_hwtype = hwtype;
}

static int mxc_gpio_probe(const void *fdt __unused, struct device *dev, const void *data)
{
	struct mxc_gpio_port *port;
	if (!(port = malloc(sizeof(*port)))) {
		EMSG("[GPIO] Could not allocate memory for driver structure for device %s\n", dev->name);
		return -ENOMEM;
	}
	memset(port, 0, sizeof(*port));

	bool first_port = !mxc_gpio_hwdata;
	mxc_gpio_get_hw(data);

	if (dev->num_resources != 1 || dev->resource_type != RESOURCE_MEM) {
		EMSG("[GPIO] Resource is not valid for device %s\n", dev->name);
		return -EINVAL;
	}

	paddr_t paddr = dev->resources[0].address[0];
	size_t size = dev->resources[0].size[0];
	if (!core_mmu_add_mapping(MEM_AREA_IO_SEC, paddr, size)) {
		EMSG("[GPIO] Could not map I/O memory (paddr %lX, size %X) for device %s\n", paddr, size, dev->name);
		return -EINVAL;
	}

	port->dev = dev;
	port->pbase = paddr;
	port->base = (vaddr_t)phys_to_virt(paddr, MEM_AREA_IO_SEC);

	if (dev->num_irqs < 2 || dev->num_irqs > 3) {
		EMSG("[GPIO] IRQs are not valid for device %s\n", dev->name);
		return -EINVAL;
	}

	write32(0, port->base + GPIO_IMR);
	write32(~0, port->base + GPIO_ISR);

	port->irq = &dev->irqs[0].desc;
	port->handler.handle = mxc_gpio_hwtype == IMX21_GPIO ? mx2_gpio_irq_handler : mx3_gpio_irq_handler;
	port->handler.data = port;
	if (mxc_gpio_hwtype != IMX21_GPIO || first_port) {
		irq_add(port->irq, ITRF_TRIGGER_LEVEL, &port->handler);
		irq_enable(port->irq);
	}

	port->irq_high = dev->num_irqs == 3 ? &dev->irqs[1].desc : NULL;
	if (mxc_gpio_hwtype != IMX21_GPIO && port->irq_high) {
		port->handler_high.handle = mx3_gpio_irq_handler;
		port->handler_high.data = port;
		irq_add(port->irq_high, ITRF_TRIGGER_LEVEL, &port->handler_high);
		irq_enable(port->irq_high);
	}

	port->irq_pass = dev->num_irqs == 3 ? &dev->irqs[2].desc : &dev->irqs[1].desc;

	emu_add_region(port->pbase, port->dev->resources[0].size[0], gpio_emu_check);
	csu_set_csl(port->dev->csu[0], true);

	irq_construct_chip(&port->irq_chip, dev, &irq_ops, 32, port, true);

	SLIST_INSERT_HEAD(&mxc_gpio_ports, port, node);
	IMSG("[GPIO] Registered device %s with memory region (0x%lX, 0x%X) and FIQs %d %d\n", dev->name, paddr, size, port->irq->irq, port->irq_high->irq);

	return 0;
}

static const struct dt_device_match mxc_gpio_match_table[] = {
	{ .compatible = "fsl,imx1-gpio", .data = (void *)IMX1_GPIO, },
	{ .compatible = "fsl,imx21-gpio", .data = (void *)IMX21_GPIO, },
	{ .compatible = "fsl,imx31-gpio", .data = (void *)IMX31_GPIO, },
	{ .compatible = "fsl,imx35-gpio", .data = (void *)IMX35_GPIO, },
	{ 0 }
};

const struct dt_driver mxc_gpio_dt_driver __dt_driver = {
	.name = "gpio-mxc",
	.match_table = mxc_gpio_match_table,
	.probe = mxc_gpio_probe,
};

static int32_t irq_op_map(struct irq_chip *chip __unused, const fdt32_t *dt_spec, size_t *irq, uint32_t *flags) {
	*irq = fdt32_to_cpu(dt_spec[0]);
	*flags = fdt32_to_cpu(dt_spec[1]);

	return ((*irq < 32) && (*flags < 5)) ? 0 : -EINVAL;
}

static int32_t irq_op_add(struct irq_chip *chip, size_t irq, uint32_t flags) {
	struct mxc_gpio_port *port = chip->data;

	irq_op_secure(chip, irq);

	gpio_set_dir(port, irq, GPIO_DIR_INPUT);
	gpio_set_irq_type(port, irq, flags);
	gpio_irq_mask(port, irq, true);

	return 0;
}

static int32_t irq_op_remove(struct irq_chip *chip, size_t irq) {
	irq_op_unsecure(chip, irq);
	return 0;
}

static int32_t irq_op_enable(struct irq_chip *chip, size_t irq) {
	struct mxc_gpio_port *port = chip->data;
	gpio_irq_mask(port, irq, false);
	return 0;
}

static int32_t irq_op_disable(struct irq_chip *chip, size_t irq) {
	struct mxc_gpio_port *port = chip->data;
	gpio_irq_mask(port, irq, true);
	return 0;
}

static int32_t irq_op_secure(struct irq_chip *chip, size_t irq) {
	struct mxc_gpio_port *port = chip->data;
	port->secure_mask |= (1 << irq);
	IMSG("[GPIO] Secured IRQ %d on Port 0x%lX", irq, port->pbase);
	return 0;
}

static int32_t irq_op_unsecure(struct irq_chip *chip, size_t irq) {
	struct mxc_gpio_port *port = chip->data;
	port->secure_mask &= ~(1 << irq);
	IMSG("[GPIO] Unsecured IRQ %d on Port 0x%lX", irq, port->pbase);
	return 0;
}

static int32_t irq_op_raise(struct irq_chip *chip __unused, size_t irq __unused) {
	// Unimplemented
	return -EINVAL;
}

