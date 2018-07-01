#include <drivers/dt.h>

#include <drivers/imx_csu.h>
#include <errno.h>
#include <initcall.h>
#include <io.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <malloc.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <secloak/emulation.h>

static inline uint32_t hash_32(uint32_t value, unsigned int bits) {
	return (value * 0x61C88647) >> (32 - bits);
}

#define DEVICE_TABLE_SIZE_LOG2 7
#define DEVICE_TABLE_SIZE (1 << (DEVICE_TABLE_SIZE_LOG2))

struct device_bucket {
	SLIST_HEAD(, device) entries;
};

struct device_table {
	struct device_bucket buckets[DEVICE_TABLE_SIZE];
};

static struct device_table g_device_table;

static struct device *device_lookup(int node) {
	struct device_bucket *bucket = &g_device_table.buckets[hash_32(node, DEVICE_TABLE_SIZE_LOG2)];
	struct device *dev;

	SLIST_FOREACH(dev, &bucket->entries, entry) {
		if (dev->node == node) {
			break;
		}
	}

	return dev;
}

static inline struct device* device_next(int *b, struct device *dev) {
	if (!dev && (*b == 0)) {
		dev = SLIST_FIRST(&g_device_table.buckets[0].entries);
	} else {
		dev = SLIST_NEXT(dev, entry);
	}

	while (!dev && (++(*b) < DEVICE_TABLE_SIZE)) {
		dev = SLIST_FIRST(&g_device_table.buckets[*b].entries);
	}

	return dev;
}

#define device_for_each(dev) \
	for (int b = 0; (dev = device_next(&b, dev)) != NULL; )

#define device_for_each_parent(start, dev) \
	for (dev = start; dev != NULL; dev = dev->parent)

#define device_for_each_parent_excl(start, dev) \
	for (dev = start->parent; dev != NULL; dev = dev->parent)

static void device_insert(struct device *dev) {
	struct device_bucket *bucket = &g_device_table.buckets[hash_32(dev->node, DEVICE_TABLE_SIZE_LOG2)];
	SLIST_INSERT_HEAD(&bucket->entries, dev, entry);
}

static struct device* dt_probe_device(void *fdt, int offset, struct device *parent, bool probe_children);

const char *simple_bus_match_table[] = {
	"simple-bus",
	"simple-mfd",
	"isa"
};

// Note: Taken from a newer version of libFDT
static int fdt_stringlist_count(const void *fdt, int nodeoffset, const char *property)
{
	const char *list, *end;
	int length, count = 0;

	list = fdt_getprop(fdt, nodeoffset, property, &length);
	if (!list)
		return length;

	end = list + length;

	while (list < end) {
		length = strnlen(list, end - list) + 1;

		/* Abort if the last string isn't properly NUL-terminated. */
		if (list + length > end)
			return -1;

		list += length;
		count++;
	}

	return count;
}

static bool dt_parse_resources(void *fdt, struct device *dev) {
	int reg_length;
	const fdt32_t *reg = fdt_getprop(fdt, dev->node, "reg", &reg_length);
	if (reg) {
		int addr_cells = fdt_address_cells(fdt, dev->parent->node);
		int size_cells = fdt_size_cells(fdt, dev->parent->node);
		int resource_cells = addr_cells + size_cells;

		dev->num_resources = reg_length / (4 * resource_cells);
		dev->resources = malloc(sizeof(*dev->resources) * dev->num_resources);
		if (!dev->resources) {
			EMSG("[DT] \tOut of memory");
			return false;
		}

		for (int r = 0; r < dev->num_resources; r++) {
			for (int a = 0; a < addr_cells; a++) {
				dev->resources[r].address[a] = fdt32_to_cpu(reg[(r * resource_cells) + a]);
			}
			for (int s = 0; s < size_cells; s++) {
				dev->resources[r].size[s] = fdt32_to_cpu(reg[(r * resource_cells) + addr_cells + s]);
			}
		}
	}

	return true;
}

