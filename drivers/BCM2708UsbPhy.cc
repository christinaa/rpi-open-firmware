/*
 * VideoCore4_Drivers
 * Copyright (c) 2017 Authors of rpi-open-firmware
 *
 * USB PHY initialization driver.
 */

#include <drivers/IODevice.hpp>
#include <drivers/BCM2708PowerManagement.hpp>

#define FLAG_BUSY (1 << 31)

struct BCM2708UsbPhy : IODevice {
	IODriverConstructor(BCM2708UsbPhy);

	void wait() {
		while(USB_GMDIOCSR & FLAG_BUSY);
	}

	void write_bare(int reg, uint16_t value, int type) {
		reg &= 0x1F;

		/* precede MDIO access */
		USB_GMDIOGEN = 0xFFFFFFFF;
		wait();

		/* write the actual value, with flags */
		USB_GMDIOGEN = type | (reg << 18) | value;
		wait();

		/* dummy write due to errata; see BCM2835 peripheral manual */
		USB_GMDIOGEN = 0; 
		wait();
	}

	uint16_t usb_read(int reg) {
		write_bare(reg, 0, 0x60020000);
		return USB_MDIO_CSR & 0x3FF;
	}

	virtual void usb_write(int reg, uint16_t value) {
		IODriverLog("Writing 0x%X to reg 0x%X", value, reg);
		write_bare(reg, value, 0x50020000);
	}

	virtual void start() override {
		IODriverLog("starting ...");

		USB_GMDIOCSR = (1 << 18);

		usb_write(0x15, 4369/*cond ? 4569 : 272*/);
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

		IODriverLog("started");

		IODevice::start();
	}

	virtual void init() override {
		setName("BCM2708UsbPhy");
		setTag('USBP');
	}
};

IODriverCreateSingletonInstance(BCM2708UsbPhy);

