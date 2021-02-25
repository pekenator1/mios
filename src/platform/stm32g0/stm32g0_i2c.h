#pragma once

#include <mios/io.h>

i2c_t *stm32g0_i2c_create(int instance, gpio_t scl, gpio_t sda,
                          gpio_pull_t pull);

