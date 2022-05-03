/**
 * @file usb5807c.c
 * @brief  Microchip USB5807C device driver
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-28
 */

#include <string.h>

#include "logger.h"

#include "usb5807c.h"
#include "usb5807c_regmap.h"

#include "fsl_gpio.h"

#define I2C_SUBADDR_LEN 2

static uint8_t _usb_i2c_rx_buf[USB5807C_MAX_DATALEN] = { 0 };   //!< I2C Buffer
static uint8_t _usb_i2c_tx_buf[USB5807C_MAX_DATALEN] = { 0 };   //!< I2C Buffer

struct usb5807c_ctxt_t _usb_ctxt = {
	.i2c_handle		= NULL,
	.i2c_transfer		= {
		.slaveAddress	= USB5807C_I2C_ADDR,
		.flags		= kI2C_TransferDefaultFlag,
		.subaddress	= 0,
		.subaddressSize = I2C_SUBADDR_LEN,
		.direction	= 0,
		.data		= NULL,
		.dataSize	= 0,
	},
	.in_runtime		= false,
	.curr_port_status	= 0xff,
};

void usb5807c_init_ctxt(I2C_Type *base, i2c_master_handle_t *handle)
{
	assert(base);
	assert(handle);
	_usb_ctxt.i2c_base = base;
	_usb_ctxt.i2c_handle = handle;
}

/*
 * @brief Write to an internal register of the Hub using I2C
 *
 * @param reg Register to be written
 * @param data Data that will be transferred
 * @param count Amount of bytes in the data buffer
 *
 * @return -1 if failed, otherwise 0
 */
static int _usb5807c_i2c_write(uint16_t reg, uint8_t *data, uint16_t count)
{
	assert(_usb_ctxt.i2c_base);
	assert(_usb_ctxt.i2c_handle);

	_usb_ctxt.i2c_transfer.subaddress = reg;
	_usb_ctxt.i2c_transfer.data = data;
	_usb_ctxt.i2c_transfer.dataSize = count;
	_usb_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
	_usb_ctxt.i2c_transfer.direction = kI2C_Write;

	status_t error =
		I2C_MasterTransferBlocking(_usb_ctxt.i2c_base,
					   &_usb_ctxt.i2c_transfer);

	if (error != kStatus_Success) {
		return -1;
	}
	return 0;
}

/*
 * @brief Read from an internal register of the Hub using I2C
 *
 * @param reg Register to be written
 * @param data Data that will be transferred
 * @param count Amount of bytes in the data buffer
 *
 * @return -1 if failed, otherwise 0
 */
static int _usb5807c_i2c_read(uint16_t reg, uint8_t *data, uint8_t count)
{
	assert(_usb_ctxt.i2c_base);
	assert(_usb_ctxt.i2c_handle);

	_usb_ctxt.i2c_transfer.subaddress = reg;
	_usb_ctxt.i2c_transfer.data = data;
	_usb_ctxt.i2c_transfer.dataSize = count;
	_usb_ctxt.i2c_transfer.direction = kI2C_Read;
	_usb_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;

	memset(data, 0, count); //!< Clean receiving buffer first
	status_t error =
		I2C_MasterTransferBlocking(_usb_ctxt.i2c_base,
					   &_usb_ctxt.i2c_transfer);

	if (error != kStatus_Success) {
		return -1;
	}
	return 0;
}

/**
 * @brief  Run the Write Configuration Register command sequence
 *
 * Write sequence:
 * Transfer Register addr. Count here is length of the data
 * <i2c_addr + W Bit><Mem MSB><Mem LSB><Count><Write><Nr To Write><Reg MSB><Reg LSB><data>
 *
 * Run Configuration Register Access Command
 * <i2c_addr + W Bit><0x99><0x37><0x00>
 *
 * @param reg Register to be written
 * @param data Pointer which holds the data to be written
 * @param count Number of bytes that will be written
 *
 * @returns -1 if failed, otherwise 0
 */
