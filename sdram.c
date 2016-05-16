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
VideoCoreIV SDRAM initialization code.

=============================================================================*/

#include "lib/common.h"
#include "hardware.h"


extern uint32_t g_CPUID;

#define MR_REQUEST_SUCCESS(x) ((SD_MR_TIMEOUT_SET & x) != SD_MR_TIMEOUT_SET)
#define MR_GET_RDATA(x) ((x & SD_MR_RDATA_SET) >> SD_MR_RDATA_LSB)

#define RAM_TEST_ADDR 0xC0000000
#define RAM_TEST_PATTERN 0xAAAAAAAA

#define SIP_DEBUG(x) x
#define SCLKU_DEBUG(x) //SIP_DEBUG(x)

ALWAYS_INLINE inline void sdram_clkman_update_begin() {
	CM_SDCCTL |= CM_PASSWORD | CM_SDCCTL_UPDATE_SET;
	SCLKU_DEBUG(printf("%s: waiting for ACCPT (%X) ...\n", __FUNCTION__, CM_SDCCTL));
	for (;;) if (CM_SDCCTL & CM_SDCCTL_ACCPT_SET) break;
	SCLKU_DEBUG(printf("%s: ACCPT received! (%X)\n", __FUNCTION__, CM_SDCCTL));
}

ALWAYS_INLINE inline void sdram_clkman_update_end() {
	CM_SDCCTL = CM_PASSWORD | (CM_SDCCTL & CM_SDCCTL_UPDATE_CLR);
	SCLKU_DEBUG(printf("%s: waiting for ACCPT clear (%X) ...\n", __FUNCTION__, CM_SDCCTL));
	for (;;) if ((CM_SDCCTL & CM_SDCCTL_ACCPT_SET) == 0) break;
	SCLKU_DEBUG(printf("%s: ACCPT cleared! (%X)\n", __FUNCTION__, CM_SDCCTL));
}

ALWAYS_INLINE void sdram_reset_phy_lines() {
	SIP_DEBUG(printf("%s: resetting APHY/DPHY lines ...\n", __FUNCTION__));

	/* politely tell sdc that we'll be messing with address lines */
	APHY_CSR_PHY_BIST_CNTRL_SPR = 0x30;

	DPHY_CSR_GLBL_DQ_DLL_RESET = 0x1;
	APHY_CSR_GLBL_ADDR_DLL_RESET = 0x1;
	
	/* stall ... */
	SD_CS;
	SD_CS;
	SD_CS;
	SD_CS;
	
	DPHY_CSR_GLBL_DQ_DLL_RESET = 0x0;
	APHY_CSR_GLBL_ADDR_DLL_RESET = 0x0;

	SIP_DEBUG(printf("%s: waiting for DPHY master PLL to lock ...\n", __FUNCTION__));
	for (;;) if ((DPHY_CSR_GLBL_MSTR_DLL_LOCK_STAT & 0xFFFF) == 0xFFFF) break;
	SIP_DEBUG(printf("%s: DPHY master PLL locked!\n", __FUNCTION__));
}

