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

#define VOLTAGE_SUPPLY_RANGE 0x100
#define CHECK_PATTERN 0x55


#define SDHSTS_BUSY_IRPT                0x400
#define SDHSTS_BLOCK_IRPT               0x200
#define SDHSTS_SDIO_IRPT                0x100
#define SDHSTS_REW_TIME_OUT             0x80
#define SDHSTS_CMD_TIME_OUT             0x40
#define SDHSTS_CRC16_ERROR              0x20
#define SDHSTS_CRC7_ERROR               0x10
#define SDHSTS_FIFO_ERROR               0x08

#define SDHSTS_TRANSFER_ERROR_MASK      (SDHSTS_CRC7_ERROR|SDHSTS_CRC16_ERROR|SDHSTS_REW_TIME_OUT|SDHSTS_FIFO_ERROR)
#define SDHSTS_ERROR_MASK               (SDHSTS_CMD_TIME_OUT|SDHSTS_TRANSFER_ERROR_MASK)

#define logf(fmt, ...) printf("[sdhost::%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

struct sdhost_t {
	bool is_sdhc;
	bool is_high_capacity;

	uint32_t ocr;
	uint32_t rca;

	uint32_t cid[4];
	uint32_t csd[4];

	uint32_t r[4];

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

	bool send_raw(uint32_t command, uint32_t arg = 0) {
		uint32_t sts;

		wait();

		sts = SH_HSTS;
		if (sts & SDHSTS_ERROR_MASK)
			SH_HSTS = sts; 

		SH_ARG = arg;
		SH_CMD = command | SH_CMD_NEW_FLAG_SET;

		mfence();

		return true;
	}

	bool send(uint32_t command, uint32_t arg = 0) {
		return send_raw(command & SH_CMD_COMMAND_SET, arg);
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

	inline void get_response() {
		r[0] = SH_RSP0;
		r[1] = SH_RSP1;
		r[2] = SH_RSP2;
		r[3] = SH_RSP3;
	}

	bool wait_and_get_response() {
		if (!wait())
			return false;

		get_response();

		logf("HSTS: 0x%x, RSP: %08x %08x %08x %08x\n", SH_HSTS, r[0], r[1], r[2], r[3]);

		return true;
	}

	bool query_voltage_and_type() {
		uint32_t t;

		/* identify */
		send(SD_SEND_IF_COND, 0x1AA);
		wait_and_get_response();

		/* set voltage */
		t = MMC_OCR_3_2V_3_3V;
		if (r[0] == 0x1AA) {
			t |= MMC_OCR_HCS;
			is_sdhc = true;
		}

		/* query voltage and type */
		for (;;) {
			send(MMC_APP_CMD);
			send(SD_APP_OP_COND, t);

			if (!wait_and_get_response())
				return false;

			if (r[0] & MMC_OCR_MEM_READY)
				break;

			logf("waiting for sd card (0x%x) ...\n", r[0]);
			udelay(100);
		}

		logf("sd card has arrived!\n", r);

		is_high_capacity = (r[0] & MMC_OCR_HCS) == MMC_OCR_HCS;

		if (is_high_capacity)
			logf("this is an SDHC card!\n");

		return true;

	}

	bool identify_card() {
		logf("identifying card ...\n");
		
		send_raw(SH_CMD_LONG_RESPONSE_SET | MMC_ALL_SEND_CID);
		if (!wait_and_get_response())
			return false;

		cid[0] = r[0];
		cid[1] = r[1];
		cid[2] = r[2];
		cid[3] = r[3];

		send(MMC_SET_RELATIVE_ADDR);

		if (!wait_and_get_response())
			return false;

		rca = SD_R6_RCA(r);

		/* get card specific data */

		send_raw(SH_CMD_LONG_RESPONSE_SET | MMC_SEND_CSD, MMC_ARG_RCA(rca));
		if (!wait_and_get_response())
			return false;

		csd[0] = r[0];
		csd[1] = r[1];
		csd[2] = r[2];
		csd[3] = r[3];

		return true;
	}

	bool select_card() {
		send(MMC_SELECT_CARD, MMC_ARG_RCA(rca));

		if (!wait())
			return false;

		return true;
	}

	bool init_card() {
		char pnm[8];

		send(MMC_GO_IDLE_STATE);

		if (!query_voltage_and_type()) {
			logf("ERROR: Failed to query card voltage!\n");
			return false;
		}

		if (!identify_card()) {
			logf("ERROR: Failed to identify card!\n");
			return false;
		}

		SD_CID_PNM_CPY(cid, pnm);
		logf("Product : %s\n", &pnm);
		logf("MMC Ver : %d\n", SD_CSD_MMCVER(csd));

		logf("MMC Size: %d\n", MMC_CSD_C_SIZE(csd));
		logf("MMC Cap : %d\n", MMC_CSD_CAPACITY(csd));
		
		if (SD_CSD_CSDVER(csd) == SD_CSD_CSDVER_2_0) {
			logf("CSD     : version 2.0\n");
			logf("Capacity: %d\n", SD_CSD_V2_CAPACITY(csd));
			logf("Size    : %d\n", SD_CSD_V2_C_SIZE(csd));
		}
		else if (SD_CSD_CSDVER(csd) == SD_CSD_CSDVER_1_0) {
			logf("CSD     : version 1.0\n");
			logf("Capacity: %d\n", SD_CSD_CAPACITY(csd));
			logf("Size    : %d\n", SD_CSD_C_SIZE(csd));
		}
		else {
			logf("ERROR: unknown CSD version 0x%x!\n", SD_CSD_CSDVER(csd));
			return false;
		}

		if (!select_card()) {
			logf("ERROR: Failed to select card!\n");
			return false;
		}

		logf("setting block length to 512 ...\n");
		send(MMC_SET_BLOCKLEN, 512);
		if (!wait()) {
			logf("ERROR: Failed to set block length!\n");
			return false;
		}

		logf("card initialization complete!\n");

		return true;
	}

	sdhost_t() {
		is_sdhc = false;

		logf("starting up ...\n");

		configure_pinmux();
		reset();

		SH_HCFG = SH_HCFG_SLOW_CARD_SET | SH_HCFG_WIDE_INT_BUS_SET;
		SH_CDIV = 0x96;

		udelay(300);

		logf("sdhost controller ready, initializing SD card ...\n");

		if (init_card()) {
			
		}
	}
};

sdhost_t STATIC_DRIVER g_SDHostDriver {};

/*
 * abstractions for fatfs.
 */

#include "fatfs/diskio.h"

DSTATUS disk_initialize (BYTE pdrv) {
	return 0;
}

DSTATUS disk_status (BYTE pdrv) {
	return 0;
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
	return (DRESULT)0;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff) {
	return (DRESULT)0;
}