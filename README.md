# SeCloak: ARM Trustzone-based Mobile Peripheral Control

## Overview
This is research prototype for the secure enforcement kernel used
in the evaluation of our SeCloak system, as presented in our paper:

SeCloak: ARM Trustzone-based Mobile Peripheral Control
Matthew Lentz, Rijurehkha Sen, Peter Druschel, Bobby Bhattacharjee
MobiSys 2018 (International Conference on Mobile Systems, Applications, and Services)

SeCloak provides users with secure, "virtual" switches to control peripherals
on their smart devices by providing a small, OS-agnostic enforcement layer that
mediates untrusted accesses to devices.

Our prototype is based on OP-TEE, which is a open-source operating system
for running trusted application using ARM TrustZone hardware security
extensions. We heavily modified and reduced the codebase for our purposes.
Specifically, we retained OP-TEE's kernel threading and debugging support,
and the MMU code is also based on OP-TEE. The device drivers required for
SeCloak (e.g., framebuffer and GPIO keypad), device tree parsing,
instruction interception and emulation, and the code for securing device
state was developed specifically for SeCloak. You learn more about OP-TEE,
please see the [OPTEE_README.md](OPTEE_README.md) file.

---

## Target Platform
We target the Boundary Devices Nitrogen6Q development board. While some
parts of SeCloak are platform-agnostic (e.g., DT parsing, emulation), others
such as the drivers were written specifically for this target platform.

You can find more information about the Nitrogen6Q board
[here](https://boundarydevices.com/product/nitrogen6x/).

---

## Setup

1. Update the UBoot version on the board. You can find the instructions
   [here](https://boundarydevices.com/u-boot-v2017-03/)

2. Assuming you are going to use Android/Linux in the non-secure world, you
   should follow the the instructions for setting up Android
[here](https://boundarydevices.com/android-nougat-7-1-1-release-imx6-boards/),
and then for building a custom Linux kernel
[here](https://boundarydevices.com/imx-linux-kernel-4-1-15/).

When building the custom Linux kernel, please checkout commit '1d9fc5c0d7'
to serve as the base commit. Afterwards, apply the patch
'0001-Modifications-to-run-Linux-in-the-non-secure-world-a.patch' included
in the 'support' directory in this repository to enable Linux to operate in
the non-secure world and expose interfaces for invoking SeCloak
functionality via SMC instructions.

To build the Linux kernel and device tree blobs, you can run:
> . setup_env.sh
> make boundary_android_secpath_defconfig
> make

This will generate the image for the kernel (as 'arch/arm/boot/zImage') and
the device tree blob (as 'arch/arm/boot/dts/imx6q-nitrogen6x.dtb'). You will
then place these files on the boot partition, replacing the existing files.

3. To build the SeCloak kernel, you can run:
> . setup_nitrogen6x.sh
> make

This will generate a loadable image for the secure kernel (as
'out/arm-plat-imx/core/sImage'), which you will place on the boot partition
as well.

4. Configure a bootloader script to boot into the secure kernel. I included
   my script in the 'support/uboot' directory in this repository, with a
Makefile to build the .scr file. Place the resulting sp_bootscript.scr file
on the boot partition. In order to use the script, you should add the
following UBoot environment variables:

> setenv extbootargs 'mem=952m'
> setenv loader 'load mmc 0:1'
> setenv sp_boot 'setenv bootdev mmcblk0; ${loader} 0x10008000 sp_bootscript.scr && source 10008000'
> savee

and then subsequently run the script via:

> run sp_boot

5. (Optional) I recommend using 'tftpboot' as the loader, which fetches the
   images to load over the network, instead of relying on updating the files
on the MMC card.

