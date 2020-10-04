#include <assert.h>
#include <stdint.h>
#include <task.h>
#include <irq.h>

#include "uart.h"
#include "stm32f4.h"

#include "clk_config.h"

static void monitor(uart_t *u);

#define USART_SR   0x00
#define USART_DR   0x04
#define USART_BBR  0x08
#define USART_CR1  0x0c


#define CR1_IDLE       (1 << 13) | (1 << 5) | (1 << 3) | (1 << 2)
#define CR1_ENABLE_TXI CR1_IDLE | (1 << 7)

void
uart_putc(void *arg, char c)
{
  uart_t *u = arg;

  int s = irq_forbid(IRQ_LEVEL_CONSOLE);

  if(!can_sleep()) {
    // We not on user thread, busy wait
    while(!(reg_rd(u->reg_base + USART_SR) & (1 << 7))) {}
    reg_wr(u->reg_base + USART_DR, c);
    irq_permit(s);
    return;
  }

  while(1) {
    uint8_t avail = TX_FIFO_SIZE - (u->tx_fifo_wrptr - u->tx_fifo_rdptr);

    if(avail)
      break;
    assert(u->tx_busy);
    task_sleep(&u->wait_tx, 0);
  }

  if(!u->tx_busy) {
    reg_wr(u->reg_base + USART_DR, c);
    reg_wr(u->reg_base + USART_CR1, CR1_ENABLE_TXI);
    u->tx_busy = 1;
  } else {
    u->tx_fifo[u->tx_fifo_wrptr & (TX_FIFO_SIZE - 1)] = c;
    u->tx_fifo_wrptr++;
  }
  irq_permit(s);
}




int
uart_getc(void *arg)
{
  uart_t *u = arg;

  int s = irq_forbid(IRQ_LEVEL_CONSOLE);

  while(u->rx_fifo_wrptr == u->rx_fifo_rdptr)
    task_sleep(&u->wait_rx, 0);

  char c = u->rx_fifo[u->rx_fifo_rdptr & (RX_FIFO_SIZE - 1)];
  u->rx_fifo_rdptr++;
  irq_permit(s);
  return c;
}




void
uart_irq(uart_t *u)
{
  const uint32_t sr = reg_rd(u->reg_base + USART_SR);

  if(sr & (1 << 5)) {
    const uint8_t c = reg_rd(u->reg_base + USART_DR);
    if(c == 5) {
      extern int task_trace;
      task_trace = 1;
    }
    if(c == 4) {
      monitor(u);
    }
    u->rx_fifo[u->rx_fifo_wrptr & (RX_FIFO_SIZE - 1)] = c;
    u->rx_fifo_wrptr++;
    task_wakeup(&u->wait_rx, 1);
  }

  if(sr & (1 << 7)) {
    uint8_t avail = u->tx_fifo_wrptr - u->tx_fifo_rdptr;
    if(avail == 0) {
      u->tx_busy = 0;
      reg_wr(u->reg_base + USART_CR1, CR1_IDLE);
    } else {
      uint8_t c = u->tx_fifo[u->tx_fifo_rdptr & (TX_FIFO_SIZE - 1)];
      u->tx_fifo_rdptr++;
      task_wakeup(&u->wait_tx, 1);
      reg_wr(u->reg_base + USART_DR, c);
    }
  }
}


void
uart_init(uart_t *u, int reg_base, int baudrate)
{
  const unsigned int bbr = (APB1CLOCK + baudrate - 1) / baudrate;

  u->reg_base = reg_base;
  reg_wr(u->reg_base + USART_CR1, (1 << 13)); // ENABLE
  reg_wr(u->reg_base + USART_BBR, bbr);
  reg_wr(u->reg_base + USART_CR1, CR1_IDLE);
  TAILQ_INIT(&u->wait_rx);
  TAILQ_INIT(&u->wait_tx);
}



static void __attribute__((noinline))
mon_putch(uart_t *u, char c)
{
  while(!(reg_rd(u->reg_base + USART_SR) & (1 << 7))) {}
  reg_wr(u->reg_base + USART_DR, c);
}

static void __attribute__((noinline))
mon_putu4(uart_t *u, uint8_t c)
{
  c &= 0xf;
  if(c < 10)
    mon_putch(u, c + '0');
  else
    mon_putch(u, c + 'a' - 10);
}

static void __attribute__((noinline))
mon_putu8(uart_t *u, uint8_t c)
{
  mon_putu4(u, c >> 4);
  mon_putu4(u, c);
}

static void __attribute__((noinline))
mon_putu32(uart_t *u, uint32_t u32)
{
  mon_putu8(u, u32 >> 24);
  mon_putu8(u, u32 >> 16);
  mon_putu8(u, u32 >> 8);
  mon_putu8(u, u32);
}

static void __attribute__((noinline))
mon_putstr(uart_t *u, const char *s)
{
  for(; *s; s++) {
    mon_putch(u, *s);
  }
}

static char __attribute__((noinline))
mon_getch(uart_t *u)
{
  while(!(reg_rd(u->reg_base + USART_SR) & (1 << 5))) {}
  char c = reg_rd(u->reg_base + USART_DR);
  mon_putch(u, c);
  return c;
}


static char  __attribute__((noinline))
mon_getu32(uart_t *u, uint32_t *p)
{
  uint32_t u32 = 0;
  char c = 0;
  do {
    c = mon_getch(u);
  } while(c ==' ');

  while(1) {
    switch(c) {
    case '0' ... '9':
      u32 = (u32 << 4) | (c - '0');
      break;
    case 'a' ... 'f':
      u32 = (u32 << 4) | (c - 'a' + 10);
      break;
    case 'A' ... 'F':
      u32 = (u32 << 4) | (c - 'A' + 10);
      break;
    default:
      *p = u32;
      return c;
    }
    c = mon_getch(u);
  }

}

static void
print_word(uart_t *u, uint32_t addr)
{

  mon_putu32(u, addr);
  mon_putstr(u, ": ");
  uint32_t v = *(uint32_t *)(intptr_t)addr;
  mon_putu32(u, v);
}


static void
print_basepri(uart_t *u)
{
  unsigned int basepri;
  asm volatile ("mrs %0, basepri\n\t" : "=r" (basepri));
  mon_putu32(u, basepri);
}

static void
print_task(uart_t *u, task_t *t)
{
  mon_putstr(u, "TASK @ ");
  mon_putu32(u, (uint32_t)t);
  mon_putstr(u, " Name: ");
  mon_putstr(u, t->t_name);
  mon_putstr(u, " State: ");
  mon_putch(u, "RSZ"[t->t_state]);
}

static void
monitor(uart_t *u)
{
  irq_forbid(IRQ_LEVEL_ALL);
  mon_putstr(u, "*** BREAK MONITOR\n# ");

  uint32_t addr = 0;
  char cmd = 0;

  while(1) {
    char c = mon_getch(u);
  reswitch:
    switch(c) {
    case 'x':
    case 't':
      cmd = c;
      c = mon_getu32(u, &addr);
      goto reswitch;
    case 'i':
      cmd = c;
      break;

    case 10:
    case 13:
      switch(cmd) {
      case 'x':
        print_word(u, addr);
        addr += 4;
        break;
      case 't':
        print_task(u, (void *)addr);
        break;
      case 'i':
        print_basepri(u);
        break;
      }
      mon_putstr(u, "\n# ");
    }
  }

}
