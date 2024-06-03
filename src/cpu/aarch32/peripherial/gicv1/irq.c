#include "irq.h"

#include "cpu.h"
#include "reg.h"

#include <mios/mios.h>

#include <stdio.h>


struct irqentry {
  void *arg;
  void (*fn)(void *arg);
};

struct irqentry irqvector[96];


uint32_t
irq_get_active(void)
{
  uint32_t pbase = cpu_get_periphbase();
  return reg_rd(pbase + ICCIAR);
}

void
irq_end(uint32_t irq)
{
  uint32_t pbase = cpu_get_periphbase();
  reg_wr(pbase + ICCEOIR, irq);
}

void
irq_enable(int irq, int level)
{
  uint32_t reg = irq >> 5;
  uint32_t bit = irq & 0x1f;

  uint32_t pbase = cpu_get_periphbase();

  reg_wr(pbase + GICD_ICPENDR(reg), (1 << bit));
  reg_wr(pbase + GICD_ISENABLER(reg), (1 << bit));
  reg_wr8(pbase + GICD_IPRIORITYR(irq), IRQ_LEVEL_TO_PRI(level));
}

void
irq_disable(int irq)
{
  uint32_t reg = irq >> 5;
  uint32_t bit = irq & 0x1f;

  uint32_t pbase = cpu_get_periphbase();

  reg_wr(pbase + GICD_ICENABLER(reg), (1 << bit));
  reg_wr(pbase + GICD_ICPENDR(reg), (1 << bit));
}


void
irq_enable_fn_arg(int irq, int level, void (*fn)(void *arg), void *arg)
{
  irqvector[irq].fn = fn;
  irqvector[irq].arg = arg;
  irq_enable(irq, level);
}

void
irq_enable_fn(int irq, int level, void (*fn)(void))
{
  irq_enable_fn_arg(irq, level, (void *)fn, NULL);
}


static void
spurious(void *arg, struct irqentry *ie)
{
  panic("Spurious irq %d", ie - 1 - irqvector);
}

extern void cpu_task_switch(void);

static void  __attribute__((constructor(102)))
irq_init(void)
{
  uint32_t pbase = cpu_get_periphbase();

  reg_wr(pbase + GICD_CTRL, 0);
  reg_wr(pbase + ICPICR, 0);

  for(size_t i = 0; i < 96; i++) {
    irq_disable(i);
    irqvector[i].fn = (void *)spurious;
  }

  reg_wr(pbase + ICCIPMR, 0xf8);
  reg_wr(pbase + GICD_CTRL, 1);
  reg_wr(pbase + ICPICR, 1);

  irq_enable_fn(0, IRQ_LEVEL_SWITCH, cpu_task_switch);
}



unsigned int
irq_forbid(unsigned int level)
{
  uint32_t cpsr;
  uint32_t pri = IRQ_LEVEL_TO_PRI(level);
  uint32_t pbase = cpu_get_periphbase();

  asm volatile ("mrs %0, cpsr\n\t" : "=r" (cpsr));
  asm volatile ("msr cpsr, %0\n\r" :: "r" (cpsr | 0x80));

  uint32_t pmr = reg_rd(pbase + ICCIPMR);
  if(pri < pmr) {
    reg_wr(pbase + ICCIPMR, pri);
  }
  asm volatile ("msr cpsr, %0\n\r" :: "r" (cpsr));
  return pmr;
}

void
irq_permit(unsigned int old)
{
  uint32_t pbase = cpu_get_periphbase();
  reg_wr(pbase + ICCIPMR, old);
}


unsigned int
irq_lower(void)
{
  uint32_t pbase = cpu_get_periphbase();
  uint32_t old = reg_rd(pbase + ICCIPMR);
  reg_wr(pbase + ICCIPMR, 0xf8);
  return old;
}

void
schedule(void)
{
  uint32_t pbase = cpu_get_periphbase();
  reg_wr(pbase + GICD_SGIR, (0b10 << 24));
}


int
can_sleep(void)
{
  uint32_t pbase = cpu_get_periphbase();
  return reg_rd(pbase + ICCRPR) >= 0xf8;
}
