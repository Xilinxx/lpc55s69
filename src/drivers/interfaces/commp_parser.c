/**
 * @file comm_parser.c
 * @brief  COMMP Parser
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-27
 */

#include "comm_protocol.h"
#include "comm_parser.h"
#include "logger.h"

#ifdef UNIT_TEST //!< Allow unit testing of static functions
#define static
#endif

typedef int (*packet_handler_t)(void *priv, uint8_t *data, size_t size);

static int _default_phandler(void *priv, uint8_t *data, size_t size)
{
	(void)priv;
	(void)data;
	(void)size;

	LOG_WARN("Unknown packet received with size [%d]",size);
	if (size)
		LOG_DEBUG("First 2 bytes: %d %d", data[0], data[1]);

	return -1;
}

static int _rrq_phandler(void *priv, uint8_t *data, size_t size)
{
	struct comm_ctxt_t *ctxt = (struct comm_ctxt_t *)priv;
	if (!ctxt) {
		LOG_ERROR("Invalid context");
		return -1;
	}

	if (ctxt->transfer_in_progress) {
		LOG_ERROR("Transfer already in progress, aborting..");
		return -1;
	}

	if (!data) {
		LOG_ERROR("Invalid data ptr");
		return -1;
	}

	if (size == 0 || size > COMMP_PACKET_SIZE) {
		LOG_ERROR("Invalid packet size");
		return -1;
	}

	struct comm_proto_rrq_wrq_t rrq = {
		.opcode		= data[COMMP_OPCODE_BYTE],
		.length	= (uint32_t)(data[COMMP_RRQ_LENGTH_OFFSET] << 24
				      | data[COMMP_RRQ_LENGTH_OFFSET+1]) <<16
		          | (uint16_t)(data[COMMP_RRQ_LENGTH_OFFSET+2] << 8
				      | data[COMMP_RRQ_LENGTH_OFFSET+3]),
		.romname	= &data[COMMP_RRQ_ROMNAME_OFFSET],
	};

	if (rrq.opcode != COMMP_RRQ) {
		LOG_ERROR("Invalid packet, this should never happen. Fix the bug..");
		return -1;
	}

	ctxt->rom_readsize = rrq.length;

	if (!strcmp((char *)rrq.romname, COMMP_ROMNAME_ROM0)) {
		ctxt->part_nr = COMMP_ROMID_ROM0;
	} else if (!strcmp((char *)rrq.romname, COMMP_ROMNAME_ROM1)) {
		ctxt->part_nr = COMMP_ROMID_ROM1;
	} else if (!strcmp((char *)rrq.romname, COMMP_ROMNAME_SPIFLASH0)) {
		ctxt->part_nr = COMMP_ROMID_SPIFLASH0;
	}
	else {
		LOG_ERROR("invalid romname %s",rrq.romname);
		return -1;
	}
	LOG_DEBUG("Read request received for %s length(0x%x)",
		rrq.romname, rrq.length );

	return 0;
}

static int _wrq_phandler(void *priv, uint8_t *data, size_t size)
{
	struct comm_ctxt_t *ctxt = (struct comm_ctxt_t *)priv;
	if (!ctxt) {
		LOG_ERROR("Invalid context");
		return -1;
	}

	if (ctxt->transfer_in_progress) {
		LOG_ERROR("Transfer already in progress, aborting..");
		return -1;
	}

	/* Use asserts */
	if (!data) {
		LOG_ERROR("Invalid data ptr");
		return -1;
	}

	if (size == 0 || size > COMMP_PACKET_SIZE) {
		LOG_ERROR("Invalid packet size");
		return -1;
	}

	struct comm_proto_rrq_wrq_t wrq = {
		.opcode		= data[COMMP_OPCODE_BYTE],
		.romname	= &data[COMMP_WRQ_ROMNAME_OFFSET],
	};

	if (wrq.opcode != COMMP_WRQ) {
		LOG_ERROR("Invalid packet, this should never happen. Fix the bug..");
		return -1;
	}

	LOG_INFO("Write request received for opcode[%d] name[%s]", wrq.opcode, wrq.romname);
	bool erase = true;
	if (!strcmp((char *)wrq.romname, COMMP_ROMNAME_ROM0)) {
		ctxt->part_nr = COMMP_ROMID_ROM0;
	}
	else if (!strcmp((char *)wrq.romname, COMMP_ROMNAME_ROM1)) {
		ctxt->part_nr = COMMP_ROMID_ROM1;
	}
	else if (!strcmp((char *)wrq.romname, COMMP_ROMNAME_SPIFLASH0)) {
		ctxt->part_nr = COMMP_ROMID_SPIFLASH0;
		erase = false; // no erase for starters - debug
	}
	else {
		LOG_ERROR("invalid romname [%s]",wrq.romname);
		return -1;
	}

	if (ctxt->parent && ctxt->parent->sdriver && erase) {
		storage_erase_storage(ctxt->parent->sdriver);
	}
	ctxt->transfer_in_progress = true;

	return 0;
}

