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
Entry.

A small explanation. The ROM loads bootcode.bin at 0x80000000 and jumps to
0x80000200. This region corresponds to L1/L2 cached IO and cache is never
evicted as long as we don't touch memory above that. This gives us 128KB
of memory at startup.

Exception names are from the public release from:
	brcm_usrlib\dag\vmcsx\vcfw\rtos\none\rtos_none.c

=============================================================================*/


.text

empty_space:
	.space 0x200

.include "ghetto.s"

/* main entry point */

.globl _start
.align 2
_start:
        version r0
	mov r5, r0

	/* vectors */
	mov r3, #0x1B000
	mov r1, r3

	/*
	 * populate the exception vector table using PC relative labels
	 * so the code isnt position dependent
	 */
.macro RegExceptionHandler label, exception_number
	lea r2, fleh_\label
	st r2, (r1)
	add r1, #4
.endm


	RegExceptionHandler zero, #0
	RegExceptionHandler misaligned, #1
	RegExceptionHandler dividebyzero, #2
	RegExceptionHandler undefinedinstruction, #3
	RegExceptionHandler forbiddeninstruction, #4
	RegExceptionHandler illegalmemory, #5
	RegExceptionHandler buserror, #6
	RegExceptionHandler floatingpoint, #7
	RegExceptionHandler isp, #8
	RegExceptionHandler dummy, #9
	RegExceptionHandler icache, #10
	RegExceptionHandler veccore, #11
	RegExceptionHandler badl2alias, #12
	RegExceptionHandler breakpoint, #13
	RegExceptionHandler unknown, #14

	//add r1, r3, #252
        add r1, r3, #128
	lea r2, fleh_irq
	//mov r4, #492
        add r4, r3, #492

L_setup_hw_irq:
	st r2, (r1)
        add r1, #4
	ble r1, r4, L_setup_hw_irq

	/*
	 * load the interrupt and normal stack pointers. these
	 * are chosen to be near the top of the available cache memory
	 */

	mov r28, #0x1D000 
	mov sp, #0x1C000

	/* set interrupt vector bases */
	mov r0, #IC0_VADDR
	st r3, (r0)
	mov r0, #IC1_VADDR
	st r3, (r0)

        /* unmask ARM interrupts */
        mov r0, #(IC0_BASE + 0x10)
        mov r1, #(IC1_BASE + 0x10)
        mov r2, 0xFFFFFFFF
        mov r3, #(IC0_BASE + 0x10 + 0x20)

    unmask_all:
        st r2, (r0)
        st r2, (r1)
        add r0, 4
        add r1, 4
        ble r0, r3, unmask_all

        /* enable interrupts */
	ei

	/* jump to C code */
	mov r0, r5
	lea r1, _start

	bl _main

/************************************************************
 * Debug
 ************************************************************/

blinker:
	mov r1, #GPFSEL1
	ld r0, (r1)
	and r0, #(~(7<<18))
	or r0, #(1<<18)
	st r0, (r1)
	mov r1, #GPSET0
	mov r2, #GPCLR0
	mov r3, #(1<<16)
loop:
	st r3, (r1)
	mov r0, #0
delayloop1:
	add r0, #1
	cmp r0, #0x100000
	bne delayloop1
	st r3, (r2)
	mov r0, #0
delayloop2:
	add r0, #1
	cmp r0, #0x100000
	bne delayloop2
	b loop

/************************************************************
 * Exception Handling
 ************************************************************/

.macro SaveRegsLower 
        stm lr, (--sp)
	stm r0-r5, (--sp)
.endm

.macro SaveRegsUpper
	stm r6-r15, (--sp)
	stm r16-r23, (--sp)
.endm

.macro SaveRegsAll
	SaveRegsLower
	SaveRegsUpper
.endm

fatal_exception:
	SaveRegsUpper
	mov r0, sp
	b sleh_fatal

.macro ExceptionHandler label, exception_number
fleh_\label:
	SaveRegsLower
	mov r1, \exception_number
	b fatal_exception
.endm

	ExceptionHandler zero, #0
	ExceptionHandler misaligned, #1
	ExceptionHandler dividebyzero, #2
	ExceptionHandler undefinedinstruction, #3
	ExceptionHandler forbiddeninstruction, #4
	ExceptionHandler illegalmemory, #5
	ExceptionHandler buserror, #6
	ExceptionHandler floatingpoint, #7
	ExceptionHandler isp, #8
	ExceptionHandler dummy, #9
	ExceptionHandler icache, #10
	ExceptionHandler veccore, #11
	ExceptionHandler badl2alias, #12
	ExceptionHandler breakpoint, #13
	ExceptionHandler unknown, #14

fleh_irq:
	SaveRegsAll

	/* top of savearea */
	mov r0, sp
	bl sleh_irq

return_from_exception:
	ldm r16-r23, (sp++)
	ldm r6-r15, (sp++)
	ldm r0-r5, (sp++)
	ld lr, (sp++)
	rti
