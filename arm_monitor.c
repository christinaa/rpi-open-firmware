/*=============================================================================
Copyright (C) 2016 Kristina Brooks
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

void monitor_irq() {

}

void monitor_start() {
	printf("Starting IPC monitor ...\n");

        /* dump status */
        printf("Status --- %X\n", mmio_read32(0x7E00B9B8));

        /* enable IRQ */
        mmio_write32(0x7E00B9BC, 0x1);

        for(;;) {
            if(mmio_read32(0x7E00B9B8) != 0x40000000) {
                printf("Squeal!\n");
            }
        }

	__asm__ __volatile__ ("sleep" :::);
        for(;;);
}
