#ifndef DRIVERS_IMX_GPIO_H
#define DRIVERS_IMX_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include <types_ext.h>

struct mxc_gpio_port;

struct mxc_gpio_port *gpio_port_from_address(paddr_t base);

bool gpio_acquire(struct mxc_gpio_port *port, int index, bool is_output);
bool gpio_acquire_dt(const uint32_t *gpio_info, bool is_output);

bool gpio_release(struct mxc_gpio_port *port, int index);
void gpio_release_dt(const uint32_t *gpio_info);

bool gpio_read(struct mxc_gpio_port *port, int index);
void gpio_write(struct mxc_gpio_port *port, int index, bool value);

#endif

