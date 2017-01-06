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
ARM entry point.

This is where all cores start. For RPi1, only one core starts so we can jump
straight to the main bootloader. For later models, the first core jumps to the
bootloader. The other cores wait until SMP is enabled by the kernel later in
the boot process.

=============================================================================*/

#include "memory_map.h"

.arch_extension sec

.text
.globl _start
_start:
	/* vectors */
	b _common_start /* reset */
	nop /* undefined */
	b _secure_monitor /* swi/smc */
	nop /* prefetch abort */
	nop /* data abort */
	nop /* reserved */
	nop /* irq */
	nop /* fiq */

.globl g_FirmwareData
g_FirmwareData:
	.long 0 /* SDRAM capacity */
	.long 0 /* VPU CPUID */
	.long 0 /* Reserved */
	.long 0 /* Reserved */
	.long 0 /* Reserved */

_secure_monitor:
	mrc p15, 0, r0, c1, c1, 0
	bic	r0, r0, #0x4a /* clear IRQ, EA, nET */
	orr r0, r0, #1 /* set NS */
	mcr p15, 0, r0, c1, c1, 0

	mov r0, #((1 << 7) | (1 << 8) | (1 << 6)) /* mask IRQ, AA and FIQ */
	orr r0, r0, #0x1a /* switch to hypervisor mode */
	msr spsr_cxfs, r0 

	movs pc, lr

_common_start:
	/*
	 * read MIDR, see if this is an ARMv6 system, if it is, just
	 * assume single core (BCM2708) and not bother doing SMP stuff.
	 */
	mrc p15, 0, r0, c0, c0, 0
	lsr r0, #16
	and r0, #0xF
	cmp r0, #0x7
	mov r12, #0
	beq L_finish_init

L_armv7_or_higher:
	/*
	 * okay, we're an ARMv7 or an ARMv8.
	 */
	mrc p15, 0, r0, c0, c0, 5	// read MPIDR
	and r3, r0, #0xc0000000		// multiprocessing extensions and
	teq r3, #0x80000000			// not part of a uniprocessor system?
	bne L_setup_monitor		 	// no, assume UP
	ands r0, r0, #0x03			// CPU 0?
	bne L_deadloop				// if not, spin.

L_setup_monitor:
	adr	r1, _start
	mcr	p15, 0, r1, c12, c0, 1 /* MVBAR */
	mcr p15, 0, r1, c7, c5, 4 /* ISB (ARMv6 compatible way) */

	mov r12, #1
	smc 0
	
L_finish_init:
	/* enable instruction cache */
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #(1<<12)
	mcr p15, 0, r0, c1, c0, 0

	mov sp, #(MEM_STACK_END)
	mov r0, r12
	b main

L_deadloop:
	cpsie if
	wfi
	b L_deadloop
