#include <hardware.h>

#define UART_STATUS 0x3F201018
#define UART_DATA   0x3F201000

void uart_putc(unsigned int ch)
{
	while(mmio_read32(UART_STATUS) & 0x20);
	mmio_write32(UART_DATA, ch);
}
