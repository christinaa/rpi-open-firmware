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
Mailbox driver.

=============================================================================*/

#pragma once

#include <chainloader.h>

struct Bcm2708Mailbox {
	Bcm2708Mailbox();
	bool write_word(uint32_t data, int timeout = 10000);
};

extern Bcm2708Mailbox g_Mailbox;