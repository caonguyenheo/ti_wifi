How to start
----------------------------
To avoid sudo when accessing the /dev/ttyUSB*
```
sudo addgroup <your user name> dialout
sudo service udev restart
```

Clone the project, with submodule

For kernel >= 3.12 (Ubuntu 14.04), create (as root user) the file /etc/udev/rules.d/99-ti-launchpad.rules.

Add the following lines to this file:
```
# CC3200 LaunchPad
SUBSYSTEM=="usb", ATTRS{idVendor}=="0451", ATTRS{idProduct}=="c32a", MODE="0660", GROUP="dialout", 
RUN+="/sbin/modprobe ftdi-sio" RUN+="/bin/sh -c '/bin/echo 0451 c32a > /sys/bus/usb-serial/drivers/ftdi_sio/new_id'"
```
The board should have enumberated as /dev/ttyUSB{0,1}. We will use ttyUSB1 for UART backchannel

Install GNU Tools for ARM Embeded Processor: gcc-arm-none-eabi, gdb-arm-none-eabi

Install OpenOCD for onchip debugging: openocd (v0.7)

You may need to copy openocd rules, under `tools/openocd/40-openocd.rules` to `/lib/udev/rules.d`, then tell udev to reload its rules with `sudo udevadm control --reload-rules`. Why? Because the Linux service udev assigns access rights based on its rules when a device is plugged in, and these rules are restrictive by default, allowing access only to the administrator.

Run your first example
-------------------------
Play around with examples/blinky, compile with ```make```, bin file is generated at .exe/*project*.bin

More options:
- flashing ```make flash```
- debugging ```make debug```
- logging ```make login```

OTA
-------------------------
Application bootloader `/sys/mcuimg.bin` is found under $(SDK)/example/application_bootloader.

Flashing the `application_bootloader.bin` fist. This program will select the right image to boot. If you want to change the way CC3200 OTA works, look at this file.

Todo
-------------------------
[x] makefile to recompile sdk lib (*.a) when one doesn't exist yet.
[] use flashing facility of *uniflash*
[x] run ssl with CA verification
[] common files for freertos using, partly at common/common.h
[] ota made easy
[] battery measurement
[x] aws iot
[] aws iot do not run stable in long term. may need to check timer

Note
-------------------------
- tested with sdk 1.1.0
- cc3200prog is extracted from energia-0101E0016
- how to know which example is w/o OS, see http://processors.wiki.ti.com/index.php/CC32xx_SDK_Sample_Applications
- gdb tip: https://web.stanford.edu/class/cs107/gdb_refcard.pdf
- aws iot use lib from https://github.com/aws/aws-iot-device-sdk-embedded-C with an adaptation layer.

Setup (tested with Ubuntu 14.04)
-------------------------
**GCC**
wget https://launchpad.net/gcc-arm-embedded/4.8/4.8-2014-q1-update/+download/gcc-arm-none-eabi-4_8-2014q1-20140314-linux.tar.bz2
tar xjf gcc-arm-none-eabi-4_8-2014q1-20140314-linux.tar.bz2
sudo mv gcc-arm-none-eabi-4_8-2014q1 /usr/local/
export PATH=/usr/local/gcc-arm-none-eabi-4_8-2014q1/bin:$PATH

**OpenOCD**
wget http://downloads.sourceforge.net/project/openocd/openocd/0.7.0/openocd-0.7.0.tar.gz
tar xvfz openocd-0.7.0.tar.gz
cd openocd-0.7.0
./configure
make
sudo make install

Ftest command
-------------------------
    Command Usage
    ------------- 
    udid
    - get device's udid
    mac [MAC_ADDRESS]
    - get device's MAC address
    - set device's MAC address if provide [MAC_ADDRESS]
    version
    - get device's firmware version

