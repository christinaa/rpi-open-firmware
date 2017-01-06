/*=============================================================================
Copyright (C) 2016-2017 Authors of rpi-open-firmware
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

#define SDEDM_FSM_MASK           0xf
#define SDEDM_FSM_IDENTMODE      0x0
#define SDEDM_FSM_DATAMODE       0x1
#define SDEDM_FSM_READDATA       0x2
#define SDEDM_FSM_WRITEDATA      0x3
#define SDEDM_FSM_READWAIT       0x4
#define SDEDM_FSM_READCRC        0x5
#define SDEDM_FSM_WRITECRC       0x6
#define SDEDM_FSM_WRITEWAIT1     0x7
#define SDEDM_FSM_POWERDOWN      0x8
#define SDEDM_FSM_POWERUP        0x9
#define SDEDM_FSM_WRITESTART1    0xa
#define SDEDM_FSM_WRITESTART2    0xb
#define SDEDM_FSM_GENPULSES      0xc
#define SDEDM_FSM_WRITEWAIT2     0xd
#define SDEDM_FSM_STARTPOWDOWN   0xf

#define SDHSTS_TRANSFER_ERROR_MASK      (SDHSTS_CRC7_ERROR|SDHSTS_CRC16_ERROR|SDHSTS_REW_TIME_OUT|SDHSTS_FIFO_ERROR)
#define SDHSTS_ERROR_MASK               (SDHSTS_CMD_TIME_OUT|SDHSTS_TRANSFER_ERROR_MASK)

#define logf(fmt, ...) printf("[GPIO:%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

enum BCM2708PinmuxSetting {
	kBCM2708PinmuxIn = 0,
	kBCM2708PinmuxOut = 1,
	kBCM2708Pinmux_ALT5 = 2,
	kBCM2708Pinmux_ALT4 = 3,
	kBCM2708Pinmux_ALT0 = 4,
	kBCM2708Pinmux_ALT1 = 5,
	kBCM2708Pinmux_ALT2 = 6,
	kBCM2708Pinmux_ALT3 = 7
};

struct BCM2708GPIO {

	static void set(uint32_t pin_num, BCM2708PinmuxSetting setting) {
		uint32_t* fsel = reinterpret_cast<uint32_t*>(
		                     reinterpret_cast<uint32_t>(&GP_FSEL0) + (0x4 * (pin_num/10))
		                 );
		uint32_t pin_shift = (pin_num % 10) * 3;



	}
};

#undef logf
#define logf(fmt, ...) printf("[EMMC:%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

#define kIdentSafeClockRate 0x148

struct BCM2708SDHost : BlockDevice {
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

		logf("waiting for pinmux pull update ...\n");

		GP_PUD = 2;
		mfence();
		udelay(500);
		GP_PUD = 0;

		logf("waiting for pinmux clock update ...\n");

		/* are these in bank 1 or 2? ah who gives a fuck ... */
		GP_PUDCLK1 = GP_PUDCLK1_PUDCLKn32_SET;
		GP_PUDCLK2 = GP_PUDCLK2_PUDCLKn64_SET;
		udelay(500);

		logf("ok ...\n");
		GP_PUDCLK1 = 0;
		GP_PUDCLK2 = 0;

		logf("pinmux configured for aux0\n");
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
		t = MMC_OCR_3_3V_3_4V;
		if (r[0] == 0x1AA) {
			t |= MMC_OCR_HCS;
			is_sdhc = true;
		}

		/* query voltage and type */
		for (;;) {
			send(MMC_APP_CMD); /* 55 */
			send_no_resp(SD_APP_OP_COND, t);

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

//#define DUMP_READ

	bool wait_for_fifo_data(uint32_t timeout = 100000) {
		uint32_t t = timeout;

		while ((SH_HSTS & SH_HSTS_DATA_FLAG_SET) == 0) {
			if (t == 0) {
				putchar('\n');
				logf("ERROR: no FIFO data, timed out after %dus!\n", timeout)
				return false;
			}
			t--;
			udelay(10);
		}

		return true;
	}

	void drain_fifo() {
		/* fuck me with a rake ... gently */

		wait();

		while (SH_HSTS & SH_HSTS_DATA_FLAG_SET) {
			SH_DATA;
			mfence();
		}
	}

	void drain_fifo_nowait() {
		while (true) {
			SH_DATA;

			uint32_t hsts = SH_HSTS;
			if (hsts != SH_HSTS_DATA_FLAG_SET)
				break;
		}
	}

	virtual bool read_block(uint32_t sector, uint32_t* buf) override {
		if (!card_ready)
			panic("card not ready");

		if (!is_high_capacity)
			sector <<= 9;

		/* drain junk from FIFO */
		drain_fifo();

		/* enter READ mode */
		send_raw(MMC_READ_BLOCK_MULTIPLE | SH_CMD_READ_CMD_SET, sector);

		int i;
		uint32_t hsts_err = 0;

#ifdef DUMP_READ
		if (buf)
			logf("Reading %d bytes from sector %d using FIFO ...\n", block_size, sector);
#endif

#ifdef DUMP_READ
		if (buf)
			printf("----------------------------------------------------\n");
#endif

		/* drain useful data from FIFO */
		for (i = 0; i < 128; i++) {
			/* wait for FIFO */
			if (!wait_for_fifo_data()) {
				break;
			}

			uint32_t hsts_err = SH_HSTS & SDHSTS_ERROR_MASK;
			if (hsts_err) {
				logf("ERROR: transfer error on FIFO word %d: 0x%x\n", i, SH_HSTS);
				break;
			}


			volatile uint32_t data = SH_DATA;

#ifdef DUMP_READ
			printf("%08x ", data);
#endif
			if (buf)
				*(buf++) = data;
		}

		send_raw(MMC_STOP_TRANSMISSION | SH_CMD_BUSY_CMD_SET);

#ifdef DUMP_READ
		if (buf)
			printf("\n----------------------------------------------------\n");
#endif

		if (hsts_err) {
			logf("ERROR: Transfer error, status: 0x%x\n", SH_HSTS);
			return false;
		}

#ifdef DUMP_READ
		if (buf)
			logf("Completed read for %d\n", sector);
#endif
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
		uint32_t block_length;
		uint32_t clock_div = 0;

		send_no_resp(MMC_GO_IDLE_STATE);

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

			block_length = 1 << SD_CSD_V2_BL_LEN;

			/* work out the capacity of the card in bytes */
			capacity_bytes = (SD_CSD_V2_CAPACITY(csd) * block_length);

			clock_div = 5;
		} else if (SD_CSD_CSDVER(csd) == SD_CSD_CSDVER_1_0) {
			printf("    CSD     : Ver 1.0\n");
			printf("    Capacity: %d\n", SD_CSD_CAPACITY(csd));
			printf("    Size    : %d\n", SD_CSD_C_SIZE(csd));

			block_length = 1 << SD_CSD_READ_BL_LEN(csd);

			/* work out the capacity of the card in bytes */
			capacity_bytes = (SD_CSD_CAPACITY(csd) * block_length);

			clock_div = 10;
		} else {
			printf("ERROR: Unknown CSD version 0x%x!\n", SD_CSD_CSDVER(csd));
			return false;
		}

		printf("    BlockLen: 0x%x\n", block_length);

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

		logf("Card initialization complete: %s %dMB SD%s Card\n", &pnm, capacity_bytes >> 20, is_high_capacity ? "HC" : "");

		/*
		 * this makes some dangerous assumptions that the all csd2 cards are sdio cards
		 * and all csd1 cards are sd cards and that mmc cards won't be used. this also assumes
		 * PLLC.CORE0 is at 250MHz which is probably a safe assumption since we set it.
		 */
		if (clock_div) {
			logf("Identification complete, changing clock to %dMHz for data mode ...\n", 250 / clock_div);
			SH_CDIV = clock_div - 2;
		}

		return true;
	}

	void restart_controller() {
		is_sdhc = false;

		logf("hcfg 0x%X, cdiv 0x%X, edm 0x%X, hsts 0x%X\n",
		     SH_HCFG,
		     SH_CDIV,
		     SH_EDM,
		     SH_HSTS);

		logf("Restarting the eMMC controller ...\n");

		configure_pinmux();
		reset();

		SH_HCFG &= ~SH_HCFG_WIDE_EXT_BUS_SET;
		SH_HCFG = SH_HCFG_SLOW_CARD_SET | SH_HCFG_WIDE_INT_BUS_SET;
		SH_CDIV = kIdentSafeClockRate;

		udelay(300);
		mfence();

		if (init_card()) {
			card_ready = true;

			/*
			 * looks like a silicon bug to me or a quirk of csd2, who knows
			 */
			for (int i = 0; i < 3; i++) {
				if (!read_block(0, nullptr)) {
					panic("fifo flush cycle %d failed", i);
				}
			}
		} else {
			panic("failed to reinitialize the eMMC controller");
		}
	}

	virtual void stop() override {
		if (card_ready) {
			logf("flushing fifo ...\n");
			drain_fifo_nowait();

			logf("asking card to enter idle state ...\n");
			SH_CDIV = kIdentSafeClockRate;
			udelay(150);

			send_no_resp(MMC_GO_IDLE_STATE);
			udelay(500);
		}

		logf("stopping sdhost controller driver ...\n");

		SH_CMD = 0;
		SH_ARG = 0;
		SH_TOUT = 0xA00000;
		SH_CDIV = 0x1FB;

		logf("powering down controller ...\n");
		SH_VDD = 0;
		SH_HCFG = 0;
		SH_HBCT = 0x400;
		SH_HBLC = 0;
		SH_HSTS = 0x7F8;

		logf("resetting state machine ...\n");

		SH_CMD = 0;
		SH_ARG = 0;
	}

	BCM2708SDHost() {
		restart_controller();
		logf("eMMC driver sucessfully started!\n");
	}
};

BCM2708SDHost STATIC_DRIVER g_SDHostDriver {};

BlockDevice* get_sdhost_device() {
	return &g_SDHostDriver;
}