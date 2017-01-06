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

.text
.globl _start
_start:
	/* vectors */
	b L_all_cores_start
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	/* comm chan */
	nop
	nop
	nop
	nop

L_all_cores_start:
        /* check CPU id */
	mrc p15, 0, r0, c0, c0, 5	@ read MPIDR
	and r3, r0, #0xc0000000		@ multiprocessing extensions and
	teq r3, #0x80000000		@ not part of a uniprocessor system?
	bne L_core0			@ no, assume UP
	ands r0, r0, #0x03		@ CPU 0?
	bne L_deadloop			@ if not, spin.

L_core0:
	mov sp, #(MEM_STACK_END)

        /* we are loaded in secure supervisor mode -- drop permissions */
        mrc p15, 0, r0, c1, c1, 0
        orr r0, r0, #1
        mcr p15, 0, r0, c1, c1, 0

	b _firmware_rendezvous

L_deadloop:
	b L_deadloop
