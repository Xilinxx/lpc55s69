# Zeus300S/Gaia300D Green Power MCU

- [Zeus300S/Gaia300D Green Power MCU](#zeus300sgaia300d-green-power-mcu)
  - [Intro](#intro)
  - [Processor details](#processor-details)
  - [Project Structure](#project-structure)
  - [Build and Debug](#build-and-debug)
    - [Docker](#docker)
    - [Checkout](#checkout)
    - [Change branch](#change-branch)
    - [Containers](#containers)
    - [Setup and make for embedded platform](#setup-and-make-for-embedded-platform)
    - [Run interactively](#run-interactively)
    - [Local setup](#local-setup)
      - [Supported targets](#supported-targets)
      - [Building](#building)
    - [Jenkins build server](#jenkins-build-server)
    - [Docker info](#docker-info)
    - [Debugging](#debugging)
      - [Flashing LPC55S69XPresso with JLink Firmware](#flashing-lpc55s69xpresso-with-jlink-firmware)
      - [Running JLinkGDBServer](#running-jlinkgdbserver)
  - [Testing](#testing)

## Intro

The Green Power MCU's main roles in the Zeus300S/Gaia300D Project:
1. Correct Power sequencing for the Main processor at board startup via the Gowin FPGA.
2. Foresee Power Modes
   * Low Power mode
     * Provide a very low power standby mode when the display is not active. Less than **0.5 Watt**.
     * Wake up depending on I/O Pins see [detail wiki page](https://wiki.barco.com/display/p900/Platform+300+-+Green+Power+Processor+pre-study)
   * Semi Low Power mode
     * Same as Low Power mode
     * Monitor USB5807C for bus-status
   * Full Power Mode
3. UART Communication
   * towards and control of the GOWIN FPGA which is responsible for the DisplayPort AUX and EDID processing + GPIO
   * towards the main Zynq processor
4. Gowin SPI Flash bitstream update

Extra details regarding:
- The bootloader can be found in [src/bootloader/README.md](src/bootloader/README.md)
- The Application can be found in [src/application/README.md](src/application/README.md)
- Device drivers can be found in [src/drivers/devices/README.md](src/drivers/devices/README.md)
- A TODO list can be found in [TODO.md](TODO.md)

Hardware Interconnections can be found on this [wiki](https://wiki.barco.com/display/p900/Platform+300-Linux+Green+Power+Controller) page.

## Processor details

- NXP LPC55S69
  - Cortex M33 cores
  - Dual core
  - 640kB Flash (of which only 608KB is actually usable)
  	- Split in 3 regions (Crypto regions)
	  - 2 x 256kB
	  - 1 x 128kB
	- Regions are / can be cryptographically independent (Different keys)
  - 320kB SRAM split over various regions
  	- 32kB on the Code Bus
	- 272kB on the System Bus (contiguous)
	- 16kB USB RAM on the Systembus
  - FlexCOMM interfaces (USART / SPI / I2C / I2S) of which we use
  	- 3 UARTS (FlexCOMM0, FlexCOMM1 and FlexCOMM6)
	- 2 I2C (FlexCOMM2 and FlexCOMM4)
  - 100 Pin Package (HLQFP100)
  - Upto 64 IO's (Which are of course shared with the Peripherals such as FlexCOMM's, SWD,...)
  - SWD (Serial Wire Debug interface)

## Project Structure

| Directory | What's in here?|
|---|---|
| application | *Main application source directory for the Green Power MCU* |
| boards | *Board files for the different platforms* |
| bootloader | *Bootloader source directory* |
| CMSIS | *ARM CMSIS Abstraction layer* |
| device | *CPU Specific files (LPC5Sxx)* |
| docs | *Output directory for Docs + Some drawings for the readme* |
| Doxyfile | *Config file for Doxygen* |
| drivers | *NXP Driver files* |
| include | *Main include directory for GPMCU Project* |
| libs | *NXP Binary only libraries. Libraries for power-management* |
| README.md | *The file you're currently reading.. Readme-ception..* |
| scripts | *Some scripts to speed-up certain tasks, usually very quick and dirty* |
| startup | *Startup files for the board, Reset Vector, Vector Table are here* |
| tests | *Sources for unit testing and  component testing* |
| third-party | *Software which is included from other companies / vendors / ...* |
| .Jenkinsfile | jenkins build job |

___
## Build and Debug

### Docker
Sometimes docker can given an authentication error if the image is not yet on the system then you have to run docker login docker.barco.com and login with ldap account.

### Checkout
```sh
git clone --recurse-submodules ssh://git@git.barco.com:7999/hdis/p300-gpmcu.git -b <branch>
```
### Change branch
```sh
git checkout <branch> && git submodule update --init
```
### Containers
The images for the build containers should be located in the docker registry so nothing needs to be done here.

Manually build the containers (arm-none-eabi-gcc-sdk) see [here](https://git.barco.com/projects/HDIS/repos/docker-infrastructure/browse/README.md)

### Setup and make for embedded platform
```sh
./s300-scripts/build.sh -t arm -b zeus300s
or
./s300-scripts/build.sh -t arm -b gaia300d
```
### Run interactively
It is possible to run the CMake and make commands manually in the build directory of the arm build container
```sh
./s300-scripts/build.sh -t arm -b zeus300s -i
```
Inside docker you start with creating your Makefile with
```sh
cmake /home/dev/app -DCMAKE_TOOLCHAIN_FILE=/home/dev/app/cmake/toolchain/arm-none-eabi.cmake -DBOARD_NAME="zeus300s"

make
```
Once the build process finishes successfully, the build directory <i>build-arm or build-buildroot></i> will get populated with
```txt
CMakeCache.txt                  app-flash0.elf  boards                          compile_commands.json  tests
CMakeFiles                      app-flash0.map  bootloader.elf                  flash0.ld              third-party
CTestTestfile.cmake             app-flash1.bin  bootloader.map                  flash1.ld              vendor
Makefile                        app-flash1.elf  bootloader.ld                   memmap.ld
app-flash0.bin                  app-flash1.map  cmake_install.cmake             src
```
___
### Local setup

Project is build using CMake. It expects an arm-none-eabi toolchain to be installed on the system, if you don't use docker container.
Building is incredibly difficult. One must follow the steps below very carefully...

#### Supported targets
Currently, one can select a target board with a statically defined variable in the main `CMakeLists.txt` file. This then selects the correct directory from the `boards` directory. Currently supported boards are:

| Target name | Description | Remark |
|---|---|---|
| lpcxpresso55s69 | *Main [LPCXpresso55S69 Development kit](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/lpc5500-cortex-m33/lpcxpresso55s69-development-board:LPC55S69-EVK).* | does not compile (to be removed)
| zeus300s | *S300 Zeus Surgical display* |
| gaia300d | *D300 Gaia Diagnostic display* |

#### Building
```sh
# Standard build
cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchain/arm-none-eabi.cmake -DBOARD_NAME="<BOARDNAME>" CMakeLists.txt

# Semihosting enabled
cmake -DSEMIHOSTING=true -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchain/arm-none-eabi.cmake -DBOARD_NAME="<BOARDNAME>" CMakeLists.txt

# Build
make
```

This should result in three .elf files which can be flashed to the target.
The files are located in the bin-directory of the build directory `build-arm/bin`

| File | Description |
|---|---|
| bootloader.elf | *Bootloader application* |
| app-flash0.elf | *Main application* |
| app-flash1.elf | *Main application* |

* the `flash_tool` requires .bin files which are precompiled.

___
### Jenkins build server
Jenkins build is taken care of inside a docker container, login interactively on KORBLD11-server/KORBLD18-server via command
```
docker exec -it jenkins-barco-master /bin/bash
```
The Build job can be found in
```
 /var/lib/jenkins/jobs/p300-gpmcu
```
* [Jenkins p300-gpmcu](http://p300-jenkins.barco.com:8080/job/p300-gpmcu/)

___
### Docker info
The container in use is defined in `arm-none-eabi-gcc-sdk` , the Dockerfile [here](https://git.barco.com/projects/HDIS/repos/docker-infrastructure/browse/arm-none-eabi-gcc-sdk)

___

### Debugging
#### Flashing LPC55S69XPresso with JLink Firmware

First download LPCScrypt. This is NXP's firmware upgrade tool used to upgrade the MCU which is used as a debugger. You can find it [here](https://www.nxp.com/design/microcontrollers-developer-resources/lpc-microcontroller-utilities/lpcscrypt-v2-1-1:LPCSCRYPT?&tid=vanLPCSCRYPT).

Next, download the JLink firmware for the LPC platform. which can be found here: [here](https://www.segger.com/downloads/jlink/Firmware_JLink_LPCXpressoV2).

Once you've got the files downloaded and installed, go to the LPCScrypt install dir and remove the old JLink files. They should be located in:
`/usr/local/lpcscrypt-<version>/probe_firmware/`. Place the new files from JLink in the same directories.

**Note:** this only works on Debian/Ubuntu. On Arch Linux you can use `yay` to install lpcscrypt and on other systems, you can use compile the `unmakeself` packages from Gentoo, use that to extract the `.deb.bin` and then `bsdtar` the resulting `.deb` package. Once the `deb` package is extracted, you should find a `data.tar.gz` which you can once again extract. This should result in a `lib` and `usr` dir which you can just copy. [pkgbuild arch](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=lpcscrypt) can serve as a reference how to install the package on unsupported systems.

Once that's done
- place a jumper on the J4 header (located close to the debug USB connector).
- Connect the board
- In dmesg, you should now see an NXP device (Product: LPC, Manufacturer: NXP, SerialNumber: ABCD).
- Now run `<lpcscrypt_INSTALDIR>/scripts/boot_lpcscrypt`
- Wait a few seconds for the USB device to correctly enumerate
- run `<lpcscrypt_INSTALDIR>/scripts/program_JLINK`

**Note: This process seems very hit or miss... It took multiple attempts before it correctly flashed the firmware so do not be alarmed if it does not succeed at first**

To revert to the original CMSIS-DAP debugger follow the same steps but instead of `program_JLINK` run `program_CMSIS`.

#### Running JLinkGDBServer

Starting the debug server can be done through the following script: `scripts/run_jlink.sh`. This will run the JLinkGDBServer in a new terminal

Once in GDB one can load the elf file (for the symbols), connect to the debug server and flash the file to RAM using the commands below.
```sh
file <name_of_file_here>.elf
target ext:2331
load <name_of_file_here>.elf
```

Or just run arm-none-eabi-gdb for the source directory. Normally gdb will automagically load the `.gdbinit` script. Small note, probably GDB will complain about auto-load paths. You can fix this by adding the following to `~/.gdbinit`.

```sh
add-auto-load-safe-path PATH_TO_FOLDER_HERE/.gdbinit
```
This should shut up GDB about loading unsafe paths.

After that, it's GDB as we know and love... (or hate..)

Segger JLink does implement it's GDB commands a bit differently than most. Command references can be found here: [Segger JLink GDB](https://wiki.segger.com/J-Link_GDB_Server)

<del>
> Note: The JLinkGDBServer seems to have some trouble resetting the security settings between debug runs. A work around for this is to run `mon reset` in the GDB command window and then restart the JLinkGDBServer server and run `source .gdbinit` again in the GDB Command window.
</del>

## Testing

All matters related to testing should be located in the `tests` folder. Currently, the `test` directory is split into two folders. `target` and `native`, the names are pretty self-explanatory.
`target` contains test programs which run on the MCU. These are mainly PoC applications or applications used to facilitate testing. `native` contains all code used to validate business logic on a test machine.

The testing is performed with the help of the CMocka framework.
```sh
./s300-scripts/test.sh -t cmocka
```
