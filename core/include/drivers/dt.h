#ifndef DRIVERS_DT_H
#define DRIVERS_DT_H

#include <libfdt.h>
#include <sys/queue.h>
#include <kernel/interrupt.h>

#define fdt_for_each_subnode(node, fdt, parent) \
	for (node = fdt_first_subnode(fdt, parent); \
			node >= 0; \
			node = fdt_next_subnode(fdt, node))

enum resource_type {
	RESOURCE_MEM,
	RESOURCE_OTHER,
};

struct resource {
	uint32_t address[FDT_MAX_NCELLS];
	uint32_t size[FDT_MAX_NCELLS];
};

struct irq_info {
	struct irq_desc desc;
	uint32_t flags;
};

struct device {
	int node;
	uint32_t phandle;
	const char *name;
	struct device *parent;
	struct device **deps;
	int num_deps;
	enum resource_type resource_type;
	struct resource *resources;
	int num_resources;
	struct irq_info *irqs;
	int num_irqs;
	int *csu;
	int num_csu;
	const char **classes;
	int num_classes;
	bool enabled;
	bool probed;
	bool is_simple_bus;
	SLIST_ENTRY(device) entry;
};

struct device *dt_lookup_device(const void *fdt, fdt32_t phandle);
bool dt_enable_device(struct device *dev, bool enable);
void dt_enable_class(const char *name, bool enable);
bool dt_is_class_enabled(const char *name);

#endif

