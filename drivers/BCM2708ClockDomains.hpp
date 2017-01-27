/*
 * VideoCore4_Drivers
 * Copyright (c) 2017 Kristina Brooks
 *
 * PLL VCOs and their channels.
 */

#pragma once
#include <drivers/IODevice.hpp>

struct BCM2708VCO : IODevice {
	volatile uint32_t* dig;
	volatile uint32_t* ana;
	volatile uint32_t* ctrl;
	volatile uint32_t* cmPllCtrl;

	void setDigValues();
	void dumpDigValues();
};