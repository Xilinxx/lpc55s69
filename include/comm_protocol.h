/**
 * @file comm_protocol.h
 * @brief  Abstract layer for the communication protocol towards the GPMCU
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-24
 */

#ifndef _COMM_PROTOCOL_H_
#define _COMM_PROTOCOL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "bootloader.h"
#include "comm_driver.h"
#include "storage.h"

#define COMMP_ROMNAME_ROM0 "approm0\0"          //!< Constant for ROM name 0
#define COMMP_ROMID_ROM0 (0x1)                  //!< ID For ROM 0
#define COMMP_ROMNAME_ROM1 "approm1\0"          //!< Constant for ROM name 1
#define COMMP_ROMID_ROM1 (0x2)                  //!< ID For ROM 1
#define COMMP_ROMNAME_SPIFLASH0 "spi0\0"        //!< Constant for SPI Flash 0
#define COMMP_ROMID_SPIFLASH0 (0x3)             //!< ID For ROM 1

typedef enum __attribute__((__packed__)) {      //!< Force enum to be packed to 1 byte
	COMMP_RRQ = 0x1,                        //!< Read request
	COMMP_WRQ = 0x2,                        //!< Write request
	COMMP_DATA = 0x3,                       //!< Data packet
	COMMP_ACK = 0x4,                        //!< Data acknowledge
	COMMP_ERR = 0x5,                        //!< Error packet
	COMMP_CMD = 0x6,                        //!< Command packet
	COMMP_DEBUG = 0x7,
	COMMP_NR_OF_OPCODES                     //!< Number of Opcodes
} comm_proto_opcode_t;

typedef enum __attribute__((__packed__)) {      //!< Force enum to be packed to 1 byte
	COMMP_CMD_BOOT = 0x1,                   //!< End the COMMP Stack via SW
	COMMP_CMD_CRC,                          //!< CRC Command packet
	COMMP_CMD_SWAP,                         //!< Force a partition swap
	COMMP_CMD_BOOTINFO,
	COMMP_CMD_PWR_ON,
	COMMP_CMD_PWR_OFF,
	COMMP_CMD_END,                          //!< End the COMMP Stack via SW
	COMMP_CMD_RESET,                        //!< Reset bootloader code
	COMMP_CMD_TRIGGER_WDOG,
	COMMP_CMD_ERASE_SPI,
	COMMP_NR_OF_COMMANDS                    //!< Number of commands
} comm_proto_cmd_t;

#define COMMP_CMD_ERROR -1

typedef enum __attribute__((__packed__)) {      //!< Force enum to be packed to 1 byte
	COMMP_ERR_SEQ = 0x1,                    //!< Sequence error, packet not what expected
	COMMP_ERR_WRITE,                        //!< Write error, failed to write to storage
	COMMP_NR_OF_ERRCODES,                   //!< Number of commands
} comm_proto_errcode;

#define COMMP_OPCODE_ZERO_BYTE 0        //!< Byte offset for the Opcode zero byte
#define COMMP_OPCODE_BYTE 1             //!< Bye offset for the Opcode byte

#define COMMP_RRQ_LENGTH_OFFSET 2       //!< Offset to read offset (4byte)
#define COMMP_RRQ_ROMNAME_OFFSET 6      //!< Romname byte offset

#define COMMP_WRQ_ROMNAME_OFFSET 2      //!< Romname byte offset

#define COMMP_DATA_PNUMBER_MSB 2        //!< MSB offset of the data packet number
#define COMMP_DATA_PNUMBER_LSB 3        //!< LSB offset of the data packet number
#define COMMP_DATA_OFFSET 4             //!< Data byte offset

#define COMMP_ACK_BUFFER_SIZE 4         //!< Ack packet buffer size
#define COMMP_ACK_PNUMBER_MSB 2         //!< Ack packet number MSB offset
#define COMMP_ACK_PNUMBER_LSB 3         //!< Ack packet number LSB oofset

#define COMMP_ERROR_BUFFER_SIZE 128     //!< Error packet size
#define COMMP_ERROR_MAX_STRING 124      //!< Maximum error string length
#define COMMP_ERROR_ERRCODE_MSB 2       //!< Error code MSB offset
#define COMMP_ERROR_ERRCODE_LSB 3       //!< Error code LSB offset
#define COMMP_ERROR_ERRSTR_OFFSET 4     //!< Error string byte offset

#define COMMP_CMD_CMDCODE_MSB 2         //!< Command code MSB offset
#define COMMP_CMD_CMDCODE_LSB 3         //!< Command code LSB offset
#define COMMP_CMD_DATA_OFFSET 4         //!< Command code DATA offset
#define COMMP_CMD_CRC_BYTE0 4           //!< CRC data byte offset
#define COMMP_CMD_CRC_BYTE1 5           //!< CRC data byte offset
#define COMMP_CMD_CRC_BYTE2 6           //!< CRC data byte offset
#define COMMP_CMD_CRC_BYTE3 7           //!< CRC data byte offset
#define COMMP_CMD_CRC_PACKET_SIZE 8     //!< CRC Packet size
#define COMMP_CMD_END_PACKET_SIZE 4     //!< CRC Packet size
#define COMMP_CMD_PACKET_SIZE 4         //!< Generic CMD Packet size

#define COMMP_PACKET_SIZE 516           //!< Block size
#define COMMP_DATA_SIZE 512             //!< Data size

#pragma pack(push, 1)
struct comm_proto_rrq_wrq_t {
	uint8_t		zero_byte;      //!< 1 Zero byte
	uint8_t		opcode;         //!< 1 Byte opcode
	uint32_t  length;					//!< 2 Byte length to read in bytes
	uint8_t * romname; //!< Null-terminated ROM name
};

struct comm_proto_data_t {
	uint8_t		zero_byte;      //!< 1 Zero byte
	uint8_t		opcode;         //!< 1 Byte opcode
	uint16_t	blocknr;        //!< 2 Byte block number
	uint8_t *	dataptr;        //!< Data pointer
};

struct comm_proto_ack_t {
	uint8_t		zero_byte;      //!< 1 Zero byte
	uint8_t		opcode;         //!< 1 Byte opcode
	uint16_t	blocknr;        //!< 2 Byte block number
};

struct comm_proto_err_t {
	uint8_t		zero_byte;      //!< 1 Zero byte
	uint8_t		opcode;         //!< 1 Byte opcode
	uint16_t	errcode;        //!< 2 Byte error code
	uint8_t *	errstr;         //!< Null-terminated error string
};

struct comm_proto_cmd_t {
	uint8_t		zero_byte;      //!< 1 Zero byte
	uint8_t		opcode;         //!< 1 Byte opcode
	uint16_t	cmdcode;        //!< 2 Byte error code
	uint8_t *	data;           //!< Null-terminated data string
};
#pragma pack(pop)

/**
 * @brief  Initialize the communication protocol
 *
 * @returns  -1 if error, otherwise 0
 */
int comm_protocol_init();


/**
 * @brief  Check if a packet buffer is a given type
 *
 * @param data The data buffer in question
 * @param type The type against which we validate
 *
 * @returns   true or false
 */
bool comm_protocol_is_packet_type(uint8_t *data, comm_proto_opcode_t type);

/**
 * @brief  Check if a packet buffer is a given command
 *
 * @param data The data buffer in question
 * @param type The command type against which we validate
 *
 * @returns   true or false
 */
bool comm_protocol_is_command_type(uint8_t *data, comm_proto_cmd_t type);

/**
 * @brief  Write to a given protocol driver
 *
 * @param drv The driver to which the data will be written
 * @param data The actual data
 * @param len Length of the data
 *
 * @returns  -1 if failed, otherwise 0
 */
int comm_protocol_write_data(struct comm_driver_t *drv, uint8_t *data, size_t
			     len);

/**
 * @brief  Read from a given protocol driver
 *
 * @param drv The driver from which the data will be read
 * @param data The actual data
 * @param len Length of the data to be read and after transaction, the actual size read
 *
 * @returns  -1 if failed, otherwise 0
 */
int comm_protocol_read_data(struct comm_driver_t *drv, uint8_t *data, size_t *len);

/**
 * @brief  Retrieve a driver with a given name
 *
 * @param name Driver name
 *
 * @returns  NULL or a reference to the driver
 */
struct comm_driver_t *comm_protocol_find_driver(char *name);

/**
 * @brief  Run a The protocol stack
 *
 * @param bctxt The bootloader context
 * @param cdriver The comm driver used
 * @param sdriver The storage driver used
 *
 * @returns 0 or -1 if failed
 */
int comm_protocol_run(struct bootloader_ctxt_t *bctxt,
					struct comm_driver_t *cdriver,
					struct storage_driver_t *sdriver,
					struct storage_driver_t *spidriver);

/**
 * @brief Transfer a given file to one of the ROM partition
 *
 * @param cdriver Comm driver used to transfer
 * @param buffer Data buffer
 * @param len Length of th data buffer
 * @param rom_nr ROM number
 * @param crc The CRC of the binary
 *
 * @returns TBD...
 */
int comm_protocol_transfer_binary(struct comm_driver_t *cdriver, uint8_t *buffer, size_t len, uint8_t rom_nr, uint32_t crc);

int comm_protocol_read_binary(struct comm_driver_t *cdriver, uint8_t *buffer, uint16_t blocknr, size_t len, uint8_t rom_nr, size_t totalByteCount);

int comm_protocol_force_boot(struct comm_driver_t *cdriver);

struct bootloader_ctxt_t comm_protocol_retrieve_bootinfo(struct comm_driver_t *cdriver);

/**
 * @brief  Close the communication protocol
 */
void comm_protocol_close();

#endif /* _COMM_PROTOCOL_H_ */
