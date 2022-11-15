#include <mios/io.h>

#include <mios/mios.h>

#include "stm32g4_reg.h"
#include "stm32g4_clk.h"
#include "irq.h"

#define GPIO_PORT_ADDR(x) (0x48000000 + ((x) * 0x400))

#define GPIO_MODER(x)   (GPIO_PORT_ADDR(x) + 0x00)
#define GPIO_OTYPER(x)  (GPIO_PORT_ADDR(x) + 0x04)
#define GPIO_OSPEEDR(x) (GPIO_PORT_ADDR(x) + 0x08)
#define GPIO_PUPDR(x)   (GPIO_PORT_ADDR(x) + 0x0c)
#define GPIO_IDR(x)     (GPIO_PORT_ADDR(x) + 0x10)
#define GPIO_ODR(x)     (GPIO_PORT_ADDR(x) + 0x14)
#define GPIO_BSRR(x)    (GPIO_PORT_ADDR(x) + 0x18)
#define GPIO_LCKR(x)    (GPIO_PORT_ADDR(x) + 0x1c)
#define GPIO_AFRL(x)    (GPIO_PORT_ADDR(x) + 0x20)
#define GPIO_AFRH(x)    (GPIO_PORT_ADDR(x) + 0x24)



void
gpio_conf_input(gpio_t gpio, gpio_pull_t pull)
{
  const int port = gpio >> 4;
  const int bit = gpio & 0xf;

  int s = irq_forbid(IRQ_LEVEL_IO);
  clk_enable(CLK_GPIO(port));
  reg_set_bits(GPIO_MODER(port), bit * 2, 2, 0);
  reg_set_bits(GPIO_PUPDR(port), bit * 2, 2, pull);
  irq_permit(s);
}



void
gpio_conf_output(gpio_t gpio,
                 gpio_output_type_t type,
                 gpio_output_speed_t speed,
                 gpio_pull_t pull)
{
  const int port = gpio >> 4;
  const int bit = gpio & 0xf;

  int s = irq_forbid(IRQ_LEVEL_IO);
  clk_enable(CLK_GPIO(port));
  reg_set_bits(GPIO_OTYPER(port),  bit, 1, type);
  reg_set_bits(GPIO_OSPEEDR(port), bit * 2, 2, speed);
  reg_set_bits(GPIO_PUPDR(port), bit * 2, 2, pull);
  reg_set_bits(GPIO_MODER(port), bit * 2, 2, 1);
  irq_permit(s);
}


void
gpio_conf_af(gpio_t gpio, int af, gpio_output_type_t type,
             gpio_output_speed_t speed, gpio_pull_t pull)
{
  const int port = gpio >> 4;
  const int bit = gpio & 0xf;

  int s = irq_forbid(IRQ_LEVEL_IO);
  clk_enable(CLK_GPIO(port));

  reg_set_bits(GPIO_OTYPER(port),  bit, 1, type);
  reg_set_bits(GPIO_OSPEEDR(port), bit * 2, 2, speed);

  if(bit < 8) {
    reg_set_bits(GPIO_AFRL(port), bit * 4, 4, af);
  } else {
    reg_set_bits(GPIO_AFRH(port), (bit - 8) * 4, 4, af);
  }

  reg_set_bits(GPIO_PUPDR(port), bit * 2, 2, pull);

  reg_set_bits(GPIO_MODER(port), bit * 2, 2, 2);
  irq_permit(s);
}



void
gpio_set_output(gpio_t gpio, int on)
{
  const int port = gpio >> 4;
  const int bit = gpio & 0xf;

  reg_set(GPIO_BSRR(port), 1 << (bit + !on * 16));
}


int
gpio_get_input(gpio_t gpio)
{
  const int port = gpio >> 4;
  const int bit = gpio & 0xf;

  uint32_t idr = reg_rd(GPIO_IDR(port));

  return !!((1 << bit) & idr);
}


