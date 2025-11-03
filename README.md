![status](https://img.shields.io/badge/Status-boots-yellow.svg)

## Kernel-3.10.14 development for Amazfit Verge
### Status
The kernel boots, both framebuffer and usb work (with some hacks).
Currently, only a minimal busybox enviroment is tested to work, stock ramdisk has issues booting.


### Compiling the kernel

Currently only gcc 4.8 is tested to work fine, but since such compiler is obsolete, you'll need to get it from 
https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/mips/mipsel-linux-android-4.8/

```bash
# Defince cross compiler and architecture
export CROSS_COMPILE=/your/compiler/path/bin/mipsel-linux-android-
export ARCH=mips && export SUBARCH=mips
# Copy the configuration file we created for Pace
make qogir_defconfig
# Configure the kernel (optional)
make menuconfig
# Build Kernel
make zImage
```

The zImage will be located at `./arch/mips/boot/compressed/zImage`.


WARNING: I AM NOT RESPONSIBLE IF YOU BRICK YOUR WATCH, START WW3 OR FORGET TO FEED YOUR CAT.
These watches need sacrificial rituals in order to boot them into fastboot when they are bricked. 
Simple precaution: Use the recovery partition or use `fastboot boot boot.img` DO NOT TOUCH THE BOOT PARTITION UNLESS YOU CAN REBOOT TO BOOTLOADER.