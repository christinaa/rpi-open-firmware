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
ARM initialization stuff.

=============================================================================*/

#include <common.h>
#include "hardware.h"


#define logf(fmt, ...) printf("[ARMLDR:%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

extern char L_arm_code_start;
extern char L_arm_code_end;

#define ARM_MEMORY_BASE 0xC0000000
#define ARM_BKPT_OPCODE 0xE1200070

/* XXX: What is this? */
#define PM_UNK_CFG_CLR 0xFFFCFFFF

static bool power_wait_bit(uint32_t bit) {
	for (int i = 0; i < 20; i++) {
		if (PM_PROC & bit) {
			return true;
		}
		udelay(100);
	}
	return false;
}

static inline void assert_global_reset() {
	logf("RSTN ...\n");
	PM_PROC |= PM_PASSWORD | PM_PROC_ARMRSTN_SET;
	udelay(300);
}

static void enable_power() {
	uint32_t pmv;

	logf("INIT PM_PROC: 0x%X\n", PM_PROC);

	logf("requesting power up ...\n");

	/* deassert all reset lines */
	pmv = ((PM_PROC & PM_PROC_ARMRSTN_CLR) & PM_UNK_CFG_CLR) | PM_PASSWORD;

	PM_PROC = pmv;

	pmv |= PM_PROC_POWUP_SET;
	udelay(10);
	PM_PROC = pmv;

	logf("POWUP PM_PROC: 0x%X\n", PM_PROC); 

	/* wait for POWOK */
	logf("waiting for power up ...\n");
	for (int i = 1; i < 5; i++) {
		if (!power_wait_bit(PM_PROC_POWOK_SET)) {
			/* only go up to 3 */
			if (i == 4) {
				panic("timed out waiting for power up, state of PM_PROC is: 0x%X", PM_PROC);
			}

			pmv = (pmv & PM_UNK_CFG_CLR) | (i << PM_PROC_CFG_LSB);
			logf("timed out, trying different CFG: 0x%X \n", pmv);
			PM_PROC = pmv;
		}
	}
	
	pmv |= PM_PROC_ISPOW_SET;
	PM_PROC = pmv;

	pmv |= PM_PROC_MEMREP_SET;
	PM_PROC = pmv;

	logf("waiting for MRDONE ...\n");
	if (!power_wait_bit(PM_PROC_MRDONE_SET)) {
		panic("timed out waiting for MRDONE, state of PM_PROC is: 0x%X", PM_PROC);
	}

	logf("setting ISFUNC ...\n");

	pmv |= PM_PROC_ISFUNC_SET;
	PM_PROC = pmv;

	logf("ARM power domain initialized succesfully, state of PM_PROC is: 0x%X!\n", PM_PROC);
}

static void bresp_cycle_write(uint32_t bits) {
	ARM_CONTROL0 = (ARM_CONTROL0 & ~(ARM_C0_BRESP1|ARM_C0_BRESP2)) | bits;
	printf("0x%X,", bits);
	udelay(30);
}

static uint32_t g_BrespTab[] = {
	0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x1C, 0x18, 0x1C, 0x18, 0x0,
	0x10, 0x14, 0x10, 0x1C, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x0,
	0x10, 0x14, 0x10, 0x1C, 0x18, 0x1C, 0x10, 0x14, 0x18, 0x1C, 0x10, 0x14, 0x10, 0x0,
	0x10, 0x14, 0x18, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x0,
	0x10, 0x14, 0x18, 0x14, 0x18, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x18, 0x0
};
static void do_bresp_cycle() {
	/* my little axi - peripherals are magic */
	logf("Cycling AXI bits ...\n\t");
	
	for (int i = 0; i < sizeof(g_BrespTab)/sizeof(g_BrespTab[0]); i++) {
		bresp_cycle_write(g_BrespTab[i]);

		if (i && ((i % 14) == 0))
			printf("\n\t");
	}

	printf("\n");
}

void setup_bridge(bool bresp_cycle) {
	logf("setting up async bridge ...\n");
 
	if (bresp_cycle) {
		assert_global_reset();
		do_bresp_cycle();
		assert_global_reset();
		udelay(300);
	}

	ARM_CONTROL1 &= ~ARM_C1_REQSTOP;
	udelay(300);

	if (!bresp_cycle) 
		assert_global_reset();
	
	logf("bridge init done, PM_PROC is now: 0x%X!\n", PM_PROC);
}

static void set_clock_source(unsigned int source) {
	CM_ARMCTL = CM_PASSWORD | source | CM_ARMCTL_ENAB_SET;
}