static int _usb5807c_write_config_reg(uint16_t reg, uint8_t *value,
				      uint8_t count)
{
	assert(_usb_ctxt.i2c_base);
	assert(_usb_ctxt.i2c_handle);

	memset(_usb_i2c_tx_buf, 0, USB5807C_MAX_DATALEN);

	/* Setup data buffer for memory transfer */
	_usb_i2c_tx_buf[USB5807C_REG_CONF_CL] =
		(uint8_t)(USB5807C_WRITE_CONF_REG_LEN +
			  count);
	_usb_i2c_tx_buf[USB5807C_REG_CONF_RW] = USB5807C_WRITE_CMD;
	_usb_i2c_tx_buf[USB5807C_REG_CONF_DC] = count;
	_usb_i2c_tx_buf[USB5807C_REG_CONF_RM] =
		(uint8_t)((reg & USB5807C_MSB) >> 8);
	_usb_i2c_tx_buf[USB5807C_REG_CONF_RL] = (uint8_t)(reg & USB5807C_LSB);

	memcpy(&_usb_i2c_tx_buf[USB5807C_REG_CONF_D0], value, count);

	LOG_DEBUG("W 0x%.4x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x  0x%.2x ",
		  reg, _usb_i2c_tx_buf[0], _usb_i2c_tx_buf[1],
		  _usb_i2c_tx_buf[2], _usb_i2c_tx_buf[3],
		  _usb_i2c_tx_buf[4], _usb_i2c_tx_buf[5],
		  _usb_i2c_tx_buf[6]);


	/* Write to data */
	int err = _usb5807c_i2c_write(USB5807C_RAM_START, _usb_i2c_tx_buf,
				      (uint16_t)(USB5807C_WRITE_CONF_REG_LEN +
						 count + 1));

	if (err < 0) {
		return -1;
	}

	/* Run configuration access command */
	err = usb5807c_run_config_reg_access();
	if (err < 0) {
		return -1;
	}

	return 0;
}

/**
 * @brief  Run the Read Configuration Register command sequence
 *
 * Read sequence:
 * Transfer Register addr. Count here is
 * <i2c_addr + W Bit><Mem MSB><Mem LSB><Count><Read><Nr To Read><Reg MSB><Reg LSB>
 *
 * Run Configuration Register Access Command
 * <i2c_addr + W Bit><0x99><0x37><0x00>
 *
 * RAM Addr here is the RAM Start + an offset which is equal to the
 * data transfer count (see above Nr To Read)
 * <i2c_addr + R Bit><RAM Addr MSB><RAM Addr LSB><Count><Data0>
 *
 * @param reg Register to be read
 * @param data Pointer which will hold the return value
 * @param count Number of bytes that will be read
 *
 * @returns -1 if failed, otherwise 0
 */
static int _usb5807c_read_config_reg(uint16_t reg, void *data, uint8_t count)
{
	/* Setup data buffer for memory transfer */
	_usb_i2c_tx_buf[USB5807C_REG_CONF_CL] = USB5807C_READ_CONF_REG_LEN;
	_usb_i2c_tx_buf[USB5807C_REG_CONF_RW] = USB5807C_READ_CMD;
	_usb_i2c_tx_buf[USB5807C_REG_CONF_DC] = count;
	_usb_i2c_tx_buf[USB5807C_REG_CONF_RM] =
		(uint8_t)((reg & USB5807C_MSB) >> 8);
	_usb_i2c_tx_buf[USB5807C_REG_CONF_RL] = (uint8_t)(reg & USB5807C_LSB);

	/* Write to data */
	int err = _usb5807c_i2c_write(USB5807C_RAM_START, _usb_i2c_tx_buf,
				      USB5807C_READ_CONF_REG_LEN + 1);

	if (err < 0) {
		return -1;
	}

	/* Run configuration access command */
	err = usb5807c_run_config_reg_access();
	if (err < 0) {
		return -1;
	}

	/* Read data */
	err = _usb5807c_i2c_read(
		(uint16_t)(USB5807C_RAM_START + USB5807C_RAM_ADDR_OFF + count),
		data, count);
	if (err != kStatus_Success) {
		return -1;
	}

	LOG_DEBUG("R 0x%.4x 0x%.2x 0x%.2x 0x%.2x 0x%.2x ", reg,
		  _usb_i2c_rx_buf[0], _usb_i2c_rx_buf[1],
		  _usb_i2c_rx_buf[2], _usb_i2c_rx_buf[3]);

	return 0;
}

int usb5807c_reset()
{
	_usb_i2c_tx_buf[0] = 0x00; //!< Make sure first byte is 0x00 for termination
	return _usb5807c_i2c_write(USB5807C_CMD_RESET, _usb_i2c_tx_buf, 1);
}

int usb5807c_enable_runtime()
{
	_usb_i2c_tx_buf[0] = 0x00; //!< Make sure first byte is 0x00 for termination
	int err = _usb5807c_i2c_write(USB5807C_CMD_ATTACH, _usb_i2c_tx_buf, 1);

	if (err < 0) {
		_usb_ctxt.in_runtime = false;
		return -1;
	}
	_usb_ctxt.in_runtime = true;
	return err;
}

int usb5807c_enable_runtime_with_smbus()
{
	_usb_i2c_tx_buf[0] = 0x00; //!< Make sure first byte is 0x00 for termination
	int err = _usb5807c_i2c_write(USB5807C_CMD_ATTACH_SMBUS,
				      _usb_i2c_tx_buf,
				      1);

	if (err < 0) {
		_usb_ctxt.in_runtime = false;
		LOG_ERROR("Failed to enable runtime!");
		return -1;
	}
	_usb_ctxt.in_runtime = true;
	return err;
}