void sdram_init_late() {
	uint32_t ctrl = 0x4;

	SD_CS = (SD_CS & ~(SD_CS_DEL_KEEP_SET|SD_CS_DPD_SET|SD_CS_RESTRT_SET)) | SD_CS_STBY_SET;

	/* wait for SDRAM controller to go down */
	SIP_DEBUG(printf("%s: waiting for SDRAM controller to go down (%X) ...\n", __FUNCTION__, SD_CS));
	for (;;) if ((SD_CS & SD_CS_SDUP_SET) == 0) break;
	SIP_DEBUG(printf("%s: SDRAM controller down!\n", __FUNCTION__));

	/* disable SDRAM clock */
	sdram_clkman_update_begin();
	CM_SDCCTL = (CM_SDCCTL & ~(CM_SDCCTL_ENAB_SET|CM_SDCCTL_CTRL_SET)) | CM_PASSWORD;
	sdram_clkman_update_end();

	SIP_DEBUG(printf("%s: SDRAM clock disabled!\n", __FUNCTION__));

	/* left */

	APHY_CSR_DDR_PLL_PWRDWN = 0;
	APHY_CSR_DDR_PLL_GLOBAL_RESET = 0;
	APHY_CSR_DDR_PLL_POST_DIV_RESET = 0;

	APHY_CSR_DDR_PLL_VCO_FREQ_CNTRL0 = (1 << 16) | 0x53 /* magic */;
	APHY_CSR_DDR_PLL_VCO_FREQ_CNTRL1 = 0;
	APHY_CSR_DDR_PLL_MDIV_VALUE = 0;
	APHY_CSR_DDR_PLL_GLOBAL_RESET = 1;

	SIP_DEBUG(printf("%s: waiting for APHY DDR PLL to lock ...\n", __FUNCTION__));
	for (;;) if (APHY_CSR_DDR_PLL_LOCK_STATUS & (1 << 16)) break;
	SIP_DEBUG(printf("%s: APHY DDR PLL locked!\n", __FUNCTION__));

 	APHY_CSR_DDR_PLL_POST_DIV_RESET = 1;

	sdram_clkman_update_begin();

	CM_SDCCTL = CM_PASSWORD | (ctrl << CM_SDCCTL_CTRL_LSB) | (CM_SDCCTL & CM_SDCCTL_CTRL_CLR);
	sdram_clkman_update_end();

	SIP_DEBUG(printf("%s: CM_SDCCTL = 0x%X\n", __FUNCTION__, CM_SDCCTL));

	/* second stage magic values */
	SD_SA = 0x0C293395;
	SD_SB = 0x0F9;
	SD_SC = 0x32200743;
	SD_SD = 0x71810F66;
	SD_SE = 0x10412136;
	SD_PT1 = 0x137B828;
	SD_PT2 = 0x0F96;
	SD_MRT = 0x3;

	sdram_reset_phy_lines();

	/* wait for address line pll to come back */
	SIP_DEBUG(printf("%s: waiting for APHY global PLL to lock ...\n", __FUNCTION__));
	for (;;) if (APHY_CSR_GLBL_ADR_DLL_LOCK_STAT == 3) break;
	SIP_DEBUG(printf("%s: APHY global PLL locked!\n", __FUNCTION__));

	/* tell sdc we're done messing with address lines */
	APHY_CSR_PHY_BIST_CNTRL_SPR = 0x0;

	/* woo, turn on sdram! */
	SD_CS = (0x200042 & ~(SD_CS_STOP_SET|SD_CS_STBY_SET)) | SD_CS_RESTRT_SET;
}

unsigned int sdram_read_mr(unsigned int addr) {
	while ((SD_MR & SD_MR_DONE_SET) != SD_MR_DONE_SET) {}
	SD_MR = addr & 0xFF;
	unsigned int mrr;
	while (((mrr = SD_MR) & SD_MR_DONE_SET) != SD_MR_DONE_SET) {}
	return mrr;
}

unsigned int sdram_write_mr(unsigned int addr, unsigned int data, bool wait) {
	while ((SD_MR & SD_MR_DONE_SET) != SD_MR_DONE_SET) {}

	SD_MR = (addr & 0xFF) | ((data & 0xFF) << 8) | SD_MR_RW_SET;
	
	if (wait) {
		unsigned int mrr;
		while (((mrr = SD_MR) & SD_MR_DONE_SET) != SD_MR_DONE_SET) {}

		if (mrr & SD_MR_TIMEOUT_SET)
			panic("MR write timed out (addr=%d data=0x%X)", addr, data);

		return mrr;
	}
	else {
		return 0;
	}
}

