#include <secloak/emulation.h>

#include <arm.h>
#include <compiler.h>
#include <drivers/dt.h>
#include <errno.h>
#include <kernel/misc.h>
#include <kernel/panic.h>
#include <initcall.h>
#include <io.h>
#include <malloc.h>
#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <string.h>

static paddr_t emu_data_pstart;
static vaddr_t emu_data_vstart;
static size_t emu_data_size;
static paddr_t emu_instr_pstart;
static vaddr_t emu_instr_vstart;
static size_t emu_instr_size;

static SLIST_HEAD(, region) regions = SLIST_HEAD_INITIALIZER(regions);

int32_t emu_add_region(paddr_t base, uint32_t size, emu_check_t check) {
	struct region *r = malloc(sizeof(struct region));
	if (!r) {
		return -ENOMEM;
	}

	r->base = base;
	r->size = size;
	r->check = check;
	SLIST_INSERT_HEAD(&regions, r, entry);

	return 0;
}

void emu_remove_region(paddr_t base, uint32_t size, emu_check_t check) {
	struct region *r;
	SLIST_FOREACH(r, &regions, entry) {
		if (r->base == base && r->size == size && r->check == check) {
			SLIST_REMOVE(&regions, r, region, entry);
			free(r);
			return;
		}
	}
}

static bool emu_check(paddr_t address, enum emu_state state, uint32_t *value) {
	bool allowed = true;

	struct region *r;
	SLIST_FOREACH(r, &regions, entry) {
		if (r->base <= address && (r->base + r->size) >= address) {
			allowed &= r->check(r, address, state, value);
		}
	}

	return allowed;
}

bool emu_allow_all(struct region *region __unused, paddr_t address __unused, enum emu_state state __unused, uint32_t *value __unused) {
	return true;
}

bool emu_deny_all(struct region *region __unused, paddr_t address __unused, enum emu_state state __unused, uint32_t *value __unused) {
	return false;
}

static inline uint32_t read(vaddr_t addr, int size) {
	switch(size) {
		case 1:
			return read8(addr);
		case 2:
			return read16(addr);
		case 4:
			return read32(addr);
		default:
			return 0; // Note: Will never happen
	}
}

static inline void write(uint32_t value, vaddr_t addr, int size) {
	switch(size) {
		case 1:
			write8(value, addr);
			break;
		case 2:
			write16(value, addr);
			break;
		case 4:
			write32(value, addr);
			break;
		default:
			break; // Note: Will never happen
	}
}

static inline void emu_handle_load(paddr_t data_paddr, vaddr_t data_vaddr, uint32_t *reg, int size, bool sign) {
	if (emu_check(data_paddr, EMU_STATE_READ_BEFORE, NULL)) {
		*reg = read(data_vaddr, size);
		if (sign) {
			if (size == 1) {
				if ((*reg) & 0x80) {
					*reg |= 0xFFFFFF00;
				}
			} else if (size == 2) {
				if ((*reg) & 0x8000) {
					*reg |= 0xFFFF0000;
				}
			}
		}
		emu_check(data_paddr, EMU_STATE_READ_AFTER, reg);
	} else {
		*reg = 0;
	}
}

static inline void emu_handle_store(paddr_t data_paddr, vaddr_t data_vaddr, uint32_t *reg, int size) {
	if (emu_check(data_paddr, EMU_STATE_WRITE, reg)) {
		write(*reg, data_vaddr, size);
	}
}

void emu_handle(struct sm_ctx *ctx, unsigned long status, unsigned long data_paddr, unsigned long instr_paddr) {
	if ((status & 0x40F) != 0x008) {
		EMSG("[EMU] Ignoring status of 0x%lX", status);
		ctx->nsec.mon_lr -= 4;
		return;
	}

	if ((data_paddr < emu_data_pstart) || (data_paddr >= (emu_data_pstart + emu_data_size))) {
		EMSG("[EMU] Could not translate PA->VA for data address 0x%lX", data_paddr);
		return;
	}
	vaddr_t data_vaddr = emu_data_vstart + (data_paddr - emu_data_pstart);

	if ((instr_paddr < emu_instr_pstart) || (instr_paddr >= (emu_instr_pstart + emu_instr_size))) {
		EMSG("[EMU] Could not translate PA->VA for instruction address 0x%lX", instr_paddr);
		return;
	}
	vaddr_t instr_vaddr = emu_instr_vstart + (instr_paddr - emu_instr_pstart);

	uint32_t instr = *((uint32_t *)instr_vaddr);

	uint32_t instr_rt = (instr >> 12) & 0xF;
	uint32_t *reg;
	if (instr_rt <= 7) {
		reg = &ctx->nsec.r0 + instr_rt;
	} else if (instr_rt <= 12) {
			reg = &ctx->nsec.r8 + (instr_rt - 8);
	} else if (instr_rt == 14) {
			reg = &ctx->nsec.lr;
	} else {
		EMSG("[EMU] Unexpected instruction with Rt of %u", instr_rt);
		panic();
		return;
	}

	uint32_t instr_type = (instr >> 25) & 0x7;
	if ((instr_type & 0x6) == 0x2) {
		int size = (instr & (1 << 22)) ? 1 : 4;
		if (instr & (1 << 20)) {
			emu_handle_load(data_paddr, data_vaddr, reg, size, false);
		} else {
			emu_handle_store(data_paddr, data_vaddr, reg, size);
		}
	} else if (instr_type == 0) {
		int size = (instr & (1 << 5)) ? 2 : 1;
		if (instr & (1 << 20)) {
			bool sign = (instr & (1 << 6));
			emu_handle_load(data_paddr, data_vaddr, reg, size, sign);
		} else {
			emu_handle_store(data_paddr, data_vaddr, reg, size);
		}
	} else {
		EMSG("[EMU] Unexpected instruction with type %u", instr_type);
		panic();
	}
}

static TEE_Result emulation_init(void) {
  const struct tee_mmap_region *map;

	map = core_mmu_find_map_by_type_and_pa(MEM_AREA_IO_SEC, 0x01000000);
	if (map == NULL) {
		EMSG("[EMU] Could not get MEM_AREA_IO_SEC mapping");
		panic();
	}

	emu_data_pstart = map->pa;
	emu_data_vstart = map->va;
	emu_data_size = map->size;

	map = core_mmu_find_map_by_type(MEM_AREA_RAM_NSEC);
	if (map == NULL) {
		EMSG("[EMU] Could not get MEM_AREA_RAM_NSEC mapping");
		panic();
	}

	emu_instr_pstart = map->pa;
	emu_instr_vstart = map->va;
	emu_instr_size = map->size;

	return 0;
}
driver_init_late(emulation_init);