int usb5807c_run_config_reg_access()
{
	_usb_i2c_tx_buf[0] = 0x00; //!< Make sure first byte is 0x00 for termination
	return _usb5807c_i2c_write(USB5807C_CMD_CONF_REG_ACCESS,
				   _usb_i2c_tx_buf, 1);
}

int usb5807c_retrieve_revision()
{
	int err = _usb5807c_read_config_reg(USB5807C_DEV_REV,
					    _usb_i2c_rx_buf, 2);

	if (err < 0) {
		return err;
	}

	return _usb_i2c_rx_buf[USB5807C_READ_DATA_OFFSET];
}

int usb5807c_retrieve_id()
{
	int err = _usb5807c_read_config_reg(USB5807C_DEV_ID,
					    _usb_i2c_rx_buf, 2);

	if (err < 0) {
		return err;
	}

	return _usb_i2c_rx_buf[USB5807C_READ_DATA_OFFSET];
}

int usb5807c_retrieve_connect_status()
{
	int err = _usb5807c_read_config_reg(USB5807C_CONNECT_CFG,
					    _usb_i2c_rx_buf, 2);

	if (err < 0) {
		return err;
	}

	return _usb_i2c_rx_buf[USB5807C_READ_DATA_OFFSET];
}

int usb5807c_set_upstream_port(uint8_t port)
{
	return _usb5807c_write_config_reg(USB5807C_CONNECT_CFG,
					  (uint8_t *)&port, 1);
}

int usb5807c_retrieve_usb_vid()
{
	int err = _usb5807c_read_config_reg(USB5807C_USB2_VIDL,
					    _usb_i2c_rx_buf, 2);

	if (err < 0) {
		return err;
	}

	int vidl = _usb_i2c_rx_buf[1];

	err = _usb5807c_read_config_reg(USB5807C_USB2_VIDM,
					_usb_i2c_rx_buf, 2);
	if (err < 0) {
		return err;
	}

	int vidm = _usb_i2c_rx_buf[1];

	return (vidm << 8) | (vidl);
}

int usb5807c_set_usb_vid(uint8_t *usb_vid)
{
	return _usb5807c_write_config_reg(USB5807C_USB2_VIDL, usb_vid, 2);
}

static char *_link_state_to_string(uint8_t state)
{
	switch (state) {
	case 0x00:
		return "normal";
	case 0x01:
		return "sleep";
	case 0x02:
		return "suspend";
	case 0x03:
		return "off";
	default:
		break;
	}
	;
	return "invalid";
}

int usb5807c_is_port_active(uint8_t port)
{
	int err = _usb5807c_read_config_reg(
		(port < 4) ? USB5807C_USB2_LINK_STATE0_3 :
		USB5807C_USB2_LINK_STATE4_7, _usb_i2c_rx_buf,
		2);

	if (err < 0) {
		return err;
	}
	int portstat = _usb_i2c_rx_buf[1];

	/* Temporary debug output... */
	LOG_DEBUG("Port %d: %s", (port < 4) ? 0 : 4,
		  _link_state_to_string(portstat & 0x3));
	LOG_DEBUG("Port %d: %s", (port < 4) ? 1 : 5,
		  _link_state_to_string((portstat & 0xC) >> 2));
	LOG_DEBUG("Port %d: %s", (port < 4) ? 2 : 6,
		  _link_state_to_string((portstat & 0x30) >> 4));
	LOG_DEBUG("Port %d: %s", (port < 4) ? 3 : 7,
		  _link_state_to_string((portstat & 0xC0) >> 6));
	/* End of ugly debug output */

	return portstat;
}

int usb5807c_is_port_enabled(uint8_t port __attribute__((unused)))
{
	int ret = _usb5807c_read_config_reg(USB5807C_PDS, _usb_i2c_rx_buf, 2);
	LOG_DEBUG("PDS: 0x%x, return value: %d", _usb_i2c_rx_buf[1], ret);
	ret = _usb5807c_read_config_reg(USB5807C_PDB, _usb_i2c_rx_buf, 2);
	LOG_DEBUG("PDB: 0x%x, return value: %d", _usb_i2c_rx_buf[1], ret);
}

int usb5807c_disable_port(uint8_t port)
{
	(void)port;
	/* TODO: Fix this... */
	/*This is not correct, Need to use USB5807C_USB2/USB3_PRT_REMAP_X*/
	/* _usb5807c_write_config_reg(USB5807C_PDB, (uint8_t *)&port, 1); */
	/* _usb5807c_write_config_reg(USB5807C_PDS, (uint8_t *)&port, 1); */
	/* _usb_ctxt.curr_port_status &= (uint8_t)(~(port)); */

	return 0;
}
