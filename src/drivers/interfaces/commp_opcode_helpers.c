/**
 * @file commp_opcode_helpers.c
 * @brief  OPCode helpers for COMMP stack
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version
 * @date 2020-11-05
 */

#include <string.h>
#include <stdlib.h>

#include "comm_protocol.h"
#include "comm_parser.h"
#include "logger.h"

void _reset_tranfer_ctxt(struct comm_ctxt_t *ctxt)
{
	ctxt->crc_received = false;
	ctxt->last_blocknr = 0;
	ctxt->transfer_in_progress = false;
	//ctxt->part_nr = 0;
	//ctxt->rom_readsize = 0;
}

int _comm_send_rrq(struct comm_driver_t *cdriver, struct comm_ctxt_t *ctxt)
{
	struct comm_proto_rrq_wrq_t rrq = { 0 };
	rrq.opcode = COMMP_RRQ;
	rrq.length = (uint32_t)ctxt->rom_readsize;
	rrq.romname = (uint8_t *)malloc(sizeof(COMMP_ROMNAME_ROM0));
	if (!rrq.romname) {
		LOG_ERROR("Failed to allocate memory");
		return -1;
	}
  size_t size_of_romname = 0;
	memset(rrq.romname, 0, sizeof(COMMP_ROMNAME_ROM0));
	if (ctxt->part_nr == 0) {
		memcpy(rrq.romname, COMMP_ROMNAME_ROM0, sizeof(COMMP_ROMNAME_ROM0));
		size_of_romname = sizeof(COMMP_ROMNAME_ROM0);
	}
	else if (ctxt->part_nr == 1)
	{
		memcpy(rrq.romname, COMMP_ROMNAME_ROM1, sizeof(COMMP_ROMNAME_ROM1));
		size_of_romname = sizeof(COMMP_ROMNAME_ROM0);
	}
	else if (ctxt->part_nr == COMMP_ROMID_SPIFLASH0)
	{
		memcpy(rrq.romname, COMMP_ROMNAME_SPIFLASH0, sizeof(COMMP_ROMNAME_SPIFLASH0));
		size_of_romname = sizeof(COMMP_ROMNAME_SPIFLASH0);
	}
	else
	{
		LOG_ERROR("Invalid APPROM");
		free(rrq.romname);
		return -1;
	}

	size_t out_len = sizeof(uint16_t)*3 + sizeof(COMMP_ROMNAME_ROM0);

	/* TODO: Probably we could use a static buffer here */
	uint8_t *out_buffer = malloc(out_len);
	if (!out_buffer) {
		LOG_ERROR("Failed to allocate out buffer");
		free(rrq.romname);
		return -1;
	}

	out_buffer[COMMP_OPCODE_ZERO_BYTE] = rrq.zero_byte;
	out_buffer[COMMP_OPCODE_BYTE] = rrq.opcode;
	out_buffer[COMMP_RRQ_LENGTH_OFFSET] = (uint8_t)(rrq.length>>24);
	out_buffer[COMMP_RRQ_LENGTH_OFFSET+1] = (uint8_t)(rrq.length>>16);
	out_buffer[COMMP_RRQ_LENGTH_OFFSET+2] = (uint8_t)(rrq.length>>8);
	out_buffer[COMMP_RRQ_LENGTH_OFFSET+3] = (uint8_t)(rrq.length&0xFF);
	memcpy(&out_buffer[COMMP_RRQ_ROMNAME_OFFSET], rrq.romname,
	       size_of_romname);

	int error = comm_protocol_write_data(cdriver, out_buffer, out_len);
	if (!error) {
		LOG_ERROR("Send of RRQ Failed");
	}
	else {
		LOG_DEBUG("_comm_send_rrq - Send %d bytes", out_len);
		LOG_DEBUG("_comm_send_rrq - %x %x %s", rrq.zero_byte, rrq.opcode, rrq.romname);
	}

	free(rrq.romname);
	free(out_buffer);
	return error;
}

int _comm_send_wrq(struct comm_driver_t *cdriver,
                   struct comm_ctxt_t *ctxt)
{
	struct comm_proto_rrq_wrq_t wrq = { 0 };
	wrq.opcode = COMMP_WRQ;
	wrq.romname = (uint8_t *)malloc(sizeof(COMMP_ROMNAME_ROM0));
	if (!wrq.romname) {
		LOG_ERROR("Failed to allocate memory");
		return -1;
	}

	memset(wrq.romname, 0, sizeof(COMMP_ROMNAME_ROM0));
	if (ctxt->part_nr == COMMP_ROMID_ROM0) {
		memcpy(wrq.romname, COMMP_ROMNAME_ROM0, sizeof(COMMP_ROMNAME_ROM0));
	} else if (ctxt->part_nr == COMMP_ROMID_ROM1) {
		memcpy(wrq.romname, COMMP_ROMNAME_ROM1, sizeof(COMMP_ROMNAME_ROM1));
	} else if (ctxt->part_nr == COMMP_ROMID_SPIFLASH0) {
		//memset(wrq.romname, 0, sizeof(COMMP_ROMNAME_SPIFLASH0));
		memcpy(wrq.romname, COMMP_ROMNAME_SPIFLASH0, sizeof(COMMP_ROMNAME_SPIFLASH0));
	} else {
		LOG_ERROR("Invalid APPROM (%d)",ctxt->part_nr);
		free(wrq.romname);
		return -1;
	}

	size_t out_len = sizeof(uint16_t) + sizeof(COMMP_ROMNAME_ROM0);

	/* TODO: Probably we could use a static buffer here */
	uint8_t *out_buffer = malloc(out_len);
	if (!out_buffer) {
		LOG_ERROR("Failed to allocate out buffer");
		free(wrq.romname);
		return -1;
	}

	out_buffer[COMMP_OPCODE_ZERO_BYTE] = wrq.zero_byte;
	out_buffer[COMMP_OPCODE_BYTE] = wrq.opcode;
	memcpy(&out_buffer[COMMP_WRQ_ROMNAME_OFFSET], wrq.romname,
	       sizeof(COMMP_ROMNAME_ROM0));

	int error = comm_protocol_write_data(cdriver, out_buffer, out_len);
	if (!error) {
		LOG_ERROR("Send of WRQ Failed");
	}

	free(wrq.romname);
	free(out_buffer);
	return error;
}

int _comm_send_ack(struct comm_driver_t *cdriver,
									 struct comm_ctxt_t *ctxt)
{
	uint8_t ack_buffer[COMMP_ACK_BUFFER_SIZE] = { 0 };
	comm_protocol_create_ack(ctxt, ack_buffer);
	int err = comm_protocol_write_data(cdriver, ack_buffer,
					   COMMP_ACK_BUFFER_SIZE);
	if (err < 0) {
		LOG_ERROR("Failed to write error");
		/* This needs to send an error */
		_reset_tranfer_ctxt(ctxt);
		return -1;
	}
	return 0;
}

int _comm_send_err(struct comm_driver_t *cdriver, uint16_t errorcode,
		   char *errorstr)
{
	uint8_t out_buffer[COMMP_ERROR_BUFFER_SIZE] = { 0 };
	comm_protocol_create_error(errorcode, errorstr, out_buffer);

	size_t outlen = strlen(errorstr) + 4 + 1; //!< Header size is 4 and +1 for \0
	int error = comm_protocol_write_data(cdriver, out_buffer, outlen);
	if (!error) {
		LOG_ERROR("Send of error Failed");
		return -1;
	}
	return 0;
}