static bool dt_parse_interrupts_ext(void *fdt, struct device *dev) {
	int irqs_length;
	const fdt32_t *irqs = fdt_getprop(fdt, dev->node, "interrupts-extended", &irqs_length);

	if (irqs) {
		int index = 0;
		while (index < (irqs_length / 4)) {
			int chip_offset = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(irqs[index]));
			if (chip_offset < 0) {
				EMSG("[DT] \tDevice '%s' has invalid extended IRQ phandle", dev->name);
				return false;
			}

			index++;

			struct device *chip_dev = device_lookup(chip_offset);
			if (!chip_dev) {
				chip_dev = dt_probe_device(fdt, chip_offset, NULL, false);
				if (!chip_dev) {
					EMSG("[DT] \tCould not probe IRQ chip device with offset %d", chip_offset);
					return false;
				}
			}

			struct irq_chip *chip = irq_find_chip(chip_dev);
			if (!chip) {
				EMSG("[DT] \tIRQ device '%s' did not register a chip", chip_dev->name);
				return false;
			}

			int irq_cells_length;
			const fdt32_t *irq_cells = fdt_getprop(fdt, chip_offset, "#interrupt-cells", &irq_cells_length);
			if (!irq_cells || (irq_cells_length != 4)) {
				EMSG("[DT] \tIRQ chip '%s' has invalid #interrupt-cells property", chip_dev->name);
				return false;
			}
			int num_irq_cells = fdt32_to_cpu(irq_cells[0]);

			dev->num_irqs++;
			dev->irqs = realloc(dev->irqs, sizeof(*dev->irqs) * dev->num_irqs);
			if (!dev->irqs) {
				EMSG("[DT] \tOut of memory");
				return false;
			}

			struct irq_info *info = &dev->irqs[dev->num_irqs - 1];
			info->desc.chip = chip;
			irq_map(chip, &irqs[index], &info->desc.irq, &info->flags);
			IMSG("[DT] \tIRQExt '%s' #%d (Flags %d)", chip_dev->name, info->desc.irq, info->flags);

			index += num_irq_cells;
		}

		assert(index == (irqs_length / 4));
	}

	return true;
}

static bool dt_parse_interrupts(void *fdt, struct device *dev) {
	int irqs_length;
	const fdt32_t *irqs = fdt_getprop(fdt, dev->node, "sp-interrupts", &irqs_length);
	if (!irqs) {
		irqs = fdt_getprop(fdt, dev->node, "interrupts", &irqs_length);
	}

	if (irqs) {
		struct device *parent = dev;
		int irq_cells_length = 0;
		const fdt32_t *irq_cells = NULL;

		while (parent) {
			int irq_parent_phandle_length;
			const fdt32_t *irq_parent_phandle = fdt_getprop(fdt, parent->node, "interrupt-parent", &irq_parent_phandle_length);
			if (irq_parent_phandle && (irq_parent_phandle_length == 4)) {
				int irq_parent = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*irq_parent_phandle));
				if (irq_parent < 0) {
					EMSG("[DT] \tParent '%s' has invalid IRQ parent phandle", fdt_get_name(fdt, irq_parent, NULL));
					return false;
				}

				irq_cells = fdt_getprop(fdt, irq_parent, "#interrupt-cells", &irq_cells_length);
				if (!irq_cells || (irq_cells_length != 4)) {
					EMSG("[DT] \tIRQ parent '%s' has invalid #interrupt-cells property", fdt_get_name(fdt, irq_parent, NULL));
					return false;
				}

				parent = device_lookup(irq_parent);
				if (!parent) {
					parent = dt_probe_device(fdt, irq_parent, NULL, false);
				}

				break;
			} else {
				parent = parent->parent;
			}
		}

		if (!parent) {
			EMSG("[DT] \tCould not find valid IRQ parent");
			return false;
		}

		struct irq_chip *chip = irq_find_chip(parent);
		if (!chip) {
			EMSG("[DT] \tIRQ device '%s' did not register a chip", parent->name);
			return false;
		}

		int num_irq_cells = fdt32_to_cpu(irq_cells[0]);
		dev->num_irqs = irqs_length / (4 * num_irq_cells);
		dev->irqs = malloc(sizeof(*dev->irqs) * dev->num_irqs);
		if (!dev->irqs) {
			EMSG("[DT] \tOut of memory");
			return false;
		}

		for (int i = 0; i < dev->num_irqs; i++) {
			struct irq_info *info = &dev->irqs[i];
			info->desc.chip = chip;
			irq_map(chip, &irqs[num_irq_cells * i], &info->desc.irq, &info->flags);
			IMSG("[DT] \tIRQ '%s' #%d (Flags %d)", parent->name, info->desc.irq, info->flags);
		}
	}

	return dt_parse_interrupts_ext(fdt, dev);
}

