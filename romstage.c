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
VideoCoreIV first stage bootloader.

=============================================================================*/

#include <common.h>
#include <hardware.h>

uint32_t g_CPUID;

#define UART_DR     (UART_BASE+0x00)
#define UART_RSRECR (UART_BASE+0x04)
#define UART_FR     (UART_BASE+0x18)
#define UART_ILPR   (UART_BASE+0x20)
#define UART_IBRD   (UART_BASE+0x24)
#define UART_FBRD   (UART_BASE+0x28)
#define UART_LCRH   (UART_BASE+0x2C)
#define UART_CR     (UART_BASE+0x30)
#define UART_IFLS   (UART_BASE+0x34)
#define UART_IMSC   (UART_BASE+0x38)
#define UART_RIS    (UART_BASE+0x3C)
#define UART_MIS    (UART_BASE+0x40)
#define UART_ICR    (UART_BASE+0x44)
#define UART_DMACR  (UART_BASE+0x48)
#define UART_ITCR   (UART_BASE+0x80)
#define UART_ITIP   (UART_BASE+0x84)
#define UART_ITOP   (UART_BASE+0x88)
#define UART_TDR    (UART_BASE+0x8C)

void uart_putc(unsigned int ch)
{
	while(UART_MSR & 0x20) break;
	UART_RBRTHRDLL = ch;
}

void uart_init(void) {
	unsigned int ra = GP_FSEL1;
	ra &= ~(7 << 12);
	ra |= 4 << 12;
	GP_FSEL1 = ra;

        CM_UARTCTL = CM_PASSWORD | CM_SRC_OSC | CM_UARTCTL_FRAC_SET;
        udelay(150);
        CM_UARTDIV = CM_PASSWORD | 0x6666;
        udelay(150);
        CM_UARTCTL |= CM_UARTCTL_ENAB_SET;
        udelay(150);
        
        mmio_write32(UART_CR, 0);

        GP_PUD = 0;
	udelay(150);
	GP_PUDCLK0 = (1 << 14) | (1 << 15);
	udelay(150);
	GP_PUDCLK0 = 0;

        mmio_write32(UART_IBRD, 1);
        mmio_write32(UART_FBRD, 40);
        mmio_write32(UART_ICR, 0x7FF);
        mmio_write32(UART_LCRH, 0x70);
        mmio_write32(UART_CR, 0x301);
}

void led_init(void) {
	unsigned int ra;

	ra = GP_FSEL1;
	ra &= ~(7 << 18);
	ra |= 1 << 18;

	GP_FSEL1 = ra;
}

/*
  #define CM_PLLC_DIGRST_BITS                                9:9
  #define CM_PLLC_DIGRST_SET                                 0x00000200
  #define CM_PLLC_ANARST_BITS                                8:8
  #define CM_PLLC_ANARST_SET                                 0x00000100
  #define CM_PLLC_HOLDPER_BITS                               7:7
  #define CM_PLLC_HOLDPER_SET                                0x00000080
  #define CM_PLLC_LOADPER_BITS                               6:6
  #define CM_PLLC_LOADPER_SET                                0x00000040
  #define CM_PLLC_HOLDCORE2_BITS                             5:5
  #define CM_PLLC_HOLDCORE2_SET                              0x00000020
  #define CM_PLLC_LOADCORE2_BITS                             4:4
  #define CM_PLLC_LOADCORE2_SET                              0x00000010
  #define CM_PLLC_HOLDCORE1_BITS                             3:3
  #define CM_PLLC_HOLDCORE1_SET                              0x00000008
  #define CM_PLLC_LOADCORE1_BITS                             2:2
  #define CM_PLLC_LOADCORE1_SET                              0x00000004
  #define CM_PLLC_HOLDCORE0_BITS                             1:1
  #define CM_PLLC_HOLDCORE0_SET                              0x00000002
  #define CM_PLLC_LOADCORE0_BITS                             0:0
  #define CM_PLLC_LOADCORE0_SET                              0x00000001
*/

void switch_vpu_to_pllc() {
	A2W_XOSC_CTRL |= A2W_PASSWORD | A2W_XOSC_CTRL_PLLCEN_SET;

	A2W_PLLC_FRAC = A2W_PASSWORD | 87380;
	A2W_PLLC_CTRL = A2W_PASSWORD | 52 | 0x1000;
	
	A2W_PLLC_ANA3 = A2W_PASSWORD | 0x100;
	A2W_PLLC_ANA2 = A2W_PASSWORD | 0x0;
	A2W_PLLC_ANA1 = A2W_PASSWORD | 0x144000;
	A2W_PLLC_ANA0 = A2W_PASSWORD | 0x0;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET;

	/* hold all */
	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET;

	A2W_PLLC_DIG3 = A2W_PASSWORD | 0x0;
	A2W_PLLC_DIG2 = A2W_PASSWORD | 0x400000;
	A2W_PLLC_DIG1 = A2W_PASSWORD | 0x5;
	A2W_PLLC_DIG0 = A2W_PASSWORD | 52 | 0x555000;

	A2W_PLLC_CTRL = A2W_PASSWORD | 52 | 0x1000 | A2W_PLLC_CTRL_PRSTN_SET;

	A2W_PLLC_DIG3 = A2W_PASSWORD | 0x42;
	A2W_PLLC_DIG2 = A2W_PASSWORD | 0x500401;
	A2W_PLLC_DIG1 = A2W_PASSWORD | 0x4005;
	A2W_PLLC_DIG0 = A2W_PASSWORD | 52 | 0x555000;

	A2W_PLLC_CORE0 = A2W_PASSWORD | 2;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET | CM_PLLC_LOADCORE0_SET;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET;

	CM_VPUCTL = CM_PASSWORD | CM_VPUCTL_FRAC_SET | CM_SRC_OSC | CM_VPUCTL_GATE_SET;
	CM_VPUDIV = CM_PASSWORD | (4 << 12);
	CM_VPUCTL = CM_PASSWORD | CM_SRC_PLLC_CORE0 | CM_VPUCTL_GATE_SET;
	CM_VPUCTL = CM_PASSWORD | CM_SRC_PLLC_CORE0 | CM_VPUCTL_GATE_SET | 0x10; /* ENAB */

	CM_TIMERDIV = CM_PASSWORD | (19 << 12) | 819;
	CM_TIMERCTL = CM_PASSWORD | CM_SRC_OSC | 0x10;
}

extern void sdram_init();
extern void arm_init();
extern void monitor_start();

void print_crap() {
	printf("TB_BOOT_OPT = 0x%X\n", TB_BOOT_OPT);
}

int _main(unsigned int cpuid, unsigned int load_address) {
	switch_vpu_to_pllc();

	led_init();
	uart_init();

        for(;;) {
	printf(
		"=========================================================\n"
		"::\n"
		":: kFW for bcm2708, Copyright 2016, Kristina Brooks. \n"
		"::\n"
		":: BUILDATE  : %s %s \n"
		":: BUILDSTYLE: %s \n"
		"::\n"
		"=========================================================\n",
		__DATE__, __TIME__,
		"OPENSOURCE"
	);

	printf("CPUID    = 0x%X\n", cpuid);
	printf("LoadAddr = 0x%X\n", load_address);

	print_crap();
        }

	g_CPUID = cpuid;

	/* bring up SDRAM */
	sdram_init();
	printf("SDRAM initialization completed successfully!\n");

	/* bring up ARM */
	arm_init();

	/* start vpu monitor */
	monitor_start();

	panic("main exiting!");
}

