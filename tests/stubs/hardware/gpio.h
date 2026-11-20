// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
constexpr bool GPIO_OUT = true;
void gpio_init(unsigned pin);
void gpio_init_mask(uint32_t mask);
void gpio_put(unsigned pin, bool value);
void gpio_put_masked(uint32_t mask, uint32_t value);
void gpio_set_dir(unsigned pin, bool out);
void gpio_set_dir_out_masked(uint32_t mask);