static bool dt_parse_sp_csu(void *fdt, struct device *dev) {
	int sp_csu_length;
	const fdt32_t *sp_csu = fdt_getprop(fdt, dev->node, "sp-csu", &sp_csu_length);
	if (sp_csu) {
		dev->num_csu = sp_csu_length / 4;
		dev->csu = malloc(sizeof(*dev->csu) * dev->num_csu);
		if (!dev->csu) {
			EMSG("[DT] \tOut of memory");
			return false;
		}

		for (int c = 0; c < dev->num_csu; c++) {
			dev->csu[c] = fdt32_to_cpu(sp_csu[c]);
		}
	}

	return true;
}

static bool dt_parse_sp_class(void *fdt, struct device *dev) {
	int sp_class_length;
	const char *sp_class = fdt_getprop(fdt, dev->node, "sp-class", &sp_class_length);
	if (sp_class) {
		dev->num_classes = fdt_stringlist_count(fdt, dev->node, "sp-class");
		dev->classes = malloc(sizeof(*dev->classes) * dev->num_classes);
		if (!dev->classes) {
			EMSG("[DT] \tOut of memory");
			return false;
		}

		for (int c = 0; c < dev->num_classes; c++) {
			dev->classes[c] = sp_class;
			sp_class += strlen(sp_class) + 1;
		}
	}

	return true;
}

static struct device *device_create(void *fdt, int node, struct device *parent, bool strict) {
	const struct fdt_property *compat = fdt_get_property(fdt, node, "compatible", NULL);
	if (strict && ((compat == NULL) || (_fdt_get_status(fdt, node) == DT_STATUS_DISABLED))) {
		return NULL;
	}

	struct device *dev = malloc(sizeof(struct device));
	if (!dev) {
		EMSG("[DT] Out of memory");
		panic();
	}

	memset(dev, 0, sizeof(*dev));

	dev->node = node;
	dev->phandle = fdt_get_phandle(fdt, node);
	dev->name = fdt_get_name(fdt, node, NULL);
	dev->parent = parent;
	dev->enabled = true;
	dev->is_simple_bus = false;
	for (unsigned int m = 0; m < sizeof(simple_bus_match_table) / sizeof(char *); m++) {
		if (fdt_node_check_compatible(fdt, node, simple_bus_match_table[m]) == 0) {
			dev->is_simple_bus = true;
			break;
		}
	}
	dev->resource_type = (parent && parent->is_simple_bus) ? RESOURCE_MEM : RESOURCE_OTHER;

	IMSG("[DT] Device '%s' [%d] (Parent = '%s')", dev->name, dev->node, parent ? parent->name : "None");

	bool success = dt_parse_resources(fdt, dev);
	success &= dt_parse_interrupts(fdt, dev);
	success &= dt_parse_sp_csu(fdt, dev);
	success &= dt_parse_sp_class(fdt, dev);
	
	if (!success) {
		panic();
	}
	
	return dev;
}

static struct device* dt_probe_device(void *fdt, int offset, struct device *parent, bool probe_children) {
	if (!parent) {
		int parent_offset = fdt_parent_offset(fdt, offset);
		if (parent_offset >= 0) {
			parent = device_lookup(parent_offset);
			if (!parent) {
				parent = dt_probe_device(fdt, parent_offset, NULL, false);
				if (!parent) {
					return NULL;
				}
			}
		}
	}

