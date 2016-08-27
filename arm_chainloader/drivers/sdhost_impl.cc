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

#include "sd_proto.hpp"
#include "block_device.hpp"

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

#define logf(fmt, ...) printf("[EMMC:%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

struct SdhostImpl : BlockDevice {
	bool is_sdhc;
	bool is_high_capacity;
	bool card_ready;

	uint32_t ocr;
	uint32_t rca;

	uint32_t cid[4];
	uint32_t csd[4];

	uint32_t capacity_bytes;

	uint32_t r[4];

	uint32_t current_cmd;

	void set_power(bool on) {
		SH_VDD = on ? SH_VDD_POWER_ON_SET : 0x0;
	}

	bool wait(uint32_t timeout = 100000) {
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

		current_cmd = command & SH_CMD_COMMAND_SET;

		SH_ARG = arg;
		SH_CMD = command | SH_CMD_NEW_FLAG_SET;

		mfence();

		return true;
	}

	bool send(uint32_t command, uint32_t arg = 0) {
		return send_raw(command & SH_CMD_COMMAND_SET, arg);
	}

	bool send_136_resp(uint32_t command, uint32_t arg = 0) {
		return send_raw((command & SH_CMD_COMMAND_SET) | SH_CMD_LONG_RESPONSE_SET, arg);
	}

	bool send_no_resp(uint32_t command, uint32_t arg = 0) {
		return send_raw((command & SH_CMD_COMMAND_SET) | SH_CMD_NO_RESPONSE_SET, arg);
	}

	void configure_pinmux() {
		GP_FSEL4 = 0x24000000;
		GP_FSEL5 = 0x924;
		GP_PUD = 2;

		logf("GPIOs set!\n");

		mfence();
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

		//printf("Cmd: 0x%x Resp: %08x %08x %08x %08x\n", current_cmd, r[0], r[1], r[2], r[3]);

		if (SH_CMD & SH_CMD_FAIL_FLAG_SET) {
			if (SH_HSTS & SDHSTS_ERROR_MASK) {
				logf("ERROR: sdhost status: 0x%x\n", SH_HSTS);
				return false;
			}
			logf("ERROR: unknown error, SH_CMD=0x%x\n", SH_CMD);
			return false;
		}
		

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

			logf("waiting for SD (0x%x) ...\n", r[0]);
			udelay(100);
		}

		logf("SD card has arrived!\n", r);

		is_high_capacity = (r[0] & MMC_OCR_HCS) == MMC_OCR_HCS;

		if (is_high_capacity)
			logf("This is an SDHC card!\n");

		return true;

	}

	inline void copy_136_to(uint32_t* dest) {
		dest[0] = r[0];
		dest[1] = r[1];
		dest[2] = r[2];
		dest[3] = r[3];
	}

	bool identify_card() {
		logf("identifying card ...\n");
		
		send_136_resp(MMC_ALL_SEND_CID);
		if (!wait_and_get_response())
			return false;

		/* for SD this gets RCA */
		send(MMC_SET_RELATIVE_ADDR);
		if (!wait_and_get_response())
			return false;
		rca = SD_R6_RCA(r);

		logf("RCA = 0x%x\n", rca);

		send_136_resp(MMC_SEND_CID, MMC_ARG_RCA(rca));
		if (!wait_and_get_response())
			return false;

		copy_136_to(cid);

		/* get card specific data */
		send_136_resp(MMC_SEND_CSD, MMC_ARG_RCA(rca));
		if (!wait_and_get_response())
			return false;

		copy_136_to(csd);

		return true;
	}

	inline bool wait_for_data_xfer() {
		for (int i = 0; i < 1000; i++) {
			if (SH_HSTS & SH_HSTS_DATA_FLAG_SET) {
				SH_HSTS = SH_HSTS_DATA_FLAG_SET;
				return true;
			}	
			udelay(100);
		}
		return false;
	}

