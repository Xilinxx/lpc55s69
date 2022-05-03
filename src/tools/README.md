# Zeus 300S tools

* [Intro](#intro)

## Intro

Since the bootloader is a custom design in order to fullfil the specific needs of the project. Dedicated tools are made to support the usage of the bootloader. Such as the tool used to flash the application binaries. This tool is very purposely called `flash_tool`.

## Usage

First and foremost `flash_tool -h` will / should tell you everything you need to know. But just for completion. Below a small user guide

In order to correctly communicate with the device it needs to know what physical interface it needs to use. Currently supported are 'ip' and 'serial'. These options are passed via the `-d` option.

```bash
flash_tool -d ip
# Or
flash_tool -d serial
```

Now, this is not enough, interfaces need configuration parameters so `-p` is used to specify connection parameters. For `serial` this is `filepath:baudrate` and for `ip` it `IPAddress:Port`.

```bash
flash_tool -d ip -p "127.0.0.1:6000"
# Or
flash_tool -d serial -p "/dev/ttyUSB0:115200"
# On target Zeus
flash_tool -d serial -p "/dev/ttyPS1:230400" -c "info"
```

Now the tool can actually do something. This is either via:

`-c` to manually send a command. (Available commands: "boot", "info", "swap", "reset", "powon", "powoff", "wdog")
```
	- boot: Force an application boot
	- force: Retrieve bootloader context (Partition info)
	- swap: Swap partition to be written to
	- reset: Reset bootloader
	- powon: wake up mainCpu
	- powoff: put mainCpu in sleep
	- wdog: trigger the watchdog
`-f` to send a application bin file
`-z` special case, this triggers a debug handler in the COMMP Protocol. This is useful to manually trigger a given action which is not a bootloader command. (For example, force a wipe of the flash during debugging).
`-t` force calculation of crc only, argument is -f `app-flash0.bin`

```bash
flash_tool -d ip -p "127.0.0.1:6000" -c "bootinfo"
flash_tool -d ip -p "127.0.0.1:6000" -f <filename.bin>
flash_tool -d ip -p "127.0.0.1:6000" -z
```
___
## Update procedure

The gpmcu bootloader will check for two valid partitions before booting into application code.
So we need to execute following commands on a new board,
```bash
flash_tool -d "serial" -p "/dev/ttyPS1:230400" -f "app-flash0.bin"
flash_tool -d "serial" -p "/dev/ttyPS1:230400" -f "app-flash1.bin"
```
When flashing with 2 valid partitions, the target partition for flashing will be the one NOT in use.
After flashing you can execute the -c "swap" command to switch boot-partition.
The bootloader will indicate which partition he expects to be flashed.


## Update procedure Gowin SPI Flash
The gpmcu functions as a bridge between de maincpu and the Gowin SPI Flash. We can send commands over i2c for updating the Gowin bootcode.
The *PAGE-PROGRAM* command is 0x02 for the SPI FLASH.
```bash
flash_tool -s 2 -f "gowin-firmware.bin"
or
flash_tool -s 0x02 -f "gowin-firmware.bin"
```
We can also send other Generic commands besides the PageProgram(0x02). Check `/tools/comm_i2c_gpmcu.h`

The PAGE-READ will always return the FIRST page Adr 0x0000, the page will be printed in the gpmcu-log but we can also fetch this 256-byte buffer on i2c adddress 0x61.<br>
* 0x03 is the commands *READ-PAGE*
```bash
flash_tool -s 0x03
i2cdump -y 1 0x61
```
Checksum of a file can be compared with the content of the SPI Flash.<br>The commands takes about 1 minute for 864 pages of 256bytes.
```txt
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
The `-p` parameter defines the amount of 64k blocks needs to be erased <br>
(0xd8 = the *Erase64K* block)
```sh
./flash_tool -s 0xd8 -p 1

# readback page 0 -> output on gpmcu debug line
i2ctransfer -y 1 w4@0x60 0x50 0x03 0x00 0x00

# view the readback, should be all 0xff
i2cdump -y 1 0x61 i
```


___
## Building the flash_tool

The flash_tool needs to be build for buildroot usage to run on the mainCPU.

Compile via script/docker container
```bash
./s300-scripts/build.sh -t buildroot
```
