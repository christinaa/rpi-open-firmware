/*
 * VideoCore4_Drivers
 * Copyright (c) 2017 Kristina Brooks
 *
 * ARM control driver.
 */

#include <drivers/IODevice.hpp>
#include <drivers/BCM2708PowerManagement.hpp>

#define FLAG_BUSY (1 << 31)

struct BCM2708ArmControl : IODevice {
	IODriverConstructor(BCM2708ArmControl);

	virtual void start() override {
		IODriverLog("starting ...");

		IODevice::start();
	}

	virtual void init() override {
		setName("BCM2708ArmControl");
		setTag('ARMC');
	}
};

IODriverCreateSingletonInstance(BCM2708ArmControl);

