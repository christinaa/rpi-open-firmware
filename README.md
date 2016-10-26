# Minimal Raspberry Pi VPU firmware
This is a small firmware for RPi VPU (VideoCore4) versions 1/2/3 that is capable of initializing UART, VPU PLL (PLLC) and ARM itself. It's intended to be used instead of stock `bootcode.bin` on RPi's SD card. You **need** to have UART to see anything meaningful as far as output goes.

This has been tested on RPi1 Model B (Hynix PoP DDR), RPi 2 Model B and RPi 3 Model B (both Elpida DDR). 

If you want to contact me because you're interested in contributing, you can message `kristina` on Freenode, but I would suggest talking in `#raspberrypi-internals` instead.

All Broadcom headers are licensed under 3-Clause BSD License while the rest of this is under GPLv2+. See `LICENSE` for more information.

## Building

You need Julian Brown's VC4 toolchain to build this (https://github.com/puppeh/vc4-toolchain) as well as a arm-none-eabi-toolchain. You can tweak the paths to it in CROSS_COMPILE in `Makefile` (for VC4) and for ARM in `arm_chainloader/Makefile`, although by default it assumes they are currently in your path. Contributors should not commit their personal paths. Contributors should also use tabs for indentation. After you've done it, run `buildall.sh` and you should have a blob in `build/bootcode.bin`. 

### Building on OSX

#### Dependencies:

+ vc4-toolchain (https://github.com/puppeh/vc4-toolchain)

+ arm-none-abi-toolchain (https://launchpad.net/gcc-arm-embedded/+download)
    + compilation instructions for OSX in the [pdf](https://launchpadlibrarian.net/287100910/How-to-build-toolchain.pdf)

#### Build instructions: 
Compiling for osx is basically the same; you just have to be careful of OSX's built-in gcc (it's actually lldb) and the default version of guile (2.x and totally not compatible with older syntax). 

install dependencies: 

    brew install gcc-6 guile18

install the arm toolchain.

    # follow the instructions in the pdf linked above

Download the source and dependencies together for vc4-toolchain:

    git clone --recursive https://github.com/puppeh/vc4-toolchain.git

Once it’s done, you’ll need to add file paths to your $PATH variable: 

    export PATH=$PATH:/path/to/gcc-arm-none-eabi-5_4-2016q3/bin:/path/to/vc4-toolchain/prefix/bin

From the main instructions ("After you've done it, run buildall.sh and you should have a blob in build/bootcode.bin"):

    git clone https://github.com/christinaa/rpi-open-firmware; cd rpi-open-firmware
    CC=gcc-6 LIBRARY_PATH=/lib:/lib64 ./build-all.sh

Note: on the last step, i set the library path here because mine initially had a trailing “:” and something in the build chain didn’t like that. CC=gcc-6 is just to ensure it's using the gcc install from the first step.

That's it! Your shiny new binary is sitting at rpi-open-firmware/build/bootcode.bin.

## Technical Details
The firmware is split into two parts, a VC4 part and and ARM part. The VC4 part initializes PLLC and moves VPU over to it, and then brings up UART. It then performs SDRAM initialization, making SDRAM available at `0xC0000000` (uncached alias). The ARM loader will do ARM initialization and then copy the ARM bootloader that's embedded in it to the alias. It will then map it to `0x0` in ARM's memory space and start ARM. The code under `arm_chainloader` is what will run on the ARM. 

The current iteration of ARM chainloader (which I've still yet to commit) can actually initialize the eMMC controller and access the boot partition with a FAT driver. From here, it would be trivial to chainloaded the next stage bootloader *but* you still face the issues of missing a substantial portion of the drivers that were previously provided by the firmware.

The current makefiles in the ARM part of it aim at **RPi1** (ie. ARMv6) but they can be changed to ARMv7 if you want to build it for a newer model. I tested it on all RPi models and it works without any issues as far as I can tell (ARM can access peripherals and memory just fine as AXI supervisor). However it cannot access any secure peripherals (OTP/SDRAM/etc) since they appear to be on a separate bus accessible only to VC4.

## Issues/Fixes
 * PLL rate on ARM is slow, it's a bit annoying. Need to tweak the PLL rate later.
 * All SDRAM configurations (including 1GB work), however, I'm not certain whether they will be reliable or not . ARM detection works for all of the RPi models I got to test it on which was two RPi2s, 1 RPi1 and 1 RPi3.
 * SD card support kind of works now, can parse MBR, parse the FAT boot partition but it still needs to pull off a next stage bootloader of it (possibly u-boot/LittleKernel or maybe even just start Linux straight from bootcode.bin).
 * The VPU still needs some sort of a firmware since ARM can't access all registers by itself, at the moment I'm trying to figure out how the VC4 interrupt controller works and how I can wake the sleeping VPU from ARM and get it to do things via a mailbox.

## Does/Will it boot Linux?

Eventually maybe. Since `start.elf` is responsible for clock and power management (all registers in the `cpr` block), these drivers will have to be rewritten on ARM (or even on the open source VC4) to have any meaningful peripherals working properly (for example HDMI/DMA/Pixel Valve/Whatever). You can boot a very minimal version of Linux without the firmware and get it to work with UART and some USB devices, but you can expect half of the things to be broken (most importantly, video and DMA).

## Thanks To
 * **Herman Hermitage** for his VC4 documentation and for helping me with working out suitable ARM PLL configurations.
 * **Julian Brown** for reviewing my code and for his awesome VC4 toolchain.
 * **David Given** for his initial LLVM project which I used as the base for my LLVM toolchain before moving to GCC.
 * **phire** for reviewing my code.
 * **Broadcom** for their header release.
 * **alyssa** for her contributions to the firmware especially in areas of Linux bringup and early ARM side initialization, as well as fixing mailbox support.
 * Various other people not mentioned here.
