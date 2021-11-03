#include <mios/io.h>
#include <mios/stream.h>

#include "platform/stm32/stm32_uart.h"

#define STM32WB_INSTANCE_USART1 0
#define STM32WB_INSTANCE_LPUART1 1

stream_t *stm32wb_uart_init(stm32_uart_t *u, unsigned int instance,
                            int baudrate,
                            gpio_t tx, gpio_t rx, uint8_t flags);
