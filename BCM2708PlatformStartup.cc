/*
 * VideoCore4_Drivers
 * Copyright (c) 2017 Kristina Brooks
 *
 * BCM2708 power management driver.
 */

#include <drivers/IODevice.hpp>
#include <drivers/BCM2708PowerManagement.hpp>

extern "C" void PEStartPlatform() {
	IODevice* imagePm = PowerManagementDomain::getDeviceForDomain(kCprPowerDomainImage);
	assert(imagePm);
	IODevice* usbPm = PowerManagementDomain::getDeviceForDomain(kCprPowerDomainUSB);
	assert(usbPm);

	/*
	 * all devices in the IMAGE domain have to be disabled before
	 * starting the domain itself.
	 */
	usbPm->stop();
	/*
	 * enable IMAGE power domain.
	 */
	imagePm->start();
	/*
	 * enable USB power domain.
	 */
	usbPm->start();

	/*
	 * start up USB PHY.
	 */
	IODevice* usbPhy = IODevice::findByTag('USBP');
	assert(usbPhy);
	usbPhy->start();
}