//#define DUMP_READ

	virtual bool read_block(uint32_t sector, uint32_t* buf) override {
		if (!card_ready)
			panic("card not ready");

#ifdef DUMP_READ
		logf("Reading %d bytes from sector %d ...\n", block_size, sector);
#endif

		if (!is_high_capacity)
			sector <<= 9;

		send_raw(MMC_READ_BLOCK_SINGLE | SH_CMD_READ_CMD_SET | SH_CMD_BUSY_CMD_SET, sector);
		if (!wait()) {
			return false;
		}

		uint32_t remaining = block_size;

#ifdef DUMP_READ
		printf("----------------------------------------------------\n");
#endif

		while (remaining) {
			if (!wait_for_data_xfer()) {
#ifdef DUMP_READ
				putchar('\n');
#endif
				logf("ERROR: Timeout while reading block\n");
				break;
			}
			
			*buf = SH_DATA;
#ifdef DUMP_READ
			printf("%08x ", *buf);
#endif
			buf++;
			remaining -= 4;
		}

#ifdef DUMP_READ
		printf("\n----------------------------------------------------\n");
#endif

		send(MMC_STOP_TRANSMISSION);

		uint32_t sdhsts_errs = SH_HSTS & SDHSTS_ERROR_MASK;
		if (sdhsts_errs) {
			logf("ERROR: Transfer error, status: 0x%x\n", SH_HSTS);
		}

		if (remaining) {
			logf("ERROR: Read failed, %d bytes left to copy\n", remaining);
			return false;
		}
		else {
#ifdef DUMP_READ
			logf("Completed read for %d\n", sector);
#endif
			return true;
		}
	}

	bool select_card() {
		send(MMC_SELECT_CARD, MMC_ARG_RCA(rca));

		if (!wait())
			return false;

		return true;
	}

	bool init_card() {
		char pnm[8];
		uint32_t block_length;

		send_no_resp(MMC_GO_IDLE_STATE | SH_CMD_NO_RESPONSE_SET);

		if (!query_voltage_and_type()) {
			logf("ERROR: Failed to query card voltage!\n");
			return false;
		}

		if (!identify_card()) {
			logf("ERROR: Failed to identify card!\n");
			return false;
		}

		SD_CID_PNM_CPY(cid, pnm);

		logf("Detected SD card:\n");
		printf("    Product : %s\n", &pnm);

		if (SD_CSD_CSDVER(csd) == SD_CSD_CSDVER_2_0) {
			printf("    CSD     : Ver 2.0\n");
			printf("    Capacity: %d\n", SD_CSD_V2_CAPACITY(csd));
			printf("    Size    : %d\n", SD_CSD_V2_C_SIZE(csd));

			block_length = SD_CSD_V2_BL_LEN;
		}
		else if (SD_CSD_CSDVER(csd) == SD_CSD_CSDVER_1_0) {
			printf("    CSD     : Ver 1.0\n");
			printf("    Capacity: %d\n", SD_CSD_CAPACITY(csd));
			printf("    Size    : %d\n", SD_CSD_C_SIZE(csd));

			block_length = 1 << SD_CSD_READ_BL_LEN(csd);
		}
		else {
			printf("ERROR: Unknown CSD version 0x%x!\n", SD_CSD_CSDVER(csd));


			return false;
		}
	
		printf("    BlockLen: 0x%x\n", block_length);

		/* work out the capacity of the card in bytes */
		capacity_bytes = (SD_CSD_CAPACITY(csd) * block_length);

		if (!select_card()) {
			logf("ERROR: Failed to select card!\n");
			return false;
		}

		if (SD_CSD_CSDVER(csd) == SD_CSD_CSDVER_1_0) {
			/*
			 * only needed for 1.0 ones, the 2.0 ones have this 
			 * fixed at 512.
			 */
			logf("Setting block length to 512 ...\n");
			send(MMC_SET_BLOCKLEN, 512);
			if (!wait()) {
				logf("ERROR: Failed to set block length!\n");
				return false;
			}
		}

		block_size = 512;

		logf("Card iniitlization complete: %s %dMB SD%s Card\n", &pnm, capacity_bytes >> 20, is_high_capacity ? "HC" : "");

		return true;
	}

	void restart_controller() {
		is_sdhc = false;

		logf("Restarting the eMMC controller ...\n");

		configure_pinmux();
		reset();

		SH_HCFG &= ~SH_HCFG_WIDE_EXT_BUS_SET;
		SH_HCFG = SH_HCFG_SLOW_CARD_SET | SH_HCFG_WIDE_INT_BUS_SET;
		SH_CDIV = 0x96;

		udelay(300);
		mfence();

		if (init_card()) {
			card_ready = true;
		}
		else {
			panic("failed to reinitialize the eMMC controller");
		}
	}

	SdhostImpl() {
		restart_controller();
		logf("eMMC driver sucessfully started!\n");
	}
};

SdhostImpl STATIC_DRIVER g_SDHostDriver {};

BlockDevice* get_sdhost_device() {
	return &g_SDHostDriver;
}