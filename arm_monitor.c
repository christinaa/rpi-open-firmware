/*=============================================================================
Copyright (C) 2016-2017 Authors of rpi-open-firmware
All rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

FILE DESCRIPTION
First stage monitor.

=============================================================================*/

#include <common.h>
#include "hardware.h"

/*
 * called from sleh_irq (trap.c)
 */
void arm_monitor_interrupt() {
	printf("VPU MBOX rcv: 0x%X, cnf 0x%X\n",
	       ARM_1_MAIL1_RD,
	       ARM_1_MAIL1_CNF);
}

void monitor_start() {
	printf("Starting IPC monitor ...\n");

	/* dump status */
	printf("Mailbox status: 0x%X\n", ARM_1_MAIL1_STA);

	/* enable IRQ */
	ARM_1_MAIL1_CNF = ARM_MC_IHAVEDATAIRQEN;

	for(;;) {
		__asm__ __volatile__ ("sleep" :::);
		printf("sleep interrupted!\n");
	}
}