static int _data_phandler(void *priv, uint8_t *data, size_t size)
{
	struct comm_ctxt_t *ctxt = (struct comm_ctxt_t *)priv;

	/* Use asserts instead... */
	if (!ctxt) {
		LOG_ERROR("Invalid context");
		return -1;
	}
	if (!data) {
		LOG_ERROR("Invalid data ptr");
		return -1;
	}

	if (size == 0 || size > COMMP_PACKET_SIZE) {
		LOG_ERROR("Invalid packet size");
		return -1;
	}

	if (!ctxt->transfer_in_progress) {
		LOG_ERROR("No transfer in progress");
		return -1;
	}

	struct comm_proto_data_t datap = {
		.opcode		= data[COMMP_OPCODE_BYTE],
		.blocknr	= (uint16_t)(data[COMMP_DATA_PNUMBER_MSB] << 8
				      | data[COMMP_DATA_PNUMBER_LSB]),
		.dataptr	= &data[COMMP_DATA_OFFSET],
	};
	LOG_DEBUG("Data len: %d", size);

	if (datap.opcode != COMMP_DATA) {
		LOG_ERROR(
			"Invalid packet, this should never happen. Fix the bug..");
		return -1;
	}

	uint16_t expected_blk = (uint16_t)(ctxt->last_blocknr + 1U);
	if (expected_blk != datap.blocknr) {
		LOG_ERROR("Invalid packet sequence. received %d, expected %d",
			  expected_blk, datap.blocknr);
		return -1;
	}
	LOG_DEBUG("Data block received nr: %d", datap.blocknr);
	ctxt->last_blocknr = datap.blocknr;

	if (ctxt->parent && ctxt->parent->sdriver) {
		return storage_write_data(ctxt->parent->sdriver, datap.dataptr,
					  size - COMMP_DATA_OFFSET);
	}
	return 0;
}

