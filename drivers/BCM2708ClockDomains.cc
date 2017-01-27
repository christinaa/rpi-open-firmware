/*
 * VideoCore4_Drivers
 * Copyright (c) 2017 Kristina Brooks
 *
 * PLL VCOs and their channels.
 */

#include <drivers/BCM2708ClockDomains.hpp>

/*
PLL/Channel tree:
|-XOSC
 |-PLLA
   |-CORE
   |-PER
   |-CCP2
   |-DSI0
 |-PLLB
   |-ARM
     |-ARM Clock (CM_ARM*)
   |-SP0 (???)
   |-SP1 (???)
   |-SP2 (???)
 |-PLLC
   |-CORE0
   |-CORE1
   |-PER
   |-CORE2 (???)
 |-PLLD
   |-DSI0
   |-DSI1
   |-CORE
   |-PER
 |-PLLH
   |-AUX
   |-PIX
   |-RCAL
   |-STS
 */

/***********************************************************************
 * 
 * Common VCO stuff.
 *
 ***********************************************************************/

void BCM2708VCO::setDigValues() {
	IODriverLog("setting DIG values for this VCO ...");

	uint32_t dig0 = dig[0],
		dig1 = dig[1],
		dig2 = dig[2],
		dig3 = dig[3];

	/* 0xAA covers all possible HOLD bits in CM_PLLx regs */
	*cmPllCtrl = CM_PASSWORD | 0xAA;
	
	dig[3] = A2W_PASSWORD | (dig3);
	dig[2] = A2W_PASSWORD | (dig2 & 0xFFEFFBFE);
	dig[1] = A2W_PASSWORD | (dig1 & ~(1 << 14));
	dig[0] = A2W_PASSWORD | (dig0);

	*ctrl = A2W_PASSWORD | 0x20000 | *ctrl;

	dig3 |= 0x42;

	dig[3] = A2W_PASSWORD | (dig3);
	dig[2] = A2W_PASSWORD | (dig2);
	dig[1] = A2W_PASSWORD | (dig1);
	dig[0] = A2W_PASSWORD | (dig0);
}

void BCM2708VCO::dumpDigValues() {
	IODriverLog("DIG0: 0x%X, DIG1: 0x%X, DIG2: 0x%X, DIG3: 0x%X",
		dig[0],
		dig[1],
		dig[2],
		dig[3]);
}

/***********************************************************************
 * 
 * PLL VCOs for specific PLLs.
 *
 ***********************************************************************/

struct BCM2708PLLB : BCM2708VCO {
	IODriverConstructor(BCM2708PLLB);

	/*
	 * Working: DIG0: 0xAAA030, DIG1: 0x403A, DIG2: 0x402401, DIG3: 0x2
	 */

	virtual void init() override {
		ctrl = RegToRef(A2W_PLLB_CTRL);
		ana = RegToRef(A2W_PLLB_ANA0);
		dig = RegToRef(A2W_PLLB_DIG0);
		cmPllCtrl = RegToRef(CM_PLLB);

		IODriverLog("PLLB VCO registered");

		setTag('PLLB');
	}
};
IODriverCreateSingletonInstance(BCM2708PLLB);

/***********************************************************************
 * 
 * ARM Clock.
 *
 ***********************************************************************/

struct BCM2708ClockDomain : IODevice {
	volatile uint32_t* cmDiv;
	volatile uint32_t* cmCtrl;
};

struct BCM2708ArmClockDomain : BCM2708ClockDomain {
	
};
