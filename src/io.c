#include <io.h>

// Weak stubs for IO methods. There are overriden by the linker if
// platform specific code provides such interface.

void __attribute__((weak))
gpio_conf_input(gpio_t gpio, gpio_pull_t pull)
{
}

void __attribute__((weak))
gpio_conf_output(gpio_t gpio, gpio_output_type_t type,
                 gpio_output_speed_t speed, gpio_pull_t pull)
{

}

void __attribute__((weak))
gpio_set_output(gpio_t gpio, int on)
{

}

int __attribute__((weak))
gpio_get_input(gpio_t gpio)
{
  return 0;
}

void __attribute__((weak))
gpio_conf_irq(gpio_t gpio, gpio_pull_t pull,
              void (*cb)(void *arg), void *arg,
              gpio_edge_t edge, int level)
{

}