void sdram_reset_phy() {
	printf("%s: resetting SDRAM PHY ...\n", __FUNCTION__);

	/* reset PHYC */
	SD_PHYC = SD_PHYC_PHYRST_SET;
	udelay(64);
	SD_PHYC = 0;

	printf("%s: resetting DPHY CTRL ...\n", __FUNCTION__);

	DPHY_CSR_DQ_PHY_MISC_CTRL = 0x7;
	DPHY_CSR_DQ_PAD_MISC_CTRL = 0x0;
	DPHY_CSR_BOOT_READ_DQS_GATE_CTRL = 0x11;

	sdram_reset_phy_lines();

	APHY_CSR_PHY_BIST_CNTRL_SPR = 0x0;
}

static void sdram_set_clock_source(unsigned int source, unsigned int div_) {
	CM_SDCDIV = CM_PASSWORD | (div_ << CM_SDCDIV_DIV_LSB);
	CM_SDCCTL = CM_PASSWORD | (CM_SDCCTL & CM_SDCCTL_SRC_CLR) | source;
	CM_SDCCTL |= CM_PASSWORD | CM_SDCCTL_ENAB_SET;

	printf("%s: source set to %d, div to %d, waiting for BUSY set (%X) ... \n", __FUNCTION__, source, div_, CM_SDCCTL);

	for (;;) if (CM_SDCCTL & CM_SDCCTL_BUSY_SET) break;

	printf("%s: BUSY set! (%X)\n", __FUNCTION__, CM_SDCCTL);
}

static void sdram_init_clkman()
{
	uint32_t ctrl = 0;

	sdram_clkman_update_begin();
	CM_SDCCTL = CM_PASSWORD | (ctrl << CM_SDCCTL_CTRL_LSB) | (CM_SDCCTL & CM_SDCCTL_CTRL_CLR);
	sdram_clkman_update_end();
}

static const char* lpddr2_manufacturer_name(uint32_t mr) {
	switch (mr) {
		case 1: return "Samsung";
		case 2: return "Qimonda";
		case 3: return "Elpida";
		case 4: return "Etron";
		case 5: return "Nanya";
		case 6: return "Hynix";
		default: return "Unknown";
	}
}

static const char* lpddr2_density(uint32_t mr) {
	/*
	 * i'm so stupid, why did i not notice that LPDDR2
	 * spec listed those as bits, not bytes *sigh*
	 */
	switch ((mr & 0x33) >> 3) {
		case 1: return "128MB";
		case 2: return "256MB";
		case 3: return "512MB";
		case 4: return "1GB";
		case 5: return "2GB";
		case 6: return "4GB";
		default: return "Unknown";
	}
}

static void sdram_calibrate() {
	/* some hw revisions require different slews */
	bool st = ((g_CPUID >> 4) & 0xFFF) == 0x14;
	uint32_t dq_slew = (st ? 2 : 3);

	/* i don't get it, the spec says do not use this register */
	sdram_write_mr(0xFF, 0, true);
	/* RL = 6 / WL = 3 */
	sdram_write_mr(LPDDR2_MR_DEVICE_FEATURE_2, 4, true);

	APHY_CSR_ADDR_PAD_DRV_SLEW_CTRL = 0x333;
	DPHY_CSR_DQ_PAD_DRV_SLEW_CTRL = (dq_slew << 8) | (dq_slew << 4) | 3;

	printf("%s: DPHY_CSR_DQ_PAD_DRV_SLEW_CTRL = 0x%X\n", __FUNCTION__, DPHY_CSR_DQ_PAD_DRV_SLEW_CTRL);

	/* tell sdc we want to calibrate */
	APHY_CSR_PHY_BIST_CNTRL_SPR = 0x20;

	APHY_CSR_ADDR_PVT_COMP_CTRL = 0x1;
	printf("%s: waiting for address PVT calibration ...\n", __FUNCTION__);
	for (;;) if (APHY_CSR_ADDR_PVT_COMP_STATUS & 2) break;

	DPHY_CSR_DQ_PVT_COMP_CTRL = 0x1;
	printf("%s: waiting for data PVT calibration ...\n", __FUNCTION__);
	for (;;) if (DPHY_CSR_DQ_PVT_COMP_STATUS & 2) break;

	/* tell sdc we're done calibrating */
	APHY_CSR_PHY_BIST_CNTRL_SPR = 0x0;

	/* send calibration command */
	uint32_t old_mrt = SD_MRT;
	SD_MRT = 20;
	printf("%s: waiting for SDRAM calibration command ...\n", __FUNCTION__);
	SD_MR = LPDDR2_MR_CALIBRATION | (0xFF << 8) | SD_MR_RW_SET | SD_MR_HI_Z_SET;
	while ((SD_MR & SD_MR_DONE_SET) != SD_MR_DONE_SET) {}
	SD_MRT = old_mrt;

	sdram_write_mr(LPDDR2_MR_IO_CONFIG, is_thing ? 3 : 2, false);
}

