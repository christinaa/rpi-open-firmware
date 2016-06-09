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
Crappy C++ runtime.

=============================================================================*/

#include <stdint.h>
#include <chainloader.h>
#include <hardware.h>

void* operator new[] (size_t sz) {
	return malloc(sz);
}

void* operator new (size_t sz) {
	return malloc(sz);
}

extern "C" void __cxa_pure_virtual() {
	panic("__cxa_pure_virtual!");
}