static void enable_clock() {
	logf("initializing PLLB ...\n");

	/* oscillator->pllb */
	A2W_XOSC_CTRL |= A2W_PASSWORD | A2W_XOSC_CTRL_PLLBEN_SET;

	A2W_PLLB_FRAC = A2W_PASSWORD | 0xeaaa8;
	A2W_PLLB_CTRL = A2W_PASSWORD | 48 | 0x1000;

	CM_PLLB = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET;
	CM_PLLB = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;

	A2W_PLLB_ANA3 = A2W_PASSWORD | 0x100;
	A2W_PLLB_ANA2 = A2W_PASSWORD | 0x0;
	A2W_PLLB_ANA1 = A2W_PASSWORD | 0x140000;
	A2W_PLLB_ANA0 = A2W_PASSWORD | 0x0;

	A2W_PLLB_DIG3 = A2W_PASSWORD | 0x0;
	A2W_PLLB_DIG2 = A2W_PASSWORD | 0x400000;
	A2W_PLLB_DIG1 = A2W_PASSWORD | 0x3a;
	A2W_PLLB_DIG0 = A2W_PASSWORD | 48 | 0xAAA000;

	A2W_PLLB_CTRL = A2W_PASSWORD | 48 | 0x21000;

	A2W_PLLB_DIG3 = A2W_PASSWORD | 0x2;
	A2W_PLLB_DIG2 = A2W_PASSWORD | 0x402401;
	A2W_PLLB_DIG1 = A2W_PASSWORD | 0x403a;
	A2W_PLLB_DIG0 = A2W_PASSWORD | 48 | 0xAAA000;

	A2W_PLLB_ARM = A2W_PASSWORD | 2;

	CM_PLLB = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET | CM_PLLB_LOADARM_SET;
	CM_PLLB = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;
	CM_PLLB = CM_PASSWORD;

	set_clock_source(4);

	logf("KAIP  = 0x%X\n", A2W_PLLB_ANA_KAIP);
	logf("MULTI = 0x%X\n", A2W_PLLB_ANA_MULTI);

	logf("ARM clock succesfully initialized!\n");
}

static void arm_load_code() {
	uint32_t* mem = (uint32_t*)(ARM_MEMORY_BASE);

	uint8_t* start = &L_arm_code_start;
	uint8_t* end = &L_arm_code_end;
	uint32_t size = (uint32_t)(end - start);

	bcopy(start, mem, size);

	logf("copied %d bytes to 0x%X!\n", size, ARM_MEMORY_BASE);

	/* verify */
	for (int i = 0; i < size; i++) {
		uint8_t* mem8 = (uint8_t*)(mem);
		if (start[i] != mem8[i])
			panic("copy failed at 0x%X expected 0x%X, got 0x%X", (uint32_t)&mem8[i],
				*((uint32_t*)&mem8[i]),
				*((uint32_t*)&start[i]));
	}
}

static void arm_pmap_enter(uint32_t bus_address, uint32_t arm_address) {
	volatile uint32_t* tte = &ARM_TRANSLATE;
	uint32_t index = arm_address >> 24;
	uint32_t pte = bus_address >> 21;

	tte[index] = pte; 

	//logf("Translation: [0x%X => 0x%X] 0x%X => 0x%X\n", index * 4, bus_address >> 21, bus_address, arm_address);
}

/*
#define ARM_C0_PRIO_PER  0x00F00000 // per priority mask
#define ARM_C0_PRIO_L2   0x0F000000
#define ARM_C0_PRIO_UC   0xF0000000
 */

void arm_init() {
	logf("arm init started\n");

	arm_load_code();

	logf("original memstart: 0x%X\n", *((volatile uint32_t*)ARM_MEMORY_BASE));

	for (uint32_t i = 0; i < 62; i++) {
		uint32_t offset = i * 0x1000000;
		arm_pmap_enter(ARM_MEMORY_BASE + offset, 0x0 + offset);
	}

	logf("mapped VC 0x%X to ARM 0x%X\n", ARM_MEMORY_BASE, 0);

	arm_pmap_enter(VC4_PERIPH_BASE, ARM_PERIPH_BASE);

	logf("mapped peripherals VC 0x%X to ARM 0x%X\n", VC4_PERIPH_BASE, ARM_PERIPH_BASE);

	/* see if the ARM block is responding */
	logf("ARM ID: 0x%X C0: 0x%X\n", ARM_ID, ARM_CONTROL0);

	/*
	 * enable peripheral access, map arm secure bits to axi secure bits 1:1 and
	 * set the mem size for who knows what reason.
	 */
	ARM_CONTROL0 |= 0x008 | ARM_C0_APROTSYST | ARM_C0_SIZ1G | ARM_C0_FULLPERI;
        //ARM_CONTROL1 |= ARM_C1_PERSON;

        ARM_IRQ_ENBL3 |= ARM_IE_MAIL;

	logf("using C0: 0x%X\n", ARM_CONTROL0);

	enable_clock();
	enable_power();
	/* start io bridge */
	setup_bridge(true);
	logf("polling ARM state ...\n");

	volatile uint32_t* arm_membase = (volatile uint32_t*)ARM_MEMORY_BASE;

	/* skip vectors and get to comm chan */
	arm_membase += 8;

	for (;;/*int i = 0; i < 10; i++*/) {
		if (arm_membase[3] == VPU_KILL_COMMAND) {
			return;
		}
		logf("0x%X 0x%X 0x%X 0x%X\n", arm_membase[0], arm_membase[1], arm_membase[2], arm_membase[3]);
		udelay(5000);
	}
}
