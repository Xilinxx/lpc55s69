/**
 * @file commp_cmd_helpers.c
 * @brief  COMMP Stack command helpers
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version
 * @date 2020-11-05
 */

#include "comm_protocol.h"
#include "comm_parser.h"
#include "logger.h"

uint32_t _comm_get_crc(uint8_t *data)
{
	uint32_t crc32 = (uint32_t)((data[COMMP_CMD_CRC_BYTE0] << 24) |
				    (data[COMMP_CMD_CRC_BYTE1] << 16) |
				    (data[COMMP_CMD_CRC_BYTE2] << 8) |
				    data[COMMP_CMD_CRC_BYTE3]);
	return crc32;
}

int _comm_send_bootinfo(struct comm_driver_t *cdriver)
{
	uint8_t out_buffer[COMMP_CMD_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_BOOTINFO;

	int error = comm_protocol_write_data(cdriver, out_buffer,
					     COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of BOOTINFO Failed");
	}
	return error;
}

int _comm_send_boot(struct comm_driver_t *cdriver)
{
	uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_BOOT;

	int error = comm_protocol_write_data(cdriver, out_buffer,
					     COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of BOOT Failed");
	}
	return error;
}

int _comm_send_reset(struct comm_driver_t *cdriver)
{
	uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_RESET;

	int error = comm_protocol_write_data(cdriver, out_buffer,
					     COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of RESET Failed");
	}
	return error;
}

int _comm_send_power(struct comm_driver_t *cdriver, bool state)
{
	uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = state ? COMMP_CMD_PWR_ON : COMMP_CMD_PWR_OFF;

	int error =
		comm_protocol_write_data(cdriver, out_buffer, COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of POWER Failed");
	}
	return error;
}

int _comm_send_swap(struct comm_driver_t *cdriver)
{
	uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_SWAP;

	int error =
		comm_protocol_write_data(cdriver, out_buffer, COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of SWAP Failed");
	}
	return error;
}

int _comm_send_crc(struct comm_driver_t *cdriver, uint32_t crc)
{
	uint8_t out_buffer[COMMP_CMD_CRC_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_CRC;

	out_buffer[COMMP_CMD_CRC_BYTE0] = (uint8_t)((crc & 0xff000000) >> 24);
	out_buffer[COMMP_CMD_CRC_BYTE1] = (uint8_t)((crc & 0x00ff0000) >> 16);
	out_buffer[COMMP_CMD_CRC_BYTE2] = (uint8_t)((crc & 0x0000ff00) >> 8);
	out_buffer[COMMP_CMD_CRC_BYTE3] = (uint8_t)(crc & 0x000000ff);

	LOG_OK("%x %x %x %x", out_buffer[COMMP_CMD_CRC_BYTE0],
	       out_buffer[COMMP_CMD_CRC_BYTE1],
	       out_buffer[COMMP_CMD_CRC_BYTE2],
	       out_buffer[COMMP_CMD_CRC_BYTE3]);

	int error = comm_protocol_write_data(cdriver, out_buffer,
					     COMMP_CMD_CRC_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of CRC Failed");
	}
	return error;
}

int _comm_send_end(struct comm_driver_t *cdriver)
{
	uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_END;

	int error = comm_protocol_write_data(cdriver, out_buffer,
					     COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of END Failed");
	}
	return error;
}

int _comm_send_wdogreset(struct comm_driver_t *cdriver)
{
	uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_TRIGGER_WDOG;

	int error = comm_protocol_write_data(cdriver, out_buffer,
					     COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of WDOGRESET Failed");
	}
	return error;
}

int _comm_send_wipespi(struct comm_driver_t *cdriver)
{
	uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];
	out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
	out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
	out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
	out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_ERASE_SPI;

	int error = comm_protocol_write_data(cdriver, out_buffer,
					     COMMP_CMD_END_PACKET_SIZE);
	if (!error) {
		LOG_ERROR("Send of WIPESPI Failed");
	}
	return error;
}