static int _ack_phandler(void *priv, uint8_t *data, size_t size)
{
	struct comm_ctxt_t *ctxt = (struct comm_ctxt_t *)priv;

	if (!ctxt) {
		LOG_ERROR("Invalid context");
		return -1;
	}

	if (!data) {
		LOG_ERROR("Invalid data ptr");
		return -1;
	}

	if (size == 0 || size > COMMP_PACKET_SIZE) {
		LOG_ERROR("Invalid packet size");
		return -1;
	}

	if (!ctxt->transfer_in_progress) {
		LOG_ERROR("No transfer in progress");
		return -1;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
	uint16_t recvd_blk_nr = (uint16_t)(data[COMMP_ACK_PNUMBER_MSB] << 8);
	recvd_blk_nr |= (uint16_t)data[COMMP_ACK_PNUMBER_LSB];
#pragma GCC diagnostic pop

	//LOG_DEBUG("Ack packet received: %d last send : %d", recvd_blk_nr,
	//	  ctxt->last_blocknr);
	if (recvd_blk_nr != ctxt->last_blocknr) {
		LOG_ERROR("Acknowledged wrong packet... %d<>%d", recvd_blk_nr,
			ctxt->last_blocknr);
		return -1;
	}

	return 0;
}

static int _err_phandler(void *priv, uint8_t *data, size_t size)
{
	(void)priv;

	if (!data) {
		LOG_ERROR("Invalid data ptr");
		return -1;
	}
	if (size == 0 || size > COMMP_PACKET_SIZE) {
		LOG_ERROR("Invalid packet size");
		return -1;
	}

	struct comm_proto_err_t err = {
		.opcode		= data[COMMP_OPCODE_BYTE],
		.errcode	= (uint16_t)(data[COMMP_ERROR_ERRCODE_MSB] << 8
				      | data[COMMP_ERROR_ERRCODE_LSB]),
		.errstr		= &data[COMMP_ERROR_ERRSTR_OFFSET],
	};

	if (err.opcode != COMMP_ERR) {
		LOG_ERROR("Invalid error code");
		return -1;
	}

	bool found = false;
	for (uint8_t availableerr = 0; availableerr < COMMP_NR_OF_COMMANDS;
	     availableerr++) {
		if (err.errcode == availableerr) {
			found = true;
			break;
		}
	}
	if (!found) {
		LOG_ERROR("Invalid command code");
		return -1;
	}

	LOG_DEBUG("Error packet received %d, %s", err.errcode, err.errstr);

	return -1; //!< TODO: Fix this later. Returns -1 for unit tests for now
}

static int _cmd_phandler(void *priv, uint8_t *data, size_t size)
{
	(void)priv;

	if (!data) {
		LOG_ERROR("Invalid data ptr");
		return -1;
	}

	if (size == 0 || size > COMMP_PACKET_SIZE) {
		LOG_ERROR("Invalid packet size");
		return -1;
	}

	LOG_INFO("Command packet received");

	struct comm_proto_cmd_t cmd = {
		.opcode		= data[COMMP_OPCODE_BYTE],
		.cmdcode	= (uint16_t)(data[COMMP_CMD_CMDCODE_MSB] << 8
				      | data[COMMP_CMD_CMDCODE_LSB]),
		.data		= &data[COMMP_CMD_DATA_OFFSET],
	};

	bool found = false;
	for (uint8_t availablecmds = 0; availablecmds < COMMP_NR_OF_COMMANDS;
	     availablecmds++) {
		if (cmd.cmdcode == availablecmds) {
			found = true;
			break;
		}
	}

	if (found) {
		LOG_INFO("Command code: %d", cmd.cmdcode);
	}
	else {
		LOG_ERROR("Invalid command code 0x%x", cmd.cmdcode);
		return -1;
	}

	if (comm_protocol_is_command_type(data, COMMP_CMD_CRC)) {
		uint32_t crc32 = (uint32_t)((data[COMMP_CMD_CRC_BYTE0] << 24) |
					    (data[COMMP_CMD_CRC_BYTE1] << 16) |
					    (data[COMMP_CMD_CRC_BYTE2] << 8) |
					    data[COMMP_CMD_CRC_BYTE3]);
		LOG_DEBUG("Received CRC32: 0x%X", crc32);
	}

	return 0;
}

void comm_protocol_create_ack(struct comm_ctxt_t *	transfer_ctxt,
			      uint8_t *	buffer)
{
	buffer[COMMP_OPCODE_ZERO_BYTE] = 0x00;
	buffer[COMMP_OPCODE_BYTE] = COMMP_ACK;
	buffer[COMMP_ACK_PNUMBER_MSB] = (uint8_t)((transfer_ctxt->last_blocknr &
						   0xff00) >> 8);
	buffer[COMMP_ACK_PNUMBER_LSB] = (uint8_t)(transfer_ctxt->last_blocknr &
						  0x00ff);
	//LOG_DEBUG("Sending ack for block %d", transfer_ctxt->last_blocknr);
	return;
}

void comm_protocol_create_error(uint16_t errorcode, char *errstr,
				uint8_t * buffer)
{
	buffer[COMMP_OPCODE_ZERO_BYTE] = 0x00;
	buffer[COMMP_OPCODE_BYTE] = COMMP_ERR;
	buffer[COMMP_ERROR_ERRCODE_MSB] = (uint8_t)((errorcode & 0xff00) >> 8);
	buffer[COMMP_ERROR_ERRCODE_LSB] = (uint8_t)(errorcode & 0x00ff);
	strncpy((char *)&buffer[COMMP_ERROR_ERRSTR_OFFSET], errstr,
		COMMP_ERROR_MAX_STRING);
	LOG_DEBUG("Sending error %d", errorcode);
}

const packet_handler_t _phandlers[] = {
	_default_phandler,
	_rrq_phandler,
	_wrq_phandler,
	_data_phandler,
	_ack_phandler,
	_err_phandler,
	_cmd_phandler,
	_default_phandler, //!< Number of packets
};

int comm_protocol_parse_packet(struct comm_ctxt_t *transfer_ctxt,
			       uint8_t *buffer, size_t len)
{
	if (!buffer) {
		LOG_ERROR("Invalid buffer pointer");
		return -1;
	}

	if (buffer[COMMP_OPCODE_BYTE] > COMMP_NR_OF_OPCODES) {
		LOG_ERROR("Invalid OPCODE");
		return -1;
	}
	return _phandlers[buffer[COMMP_OPCODE_BYTE]]((void *)transfer_ctxt,
						     buffer, len);
}
