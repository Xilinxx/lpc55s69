#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "comm_protocol.h"
#include "logger.h"

static char fake_wrq_cmd[] = { 0x00, COMMP_WRQ };
static char fake_rrq_cmd[] = { 0x00, COMMP_RRQ, 0x00, 0x00, 0x00, 0x00 }; // offset, length bytes = 0
static char fake_end_cmd[] = { 0x00, COMMP_CMD, 0x00, COMMP_CMD_END };
static char fake_crc_cmd[] = { 0x00, COMMP_CMD, 0x00, COMMP_CMD_CRC, 0x12, 0x23,
			       0x34, 0x45 };

static char fake_ack_cmd[] = { 0x00, COMMP_ACK, 0x00, 0x00 };
static char fake_data_cmd[COMMP_PACKET_SIZE] = { 0x00, COMMP_DATA, 0x00, 0x00 };

int __wrapper_unit_read(uint8_t *buffer, size_t *len)
{
	char *approm = NULL;
	uint16_t cmdcode = 0;
	uint16_t packetnr = 0;
	int magic = (int)mock();
	bool fullpacket = true;

	if (magic == 0) {
		memset(buffer, 0xff, *len);
		return 0;
	}

	switch (magic) {
	case COMMP_WRQ:
		LOG_INFO("Injecting WRQ");
		approm = (char *)mock();
		LOG_DEBUG("Injecting WRQ for %s", approm);
		memcpy(buffer, fake_wrq_cmd, sizeof(fake_wrq_cmd));
		memcpy(&buffer[COMMP_WRQ_ROMNAME_OFFSET], approm, strlen(
			       approm));
		*len = 2 + strlen(approm);
		break;
	case COMMP_RRQ:
		LOG_INFO("Injecting RRQ");
		approm = (char *)mock();
		LOG_DEBUG("Injecting RRQ for %s", approm);
		memcpy(buffer, fake_rrq_cmd, sizeof(fake_rrq_cmd));
		memcpy(&buffer[COMMP_RRQ_ROMNAME_OFFSET], approm, strlen(
			       approm));
		*len = 2 + strlen(approm);
		break;
	case COMMP_ACK:
		LOG_INFO("Injecting ACK");
		packetnr = (uint16_t)mock();
		memcpy(buffer, &fake_data_cmd, COMMP_PACKET_SIZE);
		buffer[COMMP_DATA_PNUMBER_MSB] = (packetnr & 0xff00) >> 8;
		buffer[COMMP_DATA_PNUMBER_LSB] = (packetnr & 0x00ff);
		*len = COMMP_PACKET_SIZE;
		break;
	case COMMP_DATA:
		LOG_INFO("Injecting DATA");
		fullpacket = (bool)mock();
		packetnr = (uint16_t)mock();
		memcpy(buffer, &fake_data_cmd, COMMP_PACKET_SIZE);
		buffer[COMMP_DATA_PNUMBER_MSB] = (packetnr & 0xff00) >> 8;
		buffer[COMMP_DATA_PNUMBER_LSB] = (packetnr & 0x00ff);
		if (fullpacket) {
			*len = COMMP_PACKET_SIZE;
		} else {
			*len = 42;
		}
		break;
	case COMMP_CMD:
		cmdcode = (uint16_t)mock();
		if (cmdcode == COMMP_CMD_END) {
			LOG_INFO("Injecting END CMD");
			memcpy(buffer, &fake_end_cmd, sizeof(fake_end_cmd));
			*len = sizeof(fake_end_cmd);
		} else if (cmdcode == COMMP_CMD_CRC) {
			LOG_INFO("Injecting CRC CMD");
			memcpy(buffer, &fake_crc_cmd, sizeof(fake_crc_cmd));
			*len = sizeof(fake_crc_cmd);
		}
		break;
	}
	;
	return 0;
}

static int _comm_unit_init(void *drv)
{
	return 0;
}

static int _comm_unit_write(void *drv, uint8_t *buffer, size_t len)
{
	return 0;
}

static int _comm_unit_read(void *drv, uint8_t *buffer, size_t *len)
{
	LOG_INFO("Running comm read!");
	memset(buffer, 0, *len);
	return __wrapper_unit_read(buffer, len);
}

static void _comm_unit_close(void *drv)
{
	return;
}

static const struct comm_ops_t _unit_ops = {
	.init	= _comm_unit_init,
	.write	= _comm_unit_write,
	.read	= _comm_unit_read,
	.close	= _comm_unit_close,
};

struct comm_driver_t unit_comm = {
	.enabled	= true,
	.name		= "unit",
	.ops		= &_unit_ops,
	.priv_data	= NULL,
};
