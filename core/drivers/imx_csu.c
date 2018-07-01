#include <drivers/imx_csu.h>

#include <kernel/panic.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <io.h>

#define MAX_CSL 80
#define MAX_SA 16

static vaddr_t csu_base = 0;
static int csu_csl_counts[MAX_CSL];

void csu_init(paddr_t base) {
	csu_base = (vaddr_t)phys_to_virt(base, MEM_AREA_IO_SEC);
	if (!csu_base) {
		EMSG("Could not map PA->VA for CSU base address 0x%lX", base);
		panic();
	}

	// Default to no protections
	for (int c = 0; c < MAX_CSL; c++) {
		csu_csl_counts[c] = 1;
		csu_set_csl(c, false);
	}

	// Default to non-secure DMA masters (6 and 15 are reserved)
	for (int m = 1; m < MAX_SA; m++) {
		if ((m == 6) || (m == 15)) {
			continue;
		}

		csu_set_sa(m, false);
	}

	IMSG("[CSU] Initialized");
}

void csu_set_csl(int csl, bool protect) {	
	assert((csl >= 0) && (csl < MAX_CSL));

	bool update;
	if (protect) {
		update = (csu_csl_counts[csl] == 0);
		csu_csl_counts[csl]++;
	} else {
		update = (csu_csl_counts[csl] == 1);
		csu_csl_counts[csl]--;
		assert(csu_csl_counts[csl] >= 0);
	}

	if (update) {
		int csl_reg = csl >> 1;
		vaddr_t csl_addr = csu_base + (4 * csl_reg);

		uint32_t value = read32(csl_addr);
		uint32_t mask = (csl % 2 == 0) ? 0x000000FF : 0x00FF0000;
		uint32_t value_new = (value & ~mask) | ((protect ? 0x33333333 : 0xFFFFFFFF) & mask);

		IMSG("[CSU] Updating CSU CSL %d from 0x%X to 0x%X", csl, value, value_new);
		write32(value_new, csl_addr);
	}
}

bool csu_is_csl_protected(int csl) {
	assert((csl >= 0) && (csl < MAX_CSL));

	return (csu_csl_counts[csl] > 0);
}

void csu_set_sa(int master, bool secure) {
	assert((master >= 0) && (master < MAX_SA));

	vaddr_t sa_addr = csu_base + 0x218;

	uint32_t value = read32(sa_addr);
	uint32_t mask = 0x3 << (2 * master);
	uint32_t value_new = (value & ~mask) | ((secure ? 0x00000000 : 0x55555555) & mask);

	IMSG("[CSU] Updating CSU SA %d from 0x%X to 0x%X", master, value, value_new);
	write32(value_new, sa_addr);
}

