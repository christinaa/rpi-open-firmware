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
Chainloader inclusion.

=============================================================================*/

.text
.align 2
.globl L_arm_code_start
L_arm_code_start:

	.incbin "arm_chainloader/build/arm_chainloader.bin"

.globl L_arm_code_end
L_arm_code_end: