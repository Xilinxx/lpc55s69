# Zeus 300S Bootloader

- [Zeus 300S Bootloader](#zeus-300s-bootloader)
	- [Intro](#intro)
	- [Design](#design)
		- [Flash layout](#flash-layout)
		- [Mechanisms](#mechanisms)
			- [Validation of Partitions](#validation-of-partitions)
			- [Shared SRAM](#shared-sram)
		- [Bootloader data protocol](#bootloader-data-protocol)
		- [SPI Flash layout](#spi-flash-layout)
	- [I2C](#i2c)

## Intro

While the _NXP's LPC55S69_ does have an internal ROM bootloader, it's a very minimal when it comes to functionality. While it does not fulfill the needs for the Zeus300S project, the fact that it's a ROM bootloader does mean that at least, it's very difficult to **hard brick** the device. Putting it into **ISP Mode** should (almost) always allow you to recover.

As mentioned, it is lacking some important features which are required for the Zeus300S project. Mainly, it does not support a **Dual partition** boot mechanism. In the Zeus300S project, we need this for multiple reasons. Firstly, it's not acceptable that a failed update of faulty firmware can cause the system to not boot. Since the GPMCU is responsible for the startup of the main CPU, a none booting system would be catastrophic.

Since the Main CPU which has it's power sequence controlled by the GPMCU, the update process must be performed without resetting the CPU or the pins controlling the power towards the Main CPU. This is the second reason why we need a dual partition bootloader for the Zeus300S project.

## Design

### Flash layout

Keep `boards\startup\memmap.ld` - file in sync!

```c
+--------------------+    0x00000000 (96Kb)
|                    |
|                    |
|    BOOTLOADER      |    // Bootloader software
|                    |
|                    |
+--------------------+    0x00018000 (8KB)
|    FREE            |    // 8Kb Free
+--------------------+    0x0001A000 (1KB)
|    BOOT CTXT       |    // 1k Boot context
+--------------------+    0x0001A400 (1KB)
|    BOOT CTXT.BAK   |    // 1k Boot context backup
+--------------------+    0x0001A800
|    FREE            |    //  Free (22Kb 0 0x5800)
+--------------------+    0x00020000 (251KB = 0x3EC00)
|                    |
|                    |
|                    |
|    APPLICATION 0   |    // Main application 0
|                    |
|                    |
|                    |
|                    |
+--------------------+    0x0005EC00 (251KB = 0x3EC00)
|                    |
|                    |
|                    |
|                    |
|    APPLICATION 1   |    // Main application 1
|                    |
|                    |
|                    |
|                    |
+--------------------+    0x0009D800
|      RESERVED      |    // manufacturer reserved
+--------------------+

```

### Mechanisms

The bootloader integrates multiple mechanisms to protect the fail-safe operation or allow easy debugging in case of problem.
The first 128KB is reserved for bootloader and other information. Leaving 251KB for 2 application partitions.

#### Validation of Partitions

At the end of the bootlader, a structure containing the bootloader and application info is stored. This structure looks as following:

```c
struct bootloader_ctxt_t {
	struct app_ctxt_t	apps[BTLR_NR_APPS];     //!< app contexts
	app_partition_t		part;                   //!< Current APP Partition
	uint32_t		      crc;                    //!< Boot info ctxt crc
};
```

```c
struct app_ctxt_t {
	uint32_t	start_addr;             //!< Start address of Application
	uint32_t	application_size;       //!< Actual address size
	uint32_t	partition_size;         //!< Size of the application partition
	uint32_t	crc;                    //!< CRC of the image
};
```

It contains vital information about the application partitions. Firstly, the start address for the application. This needs to be able to change dependant on the flash configuration. Then 2 sizes are included, the actual application and the size of the flash partition. The application size is important to correctly calculate the CRC of the application. The partition size is again to allow resizing of the partitions and validate that application actually fits in the partition.

Lastly, it contains the CRC that was received from the hostmachine when the application was flashed. This CRC is validated against the CRC the bootloader calculates based on the start address and the application size. If these do not match, problems. Either the application is invalid or the `app_ctxt_t` struct is invalid.

To detect invalid `app_ctxt_t` the `bootloader_ctxt_t` struct is also validated based on the `crc` field and a CRC calculated by the bootloader. If these CRC's don't match, the system needs to be reflashed. Bootloader remains operational though so there is still communication with system.

The bootloader requires both flashed partitions to be 'valid' before it will continue to application code. The flash_tool application will take care of this.

#### Shared SRAM

The bootloader and application share a region in RAM which is used for logging. The goal of this region is to be able to retrieve logging from the application when an error occurs and the application is not able to recover. This data start from address `0x20040000` to `0x20044000` which is a size of 16KB. Currently the data can be dumped using GDB (once connected to GDB) using the command:

```bash
dump memory out.bin 0x20040000 0x20044000
```

This will dump the region of memory into a file called `out.bin`. This file can then be parsed using a simple Golang script. Running the Golang script can be done using:

```bash
go run scripts/process_memlog.go
```

This command should generate some output similar to this example:

```txt
0 : Logger mem size: 16384 bytes
1 : Board initialized
2 : CRC32 Algo initialized
3 : Flash driver initialized
4 : Bootloader (11:09_20-08-20_v0.1-5-g4970fd8-dirty)
5 : App size: 44c00
6 : Entry: 0x20002230 SP: 0x20002230  PC: 0xd851
7 : APPROM0 CRC: 2d672e89
```

### Bootloader data protocol

Due to flashsize limitations, the bootloader protocol has been designed to be as lightweight as possible, yet still being very flexible / extendable. The stack itself consists of 3 main components.

1. **The COMMP stack**: Contained within the files `commp_protocol.c/.h` and `commp_parser.c.h` is the actual protocol stack. The COMMP protocol is loosely based on TFTP, which is a "semi-reliable" protocol. TFTP, and COMMP consequently, allows for a reliable transmission of data. However it doesn't implement any validation of the received data. So while the protocol can validate that the correct number of bytes have been send, it doesn't have any means built-in to verify if those were the correct bytes. The COMMP stack solves this problem by validating the data written to storage. The validation procedure while very simple is very effective. The hostmachine provides a CRC which it calculates, this is stored by the bootloader and at the end of the transfer, it validates the provided CRC with one calculated by the bootloader.

2. **COMM Drivers**: In order to design a flexible and testable protocol, the stack itself has no knowledge of how the data is physically handled. The API for this abstraction is located in `comm_driver.h` and implementations of these kinds of drivers can be found in `comm_uart.c`, `comm_serial.c` and `comm_tcp.c`.

3. **Storage Drivers**: Since the bootloader has to be capable of storing data to multiple types of storage media, the same approach of abstraction as the COMM drivers is used. API definition for the storage layer can be found in `storage.h`. An example of this driver can be found in `storage_flash.c`, `storage_spi_flash.c` and `storage_linux_file.c`.

---

### SPI Flash layout

The SPI Flash holds the Gowin bitfile bootcode. Via a GPIO-Mux the GPMCU gets access to this chip.  
The last eraseable block (4kb(0x1000)) of the partitions holds crc info context.

The `flash_tool` is used to update the Gowin bitfile over the GP-UART.

File: `include/storage_spi_flash.h`

- Actual chip IS25LP128 holds 128Mbit or 16MByte, only a fraction is reserved for Gowin code now.
- See [SPI Wiki](https://wiki.barco.com/display/p900/Platform+300-+Gpmcu+SPI)
- The CONTEXT at the end of partition is kept identical for both partitions (as backup).

```C
 typedef struct spi_partition_ctxt_t {
 uint32_t  start_addr;             //!< Start address of bitfile image
 uint32_t  image_size;             //!< Actual image size
 uint32_t  partition_size;         //!< Size of the partition
 uint32_t  crc;                    //!< CRC of the image
 uint32_t  bitfile_size;           //!< Size of the uploaded bitfile
 uint32_t  bitfile_crc;            //!< CRC32 on the length of the file
} spi_partition_ctxt_t;
```

- <u>Note</u> that we store the bitfile size, so we can readback the actual flashed data and check it's crc32.
  Bitfile size is usually identical but not hardcoded.  
  The crc is calculated on the <i>image_size</i> but if we grow the partitions to 1MByte for example we should consider checking the file-size crc in flash instead.

```C
+--------------------+    0x0000 0000 (256Kb)
|                    |
|                    |
|    GOWIN PART0     |    // Gowin boot code
|                    |
|    CONTEXT 4KB     |    0x0003 F000 (4KB)
+--------------------+    0x0004 0000 (256KB)
|                    |
|                    |
|    GOWIN PART1     |   // Gowin boot code backup
|                    |
|    CONTEXT 4KB     |    0x0007 F000 (4KB)
+--------------------+    0x0008 0000 (16MByte - 2*256KB)
|                    |
|                    |
|      FREE          |
|                    |
|                    |
|                    |
+--------------------+    0x0100 0000

```

<u>Note:</u> Erasing partition0 makes the gowin fetch the 2nd partition automatically. Could be used for failsafe strategy

- Toggling the `RECONFIGURE` pin of the Gowin re-initiates the bitfile load. (And turns off the MainCPU)

---

## I2C

No I2C implementation in the bootloader code.  
The I2C communication with the mainCPU is used in application code (as where the UART is reserved for Application-communication)
