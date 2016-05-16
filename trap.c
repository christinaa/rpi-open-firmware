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
VideoCoreIV second level exception handlers.

=============================================================================*/

#include "lib/common.h"
#include "hardware.h"

/*
 * this file in the public release documents all exception names.
 * brcm_usrlib\dag\vmcsx\vcfw\rtos\none\rtos_none.c
 */

static const char* g_ExceptionNames[] = {
	"zero",
	"misaligned",
	"divide by zero",
	"undefined instruction",
	"forbidden instruction",
	"illegal memory",
	"bus error",
	"floating point",
	"isp",
	"dummy",
	"icache",
	"vec core",
	"bad l2 alias",
	"breakpoint"
};

static const char* exception_name(uint32_t n) {
	if (n >= (sizeof(g_ExceptionNames)/4))
		return "unknown";
	return g_ExceptionNames[n];
}

void sleh_fatal(uint32_t n, uint32_t pc) {
	panic("fatal processor exception: %s (%d) at 0x%0x", exception_name(n), n, pc);
}