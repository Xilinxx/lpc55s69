# Zeus 300S tools

- [Intro](#intro)
- [Usage](#usage)
- [Update procedure](#update)

## Intro

Since the bootloader is a custom design in order to fullfil the specific needs of the project. Dedicated tools are made to support the usage of the bootloader. Such as the tool used to flash the application binaries. This tool is very purposely called `flash_tool`.  
The communication is mainly based for interaction with the bootloader-code of the microcontroller. One excepetion: a specific command for sending application to bootcode "`gp2boot`".

<div id='usage'/>

## Usage

First and foremost `flash_tool -h` will tell you everything you need to know. But just for completion. Below a small user guide

In order to correctly communicate with the device it needs to know what physical interface it needs to use. Currently supported are 'ip' and 'serial'. These options are passed via the `-d` option.

```bash
flash_tool -d ip
# Or
flash_tool -d serial     (default if none provided)
```

Now, this is not enough, interfaces need configuration parameters so `-p` is used to specify connection parameters. For `serial` this is `filepath:baudrate` and for `ip` it `IPAddress:Port`.

```bash
flash_tool -d ip -p "127.0.0.1:6000"
# Or
flash_tool -d serial -p "/dev/ttyUSB0:115200"
# On target Zeus
flash_tool -d serial -p "/dev/ttyPS1:230400"    (default if none provided)
```

Now the tool can actually do something. This is either via:

`-c` to manually send a command. (Available commands: "boot", "info", "swap", "reset", "wdog", "powoff", "wdog", "wipespi0", "wipespi1", "infospi")

````txt
  - version: git version info
	- boot: Force an application boot
	- force: Retrieve bootloader context (Partition info)
	- swap: Swap partition to be written to
	- reset: Reset bootloader
	- powon: Wake up mainCpu (removed, maincpu is sleeping!)
	- powoff: put mainCpu in sleep
	- wdog: Trigger the watchdog
	- info: Reports flash partition info
	- infospi: Reports the spi partition context
	- wipespi0/1: Erases spi0/1 flash partition
  - setspi0/1: Select spi0/1 flash partition for readback
  - reconfigure: Toggle the reconfigure line of the gowin
	- gp2boot: Sends the application code to bootcode (Application code command)
`-f` to send an application bin file
`-z` special case, this triggers a debug handler in the COMMP Protocol. This is useful to manually trigger a given action which is not a bootloader command. (For example, force a wipe of the flash during debugging).
`-t` force calculation of crc only, argument is -f `app-flash0.bin`
`-g <0,1>` Indicates binary firmware file is for Gowin(SPI flash) partition write nr
`-s` generic i2c command for spi-flash action  (Application code command)
`-r <file>` readback to file (requires -b)
`-b <size>` readback size in bytes from rom content

```bash
flash_tool -d ip -p "127.0.0.1:6000" -c "bootinfo"
flash_tool -d ip -p "127.0.0.1:6000" -f <filename.bin>
flash_tool -d ip -p "127.0.0.1:6000" -z
````

---
<div id='update'/>

## Update procedure

The gpmcu bootloader will check for **two** valid partitions before booting into application code.
So we need to execute following commands on a new board,

```bash
flash_tool -d "serial" -p "/dev/ttyPS1:230400" -c "setrom0"
flash_tool -d "serial" -p "/dev/ttyPS1:230400" -f "app-flash0.bin"

flash_tool -d "serial" -p "/dev/ttyPS1:230400" -c "setrom1"
flash_tool -d "serial" -p "/dev/ttyPS1:230400" -f "app-flash1.bin"

flash_tool -d "serial" -p "/dev/ttyPS1:230400" -c "reset"
```

When flashing with 2 valid partitions, the target partition for flashing will be the one NOT in use.
After flashing you can execute the `-c "swap"` command to switch boot-partition.
The bootloader will indicate which partition he expects to be flashed.  
To not get confused, you can also select which partition to write with the `-c "setrom0"` command.

When flashing partitions, the flash_tool is in control of booting. Launching a `-c reset` will, verify all partitions before booting into application code.

---

## Update procedure Gowin SPI Flash via <u>BOOTLOADER-code</u>

The bootcode has functionality for flashing gowin code into the SPI flash. But we can start with requesting the partition info.  
<u>Note:</u> the `current partition` info will always be the 1st partition because the reconfigure of the gowin is not worked out completely.

```sh
flash_tool -c "infospi"

SPI info (struct size is 56 bytes)
Nr of Partitions: 2
Current partition: 0x0 (4 bytes)
Own CRC32: 0xf022fd8f
Partition #0 (used)
         start    @ 0x0
         app size:  0x3f000
         part size: 0x40000
         crc:       0x8bedee7e
         file size: 0x360b8 - 221368
         file crc:  0xa0bc2695
Partition #1 (used)
         start    @ 0x40000
         app size:  0x3f000
         part size: 0x40000
         crc:       0x65f04bd8
         file size: 0x360b8 - 221368
         file crc:  0x5465c8bc
```

Flashing the partition is

```bash
flash_tool -f "gw-greenpower.bin" -g 0
or
flash_tool -f "gw-greenpower.bin" -g 1
```

Verification could be done by reading back the content:

- Note that the filesize is stored when flashing and reported with the `infospi` command

```bash
flash_tool -c setspi0
flash_tool -r "gw-readbackfile0.bin" -b 221368
or
flash_tool -c setspi1
flash_tool -r "gw-readbackfile1.bin" -b 221368
```

Now you could verify the sent file and the received file `md5sum` or `diff`.  
The only partition for startup is '0'. Partition '1' is meant for failsafe restore. (to be implemented in application code) We could actually erase partition0 and then the gowin will fetch partition1 code!

The flash_tool can also be used to calculate the crc32 of a file and compare it with the received information from `infospi`-command.  
```bash
flash_tool -r "gw-greenpower.bin" -t
```

---

## Update procedure Gowin SPI Flash via APPLICATION-code

**This was the initial flashing method but becomes obselete since we can flash via bootcode now!**  
The gpmcu functions as a bridge between de maincpu and the Gowin SPI Flash. We can send commands over i2c for updating the Gowin bootcode.
The _PAGE-PROGRAM_ command is 0x02 for the SPI FLASH.

```bash
flash_tool -s 2 -f "gowin-firmware.bin"
or
flash_tool -s 0x02 -f "gowin-firmware.bin"
```

We can also send other Generic commands besides the PageProgram(0x02). Check `/tools/comm_i2c_gpmcu.h`

The PAGE-READ will always return the FIRST page Adr 0x0000, the page will be printed in the gpmcu-log but we can also fetch this 256-byte buffer on i2c adddress 0x61.<br>

- 0x03 is the commands _READ-PAGE_

```sh
flash_tool -s 0x03
i2cdump -y 1 0x61
```

Checksum of a file can be compared with the content of the SPI Flash.<br>The commands takes about 1 minute for 864 pages of 256bytes.

```sh
# time ./flash_tool -s crc -f gw-firmware.bin -d /dev/i2c-1
[ INFO] (        flash_tool.c)(                          main @371) : Run SPI flash command 0Xfffffffc
[ INFO] (    comm_i2c_gpmcu.c)(   i2c_isp_flash_calculate_crc @320) : File(gw-firmware.bin)-CRC32: 927774d
[ INFO] (    comm_i2c_gpmcu.c)(   i2c_isp_flash_calculate_crc @333) : Going to read 864 blocks of 256byte + remainder of 184 bytes
[DEBUG] (    comm_i2c_gpmcu.c)(   i2c_isp_flash_calculate_crc @348) : Calculated SPI Flash crc = 927774d
[ INFO] (    comm_i2c_gpmcu.c)(   i2c_isp_flash_calculate_crc @352) : CRC's match!

real    0m 49.10s
user    0m 0.01s
sys     0m 0.20s
```

For Debug purpose there is also a 64block erase command:<br>
The `-p` parameter defines the amount of 64k blocks need to be erased <br>
(0xd8 = the _Erase64K_ block)

```sh
./flash_tool -s 0xd8 -p 1

# readback page 0 -> output on gpmcu debug line
i2ctransfer -y 1 w4@0x60 0x50 0x03 0x00 0x00

# view the readback, should be all 0xff
i2cdump -y 1 0x61 i
```

---

## Building the flash_tool

The flash_tool needs to be build for buildroot usage to run on the mainCPU.

Compile via script/docker container

```bash
./s300-scripts/build.sh -t buildroot
```
