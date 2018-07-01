/*                                                                                                                                    
 * Copyright 2005-2016 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

// Retained the above copyright/licensing notices from the IPU driver in
// the Linux kernel, which this driver is based on. Relevant files:
//   drivers/mxc/ipu3/ipu_param_mem.h

#include <drivers/imx_fb.h>

#include <drivers/imx_csu.h>
#include <drivers/dt.h>
#include <errno.h>
#include <io.h>
#include <kernel/dt.h>
#include <kernel/panic.h>
#include <malloc.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <platform_config.h>
#include <secloak/emulation.h>
#include <string.h>

static paddr_t g_base_paddr;
static paddr_t g_base_vaddr;
static vaddr_t g_cpmem_vaddr;

#define IPU_CM_OFFSET 0x200000
#define IPU_CPMEM_OFFSET 0x300000
#define IPU_CHAN0 23 // Channel for primary flow (Buffers 0 and 1)
#define IPU_CHAN1 69 // Channel for primary flow (Buffer 2)

static uint32_t ipu_ch_param_read_field(vaddr_t base, int channel, int w, int bit, int size) {
	int i = bit / 32;
	int off = bit % 32;
	uint32_t mask = (1UL << (size)) - 1;

	int reg_offset = channel * 64;
	reg_offset += w * 32;
	reg_offset += i * 4;

	uint32_t value = read32(base + reg_offset);
	value = mask & (value >> off);

	if (((bit + size - 1) / 32) > i) {
		uint32_t temp = read32(base + reg_offset + 4);
		temp &= mask >> (off ? (32 - off) : 0);
		value |= temp << (off ? (32 - off) : 0);
	}

	return value;
}

static void ipu_ch_param_write_field(vaddr_t base, int channel, int w, int bit, int size, uint32_t value) {
	int i = bit / 32;
	int off = bit % 32;
	uint32_t mask = (1UL << (size)) - 1;

	int reg_offset = channel * 64;
	reg_offset += w * 32;
	reg_offset += i * 4;

	uint32_t temp = read32(base + reg_offset);
	temp &= ~(mask << off);
	temp |= value << off;
	write32(temp, base + reg_offset);

	if (((bit + size - 1) / 32) > i) {
		temp = read32(base + reg_offset + 4);
		temp &= ~(mask >> (32 - off));
		temp |= (value >> (off ? (32 - off) : 0));
		write32(temp, base + reg_offset + 4);
	}
}

static bool fb_emu_check(struct region *region __unused, paddr_t address, enum emu_state state, uint32_t *value __unused) {
	paddr_t offset = address - region->base;
	EMSG("Emu Check with offset 0x%lX", offset);
	switch (offset) {
		case 0x3005E0: // Buffer 0 Address Register
		case 0x3005E4: // Buffer 1 Address Register
		case 0x301160: // Buffer 2 Address Register
			return (state != EMU_STATE_WRITE);
		default:
			return true;
	}
}

bool fb_acquire(struct fb_info *info, uint8_t r, uint8_t g, uint8_t b) {
	// Deny non-secure transactions to the buffers
	emu_add_region(g_base_paddr, 0x400000, fb_emu_check);
	csu_set_csl(61, true);

	// Save the previous set of parameters for later restoration
	for (int p = 0; p < 16; p++) {
		info->prev_params_ch0[p] = read32(g_cpmem_vaddr + (IPU_CHAN0 * 64) + (p * 4));
		info->prev_params_ch1[p] = read32(g_cpmem_vaddr + (IPU_CHAN1 * 64) + (p * 4));
	}

	// Read the size information
	info->width = ipu_ch_param_read_field(g_cpmem_vaddr, IPU_CHAN0, 0, 125, 13) + 1;
	info->height = ipu_ch_param_read_field(g_cpmem_vaddr, IPU_CHAN0, 0, 138, 12) + 1;
	info->stride = info->width * 3;

	EMSG("[FB] Screen Size of %dx%d", info->width, info->height);

	// Using memory configured by CFG_FBMEM_START and CFG_FBMEM_SIZE, which
	// is protected by the TZASC to be Secure RW + Non-Secure R
	info->buffer = phys_to_virt(CFG_FBMEM_START, MEM_AREA_IO_SEC);

	fb_clear(info, r, g, b);

	// Set the format to RGB24
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 0, 107, 3, 1); // Bits Per Pixel
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 85, 4, 7); // Pixel Format
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 78, 7, 19); // Burst Size

	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 116, 3, 8 - 1); // Red
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 128, 5, 16);
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 119, 3, 8 - 1); // Green
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 133, 5, 8);
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 122, 3, 8 - 1); // Blue
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 138, 5, 0);
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 125, 3, 8 - 1); // Alpha
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 143, 5, 24);

	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 102, 14, info->stride - 1);

	// Setting up the DMA addresses to point to the buffer
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 29 * 0, 29, CFG_FBMEM_START / 8);
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN0, 1, 29 * 1, 29, CFG_FBMEM_START / 8);
	ipu_ch_param_write_field(g_cpmem_vaddr, IPU_CHAN1, 1, 29 * 0, 29, CFG_FBMEM_START / 8);

	return true;
}

void fb_release(struct fb_info *info) {
	if (!info->buffer) {
		return;
	}

	// Restore the previous set of parameters
	for (int p = 0; p < 16; p++) {
		write32(info->prev_params_ch0[p], g_cpmem_vaddr + (IPU_CHAN0 * 64) + (p * 4));
		write32(info->prev_params_ch1[p], g_cpmem_vaddr + (IPU_CHAN1 * 64) + (p * 4));
	}

	info->buffer = NULL;

	// Allow non-secure transactions to the buffers
	csu_set_csl(61, false);
	emu_remove_region(g_base_paddr, 0x400000, fb_emu_check);
}

void fb_clear(struct fb_info *info, uint8_t r, uint8_t g, uint8_t b) {
	for (uint32_t y = 0; y < info->height; y++) {
		uint8_t *line = &info->buffer[info->stride * y];
		for (uint32_t x = 0; x < info->width; x++) {
			line[(3 * x) + 0] = r;
			line[(3 * x) + 1] = g;
			line[(3 * x) + 2] = b;
		}
	}
}

void fb_blit(struct fb_info *info, uint32_t x, uint32_t y, const uint8_t *buffer, uint32_t width, uint32_t height) {
	if (x >= info->width || y >= info->height) {
		return;
	}

	int copy_per_line;
	if (x + width > info->width) {
		copy_per_line = 3 * (info->width - x);
	} else {
		copy_per_line = 3 * width;
	}

	for (uint32_t by = 0; (by < height) && (y + by < info->height); by++) {
		uint8_t *fb_line = info->buffer + (info->stride * (y + by)) + (3 * x);
		const uint8_t *buf_line = buffer + (3 * width * by);
		memcpy(fb_line, buf_line, copy_per_line);
	}
}

void fb_blit_image(struct fb_info *info, uint32_t x, uint32_t y, const struct image *image) {
	fb_blit(info, x, y, image->buffer, image->width, image->height);
}

void fb_blit_all(struct fb_info *info, const struct blit *blits, int num_blits) {
	for (int b = 0; b < num_blits; b++) {
		fb_blit_image(info, blits[b].x, blits[b].y, &blits[b].image);
	}
}

static int fb_probe(const void *fdt __unused, struct device *dev, const void *data __unused)
{
	if (dev->num_resources != 1 || dev->resource_type != RESOURCE_MEM) {
		EMSG("Resource is not valid for device %s\n", dev->name);
		return -EINVAL;
	}

	paddr_t paddr = dev->resources[0].address[0];
	size_t size = dev->resources[0].size[0];
	if (!core_mmu_add_mapping(MEM_AREA_IO_SEC, paddr, size)) {
		EMSG("Could not map I/O memory (paddr %lX, size %X) for device %s\n", paddr, size, dev->name);
		return -EINVAL;
	}

	// Interested in IPU0 only
	if (!g_base_paddr || (paddr < g_base_paddr)) {
		g_base_paddr = paddr;
		g_base_vaddr = (vaddr_t)phys_to_virt(paddr, MEM_AREA_IO_SEC);
		g_cpmem_vaddr = (vaddr_t)phys_to_virt(paddr + IPU_CPMEM_OFFSET, MEM_AREA_IO_SEC);

		IMSG("Registered device %s with memory region (0x%lX, 0x%X)\n", dev->name, paddr, size);
	}

	return 0;
}

static const struct dt_device_match fb_match_table[] = {
	{ .compatible = "fsl,imx6q-ipu", .data = NULL, },
	{ 0 }
};

const struct dt_driver fb_dt_driver __dt_driver = {
	.name = "ipu-fb",
	.match_table = fb_match_table,
	.probe = fb_probe,
};

