#ifndef DRIVERS_IMXCSU_H
#define DRIVERS_IMXCSU_H

#include <stdbool.h>
#include <stdint.h>
#include <types_ext.h>

void csu_init(paddr_t base);

void csu_set_csl(int csl, bool protect);
bool csu_is_csl_protected(int csl);

void csu_set_sa(int master, bool secure);

#endif

