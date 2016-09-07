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
Second stage bootloader.

=============================================================================*/

#include <drivers/fatfs/ff.h>
#include <chainloader.h>
#include <drivers/mailbox.hpp>

#define logf(fmt, ...) printf("[LDR:%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

FATFS g_BootVolumeFs;

#define ROOT_VOLUME_PREFIX "0:"

extern "C" {
    void bootLinux(int zero, int machineID, void* dtb, void* kernel);
}

struct LoaderImpl {
	inline bool file_exists(const char* path) {
		return f_stat(path, NULL) == FR_OK;
	}

	bool read_file(const char* path, uint8_t** dest) {
            /* ensure file exists first */
            if(!file_exists(path)) return false;

            /* read entire file into buffer */
            FIL fp; 
            f_open(&fp, path, FA_READ);

            unsigned int len = f_size(&fp);
            uint8_t* buffer = (uint8_t*) malloc(len);

            printf("Buffer (%X) len %d\n", (unsigned int) buffer, len);

            f_read(&fp, buffer, len, &len);
            f_close(&fp);

            *dest = buffer;

            return true;
	}

	LoaderImpl() {
		logf("Mounting boot partitiion ...\n");
		FRESULT r = f_mount(&g_BootVolumeFs, ROOT_VOLUME_PREFIX, 1);
		if (r != FR_OK) {
			panic("failed to mount boot partition, error: %d", (int)r);
		}
		logf("Boot partition mounted!\n");

                /* dump cmdline.txt for test */
                uint8_t* arguments;

                if(!read_file("cmdline.txt", &arguments)) {
                    panic("Error reading cmdline arguments");
                }

                printf("\n%s\n", arguments);

                free(arguments);

                /* read device tree blob */
                uint8_t* dtb;

                if(!read_file("rpi.dtb", &dtb)) {
                    panic("Error reading device tree blob");
                }

                printf("DTB loaded at %X\n", (unsigned int) dtb);

                /* read the kernel */
                uint8_t* zImage;

                if(!read_file("zImage", &zImage)) {
                    panic("Error reading zImage");
                }

                printf("bzImage loaded at %X\n", (unsigned int) zImage);

                printf("Jumping to the Linux kernel...\n");
                
                /* this should never return */
                bootLinux(0, 3139, dtb, zImage);
	}
};

static LoaderImpl STATIC_APP g_Loader {};
