# Minimal Raspberry Pi VPU firmware
This is a small firmware for RPi VPU (VideoCore4) versions 1/2/3 that is capable of initializing UART, VPU PLL (PLLC) and ARM itself. It's intended to be used instead of stock `bootcode.bin` on RPi's SD card. You **need** to have UART to see anything meaningful as far as output goes.

This has been tested on RPi1 Model B (Hynix PoP DDR), RPi 2 Model B and RPi 3 Model B (both Elpida DDR), but it should work with minimal modifications on all of the models. Development primarily targets the RPi 2 Model B.

If you want to contact us because you're interested in contributing, development discussion occurs in `#raspberrypi-internals` on Freenode. Please come on IRC before randomly hacking on the codebase :-)

All Broadcom headers are licensed under 3-Clause BSD License while our code is under GPLv2+. See `LICENSE` for more information.

## Building

You need Julian Brown's VC4 toolchain to build this (https://github.com/puppeh/vc4-toolchain) as well as a arm-none-eabi-toolchain. You can tweak the paths to it in CROSS_COMPILE in `Makefile` (for VC4) and for ARM in `arm_chainloader/Makefile`, although by default it assumes they are currently in your path. Contributors should not commit their personal paths. Contributors should also use tabs for indentation. After you've done it, run `buildall.sh` and you should have a blob in `build/bootcode.bin`. 

### Building on OSX

#### Dependencies:

+ vc4-toolchain (https://github.com/puppeh/vc4-toolchain)

+ arm-none-abi-toolchain (https://launchpad.net/gcc-arm-embedded/+download)
    + compilation instructions for OSX in the [pdf](https://launchpadlibrarian.net/287100910/How-to-build-toolchain.pdf)

#### Build instructions: 
Compiling for osx is basically the same; you just have to be careful of OSX's built-in gcc (it's actually llvm) and the default version of guile (2.x and totally not compatible with older syntax). 

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

The ARM chainloader then initializes the eMMC controller and access the boot partition with a FAT driver. From here, it chainloads the Linux kernel (other payloads are not tested and are not likely to work due to dependence on the firmware).

The current makefiles in the ARM part of it aim at **RPi1** (ie. ARMv6) but they can be changed to ARMv7 if you want to build it for a newer model. I tested it on all RPi models and it works without any issues as far as I can tell (ARM can access peripherals and memory just fine as AXI supervisor). However it cannot access any secure peripherals (OTP/SDRAM/etc) since they appear to be on a separate bus accessible only to VC4.

## Does it boot Linux?

Yes, with some conditions. You can boot a very minimal version of Linux without the firmware and get it to work with UART. Support for eMMC, some USB devices, and Ethernet are in the works, which will be sufficient for certain headless systems. Still, you can expect half of the things to be broken (most importantly, video and DMA). Additionally, since `start.elf` is responsible for clock and power management in the original firmware (all registers in the `cpr` block), these drivers will have to be rewritten on ARM or our VC4 firmware to have most of the peripherals working properly (HDMI, for example).

## Thanks To
 * **[Herman Hermitage](https://github.com/hermanhermitage)** for his VC4 documentation and for helping with working out suitable ARM PLL configurations.
 * **[Julian Brown](https://github.com/puppeh)** for reviewing the code and for his awesome VC4 toolchain.
 * **[Alyssa Rosenzweig](https://github.com/bobbybee)** for her contributions to the firmware especially in areas of Linux bringup and early ARM side initialization, as well as fixing mailbox support.
 * **[David Given](https://github.com/davidgiven)** for his initial LLVM project which I used as the base for my LLVM toolchain before moving to GCC.
 * **[phire](https://github.com/phire)** for reviewing my code.
 * **[Broadcom](https://github.com/broadcom)** for their header release.
 * Various other people not mentioned here.