static void sdram_selftest()
{
	volatile uint32_t* p = (volatile uint32_t*)RAM_TEST_ADDR;

	printf("Testing SDRAM ...\n");

	for (int i = 0; i < 0x100000; i++) {
		p[i] = RAM_TEST_PATTERN;
		if (p[i] != RAM_TEST_PATTERN)
			panic("sdram initialization failed (idx=%d exptected=0x%x got=0x%x)", i, RAM_TEST_PATTERN, p[i]);
	}

	printf("SDRAM test successful!\n");
}

void sdram_init() {
	uint32_t vendor_id, bc;

	printf("%s: (0) SD_CS = 0x%X\n", __FUNCTION__, SD_CS);

	PM_SMPS = PM_PASSWORD | 0x1;
	A2W_SMPS_LDO1 = A2W_PASSWORD | 0x40000;
	A2W_SMPS_LDO0 = A2W_PASSWORD | 0x0;
	
	A2W_XOSC_CTRL |= A2W_PASSWORD | A2W_XOSC_CTRL_DDREN_SET;

	/*
	 * STEP 1:
	 * configure the low-frequency PLL and enable SDC and perform
	 * the calibration sequence.
	 */

	sdram_set_clock_source(CM_SRC_OSC, 1);
	
	sdram_init_clkman();

	sdram_reset_phy();

	/* magic values */
	SD_SA = 0x6E3395;
	SD_SB = 0x0F9;
	SD_SC = 0x6000431;
	SD_SD = 0x10000011;
	SD_SE = 0x10106000;
	SD_PT1 = 0x0AF002; 
	SD_PT2 = 0x8C;
	SD_MRT = 0x3;
	SD_CS = 0x200042;

	/* wait for SDRAM controller */
	printf("%s: waiting for SDUP (%X) ...\n", __FUNCTION__, SD_CS);
	for (;;) if (SD_CS & SD_CS_SDUP_SET) break;
	printf("%s: SDRAM controller has arrived! (%X)\n", __FUNCTION__, SD_CS);
	
	/* RL = 6 / WL = 3 */
	sdram_write_mr(LPDDR2_MR_DEVICE_FEATURE_2, 4, false);
	sdram_calibrate();

	/* identify installed memory */
	vendor_id = sdram_read_mr(LPDDR2_MR_MANUFACTURER_ID);
	if (!MR_REQUEST_SUCCESS(vendor_id)) {
		panic("vendor id memory register read timed out");
	}
	vendor_id = MR_GET_RDATA(vendor_id);

	bc = sdram_read_mr(LPDDR2_MR_METRICS);
	if (!MR_REQUEST_SUCCESS(bc)) {
		panic("basic configuration memory register read timed out");
	}
	bc = MR_GET_RDATA(bc);

	printf("SDRAM Type: %s %s LPDDR2 (BC=0x%X)\n",
		lpddr2_manufacturer_name(vendor_id),
		lpddr2_density(bc),
		bc);

	/*
	 * STEP 2:
	 * after calibration, enable high-freq SDRAM PLL. because we're
	 * running from cache, we can freely mess with SDRAM clock without
	 * any issues, removing the need to copy the SDRAM late init stuff
	 * to bootrom ram. if later code that's running from SDRAM wants to
	 * mess with SDRAM clock it would need to do that.
	 */

	sdram_init_late();
	sdram_selftest();
}

