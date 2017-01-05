/*=============================================================================
Copyright (C) 2016 Alyssa Rosenzweig 
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
USB bringup code

=============================================================================*/

#include <hardware.h>

#define BASE 0x7E980080
#define STATUS (BASE + 0)
#define DATA (BASE + 4)

#define STATUS_BUSY (1 << 31)

void usb_wait() {
    while(mmio_read32(STATUS) & STATUS_BUSY);
}

void usb_write(int reg, uint16_t value) {
    assert(reg <= 0x20);

    /* precede MDIO access */
    mmio_write32(DATA, 0xFFFFFFFF);
    usb_wait();
    
    /* write the actual value, with flags */
    mmio_write32(DATA, 0x50020000 | (reg << 12) | value);
    usb_wait();

    /* dummy write due to errata; see BCM2835 peripheral manual */
    mmio_write32(DATA, 0); 
    usb_wait();
}

void usb_init() {
    printf("USB initialization...\n");
}
