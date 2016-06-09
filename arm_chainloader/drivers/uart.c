#include <hardware.h>

void uart_putc(unsigned int ch)
{
	while(1) {
		if (mmio_read32(AUX_MU_LSR_REG) & 0x20)
			break;
	}
	mmio_write32(AUX_MU_IO_REG, ch);
}
