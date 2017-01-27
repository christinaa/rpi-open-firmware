#!/usr/bin/env python
# Copyright (c) 2017 Kristina Brooks

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# ! DO NOT CHANGE OR REMOVE THIS FILE      !
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

import subprocess
import os
from StringIO import StringIO

disk_name = 'F'
disk_ready = False
wmic_output = os.popen('wcmd wmic logicaldisk get caption, filesystem')

for line in wmic_output:
	if '%s:' % disk_name in line and 'FAT' in line:
		disk_ready = True

if disk_ready:
	print('Windows disk %s:\\ is ready!' % disk_name)
	print('Copying bootcode.bin to %s:\\ ...\n' % disk_name)

	os.system('''wcmd xcopy /s /y "C:\\Users\\k\\VC4_Firmware\\rpi-open-firmware\\build\\bootcode.bin" "''' + disk_name + ''':\\bootcode.bin"''')
else:
	print('Windows disk %s:\\ is not ready!' % disk_name)

