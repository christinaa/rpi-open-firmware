# Minimal Raspberry Pi VPU firmware

This is a small firmware for RPi VPU (VideoCore4), capable of initializing UART, VPU PLL (PLLC) and ARM itself. It's intended as a libre replacement of the stock `bootcode.bin` on RPi's SD card. You **need** to setup UART to see anything meaningful at the moment. The project additionally maintains a second-stage chainloader running on ARM capable of initializing eMMC, FAT, and the Linux kernel, as well as a shrinking set of patches to the Linux kernel.

Linux is confirmed booting on the RPi 2 Model B and RPi 3 Model B. It should work with minimal modifications on all of the models. 

If you want to contact us because you're interested in contributing, development discussion occurs in `#raspberrypi-internals` on [Freenode](http://webchat.freenode.net/?channels=#raspberrypi-internals). Please come on IRC before randomly hacking on the codebase :-)

All Broadcom headers are licensed under 3-Clause BSD License while our code is under GPLv2+. See `LICENSE` for more information. Various external projects are mirrored under a mix of GPL-compatible licenses.

## Building

As a prerequisite, Julian Brown's [VC4 toolchain](https://github.com/puppeh/vc4-toolchain) is necessary as well as a `arm-none-eabi-` toolchain (Debian package `gcc-arm-none-eabi`). You can tweak the VC4 toolchain path in `CROSS_COMPILE` in `Makefile` and the ARM path in `arm_chainloader/Makefile` if necessary. Contributors should not commit their personal paths. Contributors should also use tabs for indentation. After you've configured the build, run `buildall.sh` and you should have a blob in `build/bootcode.bin`, ready to be copied to an SD card.

### Building on macOS

macOS compilation is similar to GNU/Linux. Instructions for building the ARM toolchain can be found [here](https://launchpadlibrarian.net/287100910/How-to-build-toolchain.pdf). GCC must be installed manually, as Apple's default compiler is LLVM-based. Additionally, an older version of guile is necessary (homebrew packages `gcc-6` and `guile18`, respectively). Finally, set the environment variable `LIBRARY_PATH` to `/lib:/lib64` when running `buildall.sh` to workaround a platform bug.

## Technical Details
The firmware is split into two parts, one running on the VC4 and the other on ARM. The VC4 part initializes PLLC and moves VPU over to it, and then brings up UART. It then performs SDRAM initialization, making SDRAM available at `0xC0000000` (uncached alias). The ARM loader will do ARM initialization and then copy the ARM bootloader that's embedded in it to the alias. It will then map it to `0x0` in ARM's memory space and start ARM. The code under `arm_chainloader` is what will run on the ARM. 

The ARM chainloader initializes the eMMC controller and accesses the boot partition with a small FAT driver. From here, it chainloads the Linux kernel (other payloads are not tested and are not likely to work due to dependence on the firmware).

## Does it boot Linux?

Yes, with some conditions. You can boot a very minimal version of Linux without the firmware and get it to work with UART and eMMC. Support for USB, DMA, and Ethernet are in the works, which will be sufficient for certain headless systems. Still, many other peripherals require bringup, such as video. Additionally, drivers for clock and power management need to written.

## Thanks To
 * **[Herman Hermitage](https://github.com/hermanhermitage)** for his VC4 documentation and for helping determine suitable ARM PLL configurations.
 * **[Julian Brown](https://github.com/puppeh)** for reviewing the code and for his awesome VC4 toolchain.
 * **[Alyssa Rosenzweig](https://github.com/bobbybee)** for her contributions to the firmware especially in areas of Linux bringup and early ARM side initialization, as well as fixing mailbox support.
 * **[David Given](https://github.com/davidgiven)** for his initial LLVM project used as the base for the initial LLVM toolchain before moving to GCC.
 * **[Sccott Mansell](https://github.com/phire)** for reviewing the code.
 * **[Broadcom](https://github.com/broadcom)** for their header release.
 * Various other people not mentioned here.
