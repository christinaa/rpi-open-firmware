/*=============================================================================
Copyright (C) 2017 Alyssa Rosenzweig 
All rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

FILE DESCRIPTION
USB bringup code

=============================================================================*/

#include <common.h>
#include <hardware.h>

#define FLAG_BUSY (1 << 31)

void usb_wait() {
    while(USB_MDIO_CSR & FLAG_BUSY);
}

void usb_write_bare(int reg, uint16_t value, int type) {
    reg &= 0x1F;
    printf("reg 0x%X\n", reg);

    /* precede MDIO access */
    USB_MDIO_GEN = 0xFFFFFFFF;
    printf("preceded\n");
    usb_wait();
    printf("w\n");

    /* write the actual value, with flags */
    USB_MDIO_GEN = type | (reg << 12) | value;
    printf("data\n");
    usb_wait();
    printf("w\n");

    /* dummy write due to errata; see BCM2835 peripheral manual */
    USB_MDIO_GEN = 0; 
    printf("dummy\n");
    usb_wait();
    printf("w\n");
}

uint16_t usb_read(int reg) {
    usb_write_bare(reg, 0, 0x60020000);
    return USB_MDIO_CSR & 0x3FF;
}

void usb_write(int reg, uint16_t value) {
    printf("Writing 0x%X to reg 0x%X\n", value, reg);
    usb_write_bare(reg, value, 0x50020000);
}

void usb_init() {
    printf("USB initialization...\n");

    printf("Powering on USB domain..\n");
    printf("PM_USB = %X\n", PM_USB);

    A2W_XOSC_CTRL |= A2W_PASSWORD | A2W_XOSC_CTRL_USBEN_SET;
    udelay(300);

    PM_USB = PM_PASSWORD | PM_USB_CTRLEN_SET;
    udelay(300);
    printf("PM_USB = %X\n", PM_USB);
    printf("Clocking..\n");
    //CM_TDCLKEN = CM_PASSWORD | CM_TDCLKEN_USBDFT_SET;
    udelay(300);
    printf("Controlling..\n");
    //USB_MDIO_CSR = 1 << 18;
    //USB_MDIO_GEN = 0;
    for(;;);
    printf("and mdio..\n");

    bool cond = false; /* TODO */
    usb_write(0x15, cond ? 4569 : 272);
    usb_write(0x19, 0x4);
    usb_write(0x18, 0x10);
    usb_write(0x1D, 0x4);
    usb_write(0x17, 5682);

    while(!usb_read(0x1B) & 0x7);

    usb_write(0x1E, 0x01);

    usb_write(0x1D, 0x5000);
    usb_write(0x19, 0xC004);
    usb_write(0x32, 0x1C2F);
    usb_write(34, 256);
    usb_write(36, 0x10);
    usb_write(0x19, 0x04);

    USB_GUSBCFG = 0x40402700;
    delay(300);
    USB_HCFG = 0x20402700;
    printf("USB initialization done!\n");
}