	struct device *dev = device_lookup(offset);
	if (!dev) {
		dev = device_create(fdt, offset, parent, true);
		if (!dev) {
			return NULL;
		}

		device_insert(dev);
		dt_probe_compatible_driver(fdt, dev);
	}

	if (probe_children) {
		int child_offset;
		fdt_for_each_subnode(child_offset, fdt, offset) {
			dt_probe_device(fdt, child_offset, dev, true);
		}
	}

	return dev;
}

struct device *dt_lookup_device(const void *fdt, fdt32_t phandle) {
	int offset = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(phandle));
	return device_lookup(offset);
}

bool dt_enable_device(struct device *dev, bool enable) {
	bool can_protect = ((dev->num_csu > 0) && (dev->resource_type == RESOURCE_MEM));

	if (dev->enabled == enable) {
		goto out;
	}

	// Disabling IRQs for the device
	for(int i = 0; i < dev->num_irqs; i++) {
		if (enable) {
			irq_unsecure(&dev->irqs[i].desc);
			irq_enable(&dev->irqs[i].desc);
		} else {
			irq_disable(&dev->irqs[i].desc);
			irq_secure(&dev->irqs[i].desc);
		}
	}

	// If device can be protected, set protections and emulation policy
	if (can_protect) {
		if (enable) {
			for (int c = 0; c < dev->num_csu; c++) {
				csu_set_csl(dev->csu[c], false);
			}

			for (int r = 0; r < dev->num_resources; r++) {
				emu_remove_region(dev->resources[r].address[0], dev->resources[r].size[0], emu_deny_all);
			}
		} else {
			for (int r = 0; r < dev->num_resources; r++) {
				emu_add_region(dev->resources[r].address[0], dev->resources[r].size[0], emu_deny_all);
			}

			for (int c = 0; c < dev->num_csu; c++) {
				csu_set_csl(dev->csu[c], true);
			}
		}
	}

	dev->enabled = enable;

out:
	return can_protect;
}


static bool device_has_class(struct device *dev, const char *name) {
	for (int c = 0; c < dev->num_classes; c++) {
		if (strcmp(dev->classes[c], name) == 0) {
			return true;
		}
	}

	return false;
}

void dt_enable_class(const char *name, bool enable) {
	struct device *dev = NULL;
	device_for_each(dev) {
		if (device_has_class(dev, name)) {
			IMSG("[DT] Device '%s' belongs to class '%s'. Enabled? %s", dev->name, name, dev->enabled ? "Yes" : "No");
			if (dev->enabled == enable) {
				continue;
			}

			// Go through the device and each of its parents until we find one that we can protect
			struct device *cur;
			device_for_each_parent(dev, cur) {
				if (dt_enable_device(cur, enable)) {
					IMSG("\tProtected by device '%s'", cur->name);
					break;
				}
			}
		}
	}
}

bool dt_is_class_enabled(const char *name) {
	struct device *dev = NULL;
	device_for_each(dev) {
		if (device_has_class(dev, name)) {
			return dev->enabled;
		}
	}

	return false;
}

static TEE_Result dt_probe(void) {
	void *fdt;
	if (!(fdt = phys_to_virt(CFG_DT_ADDR, MEM_AREA_RAM_NSEC))) {
		panic();
	}

	for (int b = 0; b < DEVICE_TABLE_SIZE; b++) {
		SLIST_INIT(&g_device_table.buckets[b].entries);
	}

	int root = fdt_path_offset(fdt, "/");
	if (root < 0) {
		panic();
	}

	struct device *root_device = device_create(fdt, root, NULL, false);
	if (!root_device) {
		panic();
	}
	root_device->is_simple_bus = true;
	device_insert(root_device);

	int child;
	fdt_for_each_subnode(child, fdt, root) {
		dt_probe_device(fdt, child, root_device, true);
	}

	return 0;
}
driver_init(dt_probe);

