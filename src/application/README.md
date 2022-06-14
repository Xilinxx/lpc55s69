# Zeus 300S Application

* [Intro](#intro)
* [Design](#design)
  + [Flash layout](#flash-layout)
* [Software](#Software)
* [Communication example](#Communication)

## Intro

The *NXP's LPC55S69* will contain the application code twice in different versions. Programming/Flashing the `APPLICATION0/1` into flash is taken into account by the bootloader using a flash_tool cmd-line program.
Making data available for the MainCPU and controlling the FPGA power sequencer (Gowin) is part of this application.  Check the architecture overview for more details.

* [Prestudy - wiki](https://wiki.barco.com/display/p900/Platform+300+-+Green+Power+Processor+pre-study)
* [Architecture overview - wiki](https://wiki.barco.com/display/p900/Platform+300-Linux+Green+Power+Controller)
* [Communication protocol towards MainCPU - wiki](https://wiki.barco.com/display/p900/Platform+300+-+GPmcu+to+Maincpu+communication)
* [__asm - Output operands in gcc](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#OutputOperands)



## Design

### Flash layout (copy from bootloader Readme)

```c
+--------------------+    0x00000000
|                    |
|                    |
|     BOOTLOADER     |    // Bootloader software
|                    |
|                    |
+--------------------+    0x0000C800 (50KB)
|                    |
|                    |
|                    |
|    APPLICATION 0   |    // Main application 0
|                    |
|                    |
|                    |
+--------------------+    0x00051400 (275KB)
|                    |
|                    |
|                    |
|    APPLICATION 1   |    // Main application 1
|                    |
|                    |
|                    |
+--------------------+    0x00096000 (275KB)
|                    |
|      RESERVED      |    // Not used
|                    |
+--------------------+    0x00097C00 (7KB)
|      BOOTINFO      |    // Contains Application info + CRC's
+--------------------+    0x00098000 (1KB)
```
#### Shared Flash
Besides the logging, there is also a BOOTINFO format shared in flash memory from `0x00097C00`. The application can switch the application partition via the functions `_bootloader_swap_partitions` and `_bootloader_store_ctxt` in bootloader_helpers-code.


#### Shared SRAM
The bootloader and application share a region in RAM which is used for shared memory and logging. The goal of the log-region is to be able to retrieve info from the application when an error occurs and the application is not able to recover. This data starts from address `0x20042000` upto `0x20044000` which is a size of 8KB. This data will be made available for the MainCPU.
the ssram starts with a struct `ssram_data_t` at `0x20040000` which can be found in the header `memory_map`. (Starting with `reboot_counter`-variable, etc.)
When the Application requires an update, we signal the bootloader via the `application_request`-variable and then initiate a reset for jumping to bootcode. Typically for a Firmware update but can also be used to switch the application's boot partition.

___
## Software

### Communication
The *communication Protocol* has been integrated together with a *data_map* access method.

Commands are divided in 3 types:
* byte read/write
* integer/4byte read/write
* array read/write

Detailed explanation on [Communication protocol towards MainCPU - wiki](https://wiki.barco.com/display/p900/Platform+300+-+GPmcu+to+Maincpu)
```c
+-----------------------+
|                       |    // Communication:
|    comm/protocol      |    // (1) - main-loop calls comm_protocol()
|    comm/comm          |    // (2) - main-loop calls COMM_Handler() , if messages are present in LCD_Protocol
|                       |    //                 + calls Run_CommHandler() when we need data-map read/write
|                       |    //       if commands don't need to acces the global data-struct we handel them here
|                       |    //       for example the BOOTLOG-reading.
+-----------------------+
|                       |
|    run/comm_run       |    // Generic communication-loop -
|                       |    // (3) - Run_CommHandler(), called from comm/comm.c
|                       |    //       Here we handle the various read/write types towards data_map.c structure
|                       |    // (4) - WriteCommandHandler() or ReadCommandHandler()
|                       |    //       specific tasks related to the selected commands.
|                       |
|                       |    //       All commands are defined in `Command[]`
|                       |
+-----------------------+
|    spi/spi_master.c   |    // spi-master for the Gowin Flash boot-device (is25xp-device)
|    spi/is25xp.c       |    // Flash device is25xp
+-----------------------+
|    i2c/i2c_slave.c    |    // i2c-slave implementation
|    i2c/i2c_master.c   |    // i2c-m implementation (in low-power)
+-----------------------+
|    data_map.c         |    // Global variable data structure
|    softversions.h     |    // definition of the software versions
|    flash_helper.c     |
|    logger_conf.c      |
|    main.c             |
+-----------------------+
```
## MainCPU Communication Example - Switching from application code to bootcode
The Byte-write command's index 0x20 contains the bootcode-byte.
We will send value 0x01, representing the UPDATE request.
We send : `{ FE, 10, 11, 20, 01, <CRC>, FF }`

We will receive data ACK: `{ FE, 10, 11, 01, 22, FF } = ACK`

Now we still need to reset the application with the reset command on index 0x21.

We send : `{ FE, 10, 11, 21, 01, <CRC>, FF }`

We will receive data ACK: `{ FE, 10, 11, 01, 22, FF } = ACK`

The watchdog runout will be activated and the microcontroller will reboot.
Bootloader code will detect the 0x01 value and wait for flash update initiated by the flsh_tool-program.

### Shell-script to switch to boot partition:
```sh
#!/bin/sh

#set gp to boot partition
i2cset -y 5 0x60 0x30 0x01
#reset request
i2cset -y 5 0x60 0x31 0x01

echo "Execute flash_tool to the correct partition once gp rebooted!"
echo "flash_tool -d "serial" -p "/dev/ttyPS1:230400" -f app-flash?.bin"
```

## Gowin UART Communication
see [gowin wiki](https://wiki.barco.com/display/p900/GreenPOWER+FPGA+%28Gowin%29+Design)<br>
**Remark**: In the Gowin UART IRQ you will find a watchdog clear which usually only happens in the main-routine. Encountered a case where Gowin Fpga floods the uart buffer which caused a SW reset. Now we disable the USART1 if we get flooded.

Gowin Fpga needs to receive default Edid/DPCD data. This is transmitted upon startup of this application code.
___
## I2C communication

### I2C address 0x60 - (default bus is nr 5)
The byte read/write commands are accessible via a simplified i2c-slave protocol from the MainCPU. The i2c 8bit address is set to 0x60  in software.  
By example:

* set gp to boot partition :  
`i2cset -y 5 0x60 0x30 0x01`
* reset request :  
`i2cset -y 5 0x60 0x31 0x01`
* Gowin PIO commands go via register 0x40 :  
`i2cset -y 5 0x60 0x40 0x<value>`

### relevant PIO cmds are:
| Value|hex   | Description |
| ---- |----- | ----------- |
| V | 0x56    | PANEL_VOLTAGELO selection (10v) |
| v | 0x76    | PANEL_VOLTAGEHI selection (12V) |
| L | 0x4c    | BACKLIGHT_OFF |
| l | 0x6c    | BACKLIGHT_ON  |
| P | 0x50    | PANEL_PWR_OFF |
| p | 0x70    | PANEL_PWR_ON  |
(see [gowin wiki](https://wiki.barco.com/display/p900/GreenPOWER+FPGA+%28Gowin%29+Design)<br>
TO DO: A register based approach is planned instead of this ASCII based communication.


### I2C address 0x61 - SPI flash readback data
A custom command for i2c page access has been added for reading back SPI flash pages.
Address 0x61 holds 256bytes of SPI content. Used by `flash_tool` command for read back!
This command has been added beause i2cdetect only reads 1 byte, causing a communication timout if we alwasy send 256bytes.
The default startup behaviour is per byte.
* page access (read 256bytes at once) enabled<br>
`i2cset -y 5 0x61 0xff 0x01`
* page access disabled (=per byte)<br>
`i2cset -y 5 0x61 0xff 0x00`

### I2c address 0x62 - emulates 24 byte EEPROM data (24c02 device alike)
* `i2ctransfer -y 5 r12@0x62` --> `0x30 0x30 0x30 0x34 0x41 0x35 0x30 0x46 0x31 0x33 0x36 0x30`
* Used for storing Mac-address   (data_map.c)
```c
 static const u8 eepromInitValues[] = {
    // Mac address eth0: 0004A50F1360
    0x30, 0x30, 0x30, 0x34, 0x41, 0x35, 0x30, 0x46, 0x31, 0x33, 0x36, 0x30,
    // Mac address eth1: 0004A50F135F
    0x30, 0x30, 0x30, 0x34, 0x41, 0x35, 0x30, 0x46, 0x31, 0x33, 0x35, 0x46};
 ```

___
* In low power mode the GPmcu will act as a master. (to do)
___
### SPI-master communication over I2C
The gowin SPI Flash (holding the bitstream boot-code) is only accessible via the GPmcu. Therefor a specific address has been chosen on the byteRead/Write `0x50`. The Write implementation got an additional <cmd-type>-byte which will be followed by 256byte PageProgram information. The command byte value is kept consistent with the SPI Flash datasheet. Every command is bridged in the GPmcu and then put on the SPI-bus in a *blocking* manner.
* Implementation is in the <u>run/comm_run.c</u> function:  <br> `u16 byteWrite2SPI(u8 *data, u16 size);`

Each communication towards the SPI Flash requires the GPIO MUX-select to switch the communication lines of the Flash.

#### SPI Flash WRITE via i2c

Address consists of 3 bytes but the lowest adr is ignored because we read/program per 256.

* example for reading 256byte from page 0

  `i2cset -y 5 0x60 0x50 0x03 0x00 0x00 i` <br>

The output is only available on the GP uart log! <br>Example:
```txt
[DEBUG] (        spi_master.c)(                 spi_read_page @168) : is25xp_bread returned [1]
        0000 : ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
        0010 : ff ff ff ff ff ff a5 c3 06 00 00 00 11 00 38 1b
        0020 : 10 00 00 00 00 00 00 00 51 00 ff ff ff ff ff ff
        0030 : 0b 00 00 00 d2 00 ff ff 00 ff f0 00 12 00 00 00
               ...
        00F0 : 00 00 00 00 00 00 00 00 00 00 00 00 00 30 10 00
  ```
* <b>PageProgram</b> (0x02) takes place in two steps <br>
  1) Set the 2byte Address page to flash <br>`i2ctransfer -y 5 w4@0x60 0x50 0x02 0x00 0x00`<br>
  2) send the 256 bytes data <br>`i2ctransfer -y 5 w256@0x60 <data>`

* <b>Erase</b> can be performed in 4k,32k or 64k Blocks + bulk erase.
  <br>The Bulk erase does take around 30seconds and should not be used to keep the gpmcu responsive.

#### SPI Flash <b>READ</b> via i2c
The i2c-read options on addres 0x50..0x53 are returning the Flash identifaction bytes. See [SPI Wiki](https://wiki.barco.com/display/p900/Platform+300-+Gpmcu+SPI)<br>
  The read data-page can be fetched with the command: `i2cdump -y 5 0x61`  
  Address <i>0x61</i> returns the content of <u>Main.debug.spiFlashPage</u> which holds the 256 bytes.  
  This output will be equal to the displayed debug info on the Gpmcu output.  
  <u>Note:</u> When no PageRead command was initiated, the return data will be 0's.  
  <u>Note:</u> PageRead is enabled -> see i2c on 0x62.
___
## Remarks
* Application reset/exit happens via the watchdog function in the main-routine
* Beautify your code with Uncrustify : `uncrustify -c ./uncrustify_objc.cfg --no-backup --replace src/application/*`
