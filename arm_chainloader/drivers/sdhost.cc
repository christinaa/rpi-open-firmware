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
SDHOST driver. This used to be known as ALTMMC.

=============================================================================*/

#include <chainloader.h>
#include <hardware.h>

#include "sd2.hpp"

#define SDEDM_WRITE_THRESHOLD_SHIFT 9
#define SDEDM_READ_THRESHOLD_SHIFT 14
#define SDEDM_THRESHOLD_MASK     0x1f

#define SAFE_READ_THRESHOLD     4
#define SAFE_WRITE_THRESHOLD    4

#define logf(fmt, ...) printf("[sdhost::%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

struct sdhost_t {
	void set_power(bool on) {
		SH_VDD = on ? SH_VDD_POWER_ON_SET : 0x0;
	}

	bool wait(uint32_t timeout = 10000) {
		uint32_t t = timeout;

		while(SH_CMD & SH_CMD_NEW_FLAG_SET) {
			if (t == 0) {
				logf("timed out after %dus!\n", timeout)
				return false;
			}
			t--;
			udelay(10);
		}

		return true;
	}

	bool send(uint32_t command, uint32_t arg) {
		if (!wait())
			return false;

		SH_ARG = arg;
		SH_CMD = (command & SH_CMD_COMMAND_SET) | SH_CMD_NEW_FLAG_SET;

		udelay(300);
		mfence();

		return true;
	}

	void configure_pinmux() {
		GP_FSEL4 = 0x24000000;
		GP_FSEL5 = 0x924;
		GP_PUD = 2;

		logf("GPIOs set!\n");
	}

	void reset() {
		logf("resetting controller ...\n");

		set_power(false);

		SH_CMD = 0;
		SH_ARG = 0;
		SH_TOUT = 0xF00000;
		SH_CDIV = 0;
		SH_HSTS = 0x7f8;
		SH_HCFG = 0;
		SH_HBCT = 0;
		SH_HBLC = 0;
	
		uint32_t temp = SH_EDM;

		temp &= ~((SDEDM_THRESHOLD_MASK<<SDEDM_READ_THRESHOLD_SHIFT) |
		  (SDEDM_THRESHOLD_MASK<<SDEDM_WRITE_THRESHOLD_SHIFT));
		temp |= (SAFE_READ_THRESHOLD << SDEDM_READ_THRESHOLD_SHIFT) |
			(SAFE_WRITE_THRESHOLD << SDEDM_WRITE_THRESHOLD_SHIFT);

		SH_EDM = temp;
		udelay(300);

		set_power(true);

		udelay(300);
		mfence();
	}

	void init_card() {
		send(GO_IDLE_STATE, 0);
		send(SEND_IF_COND, 0x155);

		wait();

		logf("SEND_IF_COND response is: 0x%X\n", SH_RSP0);
	}

	sdhost_t() {
		logf("starting up ...\n");

		configure_pinmux();
		reset();

		SH_HCFG = SH_HCFG_SLOW_CARD_SET | SH_HCFG_WIDE_INT_BUS_SET;
		SH_CDIV = 0x96;

		udelay(300);

		logf("sdhost controller ready!\n");

		init_card();
	}
};

sdhost_t STATIC_DRIVER g_SDHostDriver {};