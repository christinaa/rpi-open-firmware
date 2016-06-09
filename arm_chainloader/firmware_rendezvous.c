#include <stdint.h>
#include <hardware.h>
#include <chainloader.h>

extern void main();
extern void uart_putc(int c);

void _firmware_rendezvous() {
	/* Channels to talk to the firmware */
	volatile uint32_t* arm_membase = (volatile uint32_t*)0x0;
	volatile uint32_t* comm1 = arm_membase + 8;
	volatile uint32_t* comm2 = arm_membase + 9;
	volatile uint32_t* comm3 = arm_membase + 10;
	volatile uint32_t* comm4 = arm_membase + 11;

	*comm1 = 0xCAFEEEEE;

	/* 
	 * check if we have peripheral access
	 * if so, we don't need the VPU anymore
	 */
	if (ARM_ID != ARM_IDVAL) {
		*comm1 = 0xDEADCAFE;
		return;
	}

	*comm4 = VPU_KILL_COMMAND;

	/* stall for a bit to let the VPU commit suicide */
	for (int i = 0; i < 0x10000; i++)
		*comm2 = i;

	main();
}