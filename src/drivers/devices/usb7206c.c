/**
 * @file usb7206c.c
 * @brief  Microchip usb7206c device driver
 * @author David Thio <david.thio@barco.com>
 * @version v0.1
 * @date 2022-06-03
 */

/* Datasheet:
 * https://ww1.microchip.com/downloads/aemDocuments/documents/UNG/ApplicationNotes/ApplicationNotes/AN2935-Configuration-of-USB7206-USB7206C-USB7216-USB7216C-USB7252-USB7252C-00002935.pdf
 *
 * Example code..
 * https://github.com/jandros/qmk/blob/d7d6dc35ffe8be6cb2611b032ffd2ea3c825d491/keyboards/system76/launch_1/usb_mux.c
 */

#include <string.h>

#include "logger.h"

#include "usb7206c.h"

#include "fsl_gpio.h"

#define I2C_SUBADDR_LEN 4

static uint8_t _usb_7206_i2c_rx_buf[USB7206C_MAX_DATALEN] = { 0 };   // !< I2C Buffer
static uint8_t _usb_7206_i2c_tx_buf[USB7206C_MAX_DATALEN] = { 0 };   // !< I2C Buffer

struct usb7206c_ctxt_t _usb7206c_ctxt = {
    .i2c_handle         = NULL,
    .i2c_transfer       = {
        .slaveAddress   = USB7206C_I2C_ADDR,
        .flags          = kI2C_TransferDefaultFlag,
        .subaddress     = 0, // Keep zero to not use a subaddress
        .subaddressSize = 0, // I2C_SUBADDR_LEN,
        .direction      = 0,
        .data           = NULL,
        .dataSize       = 0,
    },
    .in_runtime         = false,
    .curr_port_status   = 0xff,
};


void usb7206c_init_ctxt(I2C_Type * base,
                        i2c_master_handle_t * handle,
                        uint8_t i2c_address) {
    assert(base);
    assert(handle);
    _usb7206c_ctxt.i2c_base = base;
    _usb7206c_ctxt.i2c_handle = handle;
    _usb7206c_ctxt.i2c_transfer.slaveAddress = i2c_address;
}

static void _usb7206c_i2c_print_error_status(status_t status,
                                             uint32_t reg) {
    switch (status) {
        case kStatus_I2C_Busy:
            LOG_ERROR("Previous transmission still not finished.");
            break;
        case kStatus_I2C_Timeout:
            LOG_ERROR("Transfer error, wait signal timeout.");
            break;
        case kStatus_I2C_ArbitrationLost:
            LOG_ERROR("Transfer error, arbitration lost.");
            break;
        case kStatus_I2C_Nak:
            LOG_ERROR("Transfer error, receive NAK during transfer.");
            break;
        case kStatus_I2C_Addr_Nak:
            LOG_ERROR("Transfer error, receive NAK during addressing. Reg %x", reg);
            break;
        default:
            break;
    }
}

/*
 * @brief Write to an internal register of the Hub using I2C
 *
 * @param reg Register to be written
 * @param data Data that will be transferred×
 * @param count Amount of bytes in the data buffer
 *
 * @return -1 if failed, otherwise 0
 *
 * static int _usb7206c_i2c_write(uint32_t reg, uint8_t * data, uint16_t count){
 *
 *  assert(_usb7206c_ctxt.i2c_base);
 *  assert(_usb7206c_ctxt.i2c_handle);
 *
 *  uint8_t register_write[9] = {
 *      0x00,                  // Buffer address MSB: always 0
 *      0x00,                  // Buffer address LSB: always 0
 *      ((uint8_t)count) + 6,  // Number of bytes to write to command block buffer area
 *      0x00,                  // Direction: 0 = write (1 = read)
 *      (uint8_t)count,        // Number of bytes to write to register
 *      (uint8_t)(reg >> 24),  // Register address byte 3
 *      (uint8_t)(reg >> 16),  // Register address byte 2
 *      (uint8_t)(reg >> 8),   // Register address byte 1
 *      (uint8_t)(reg >> 0),   // Register address byte 0
 *  };
 *
 *  _usb7206c_ctxt.i2c_transfer.data = register_write;
 *  _usb7206c_ctxt.i2c_transfer.dataSize = sizeof(register_write) + count;
 *  _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
 *  _usb7206c_ctxt.i2c_transfer.direction = kI2C_Write;
 *
 *  status_t status =
 *      I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);
 *
 *  if (status != kStatus_Success) {
 *      _usb7206c_i2c_print_error_status(status, reg);
 *      return -1;
 *  }
 *
 *  return 0;
 * }
 */

/*
 * @brief Read from an internal register of the Hub using I2C
 *
 * @param reg RegisterAddress to be written
 * @param data Data that will be transferred
 * @param count Amount of bytes in the data buffer
 *
 * @return -1 if failed, otherwise 0
 *
 * static int _usb7206c_i2c_read(uint32_t reg, uint8_t * data, uint8_t count){
 *  assert(_usb7206c_ctxt.i2c_base);
 *  assert(_usb7206c_ctxt.i2c_handle);
 *
 *  uint8_t register_read[9] = {
 *      0x00,                                  // Buffer address MSB: always 0
 *      0x00,                                  // Buffer address LSB: always 0
 *      0x06,                                  // Number of bytes to write to command block buffer area
 *      0x01,                                  // Direction: 0 = write, 1 = read
 *      (uint8_t)count,                        // Number of bytes to read from register
 *      (uint8_t)(reg >> 24),                  // Register address byte 3
 *      (uint8_t)(reg >> 16),                  // Register address byte 2
 *      (uint8_t)(reg >> 8),                   // Register address byte 1
 *      (uint8_t)(reg >> 0),                   // Register address byte 0
 *  };
 *  _usb7206c_ctxt.i2c_transfer.data = register_read;
 *  _usb7206c_ctxt.i2c_transfer.dataSize = sizeof(register_read);
 *  _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
 *  _usb7206c_ctxt.i2c_transfer.direction = kI2C_Read;
 *
 *  memset(data, 0, count);     // !< Clean receiving buffer first
 *  status_t status =
 *      I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base,
 *                                 &_usb7206c_ctxt.i2c_transfer);
 *
 *  if (status != kStatus_Success) {
 *      _usb7206c_i2c_print_error_status(status, reg);
 *      return -1;
 *  }
 *  return 0;
 * }
 */

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
static int _usb7206c_write_config_reg(uint32_t reg,
                                      uint8_t * value,
                                      uint8_t count) {
    assert(_usb7206c_ctxt.i2c_base);
    assert(_usb7206c_ctxt.i2c_handle);

    memset(_usb_7206_i2c_tx_buf, 0, USB7206C_MAX_DATALEN);

    /* Setup data buffer for memory transfer */
    _usb_7206_i2c_tx_buf[0] = 0; // Buffer Address MSB
    _usb_7206_i2c_tx_buf[1] = 0; // Buffer Address LSB
    _usb_7206_i2c_tx_buf[2] = (uint8_t)(USB7206C_WRITE_CONF_REG_CMD_BLOCK_LEN + count);
    _usb_7206_i2c_tx_buf[3] = USB7206C_WRITE_CMD;
    _usb_7206_i2c_tx_buf[4] = (uint8_t)count;          // Number of bytes to write to register
    _usb_7206_i2c_tx_buf[5] = (uint8_t)(reg >> 24);     // Register address byte 3
    _usb_7206_i2c_tx_buf[6] = (uint8_t)(reg >> 16);     // Register address byte 2
    _usb_7206_i2c_tx_buf[7] = (uint8_t)(reg >> 8);      // Register address byte 1
    _usb_7206_i2c_tx_buf[8] = (uint8_t)(reg >> 0);      // Register address byte 0
    memcpy(&_usb_7206_i2c_tx_buf[USB7206C_WRITE_CONF_REG_LEN], value, count);

    if (count == 1) {
        LOG_DEBUG("Write Config Reg 0x%.4x [0x%.2x]",
                  reg,
                  _usb_7206_i2c_tx_buf[USB7206C_WRITE_CONF_REG_LEN]);
    } else {
        LOG_DEBUG("Write Config Reg 0x%.4x [0x%.2x 0x%.2x 0x%.2x 0x%.2x],%d",
                  reg,
                  _usb_7206_i2c_tx_buf[USB7206C_WRITE_CONF_REG_LEN],
                  _usb_7206_i2c_tx_buf[USB7206C_WRITE_CONF_REG_LEN + 1],
                  _usb_7206_i2c_tx_buf[USB7206C_WRITE_CONF_REG_LEN + 2],
                  _usb_7206_i2c_tx_buf[USB7206C_WRITE_CONF_REG_LEN + 3],
                  count);
    }

    _usb7206c_ctxt.i2c_transfer.data = _usb_7206_i2c_tx_buf;
    _usb7206c_ctxt.i2c_transfer.dataSize = USB7206C_WRITE_CONF_REG_LEN + count + 1;
    _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _usb7206c_ctxt.i2c_transfer.direction = kI2C_Write;

    /* Write to data */
    status_t status =
        I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);

    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, reg);
        return -1;
    }

    /* Run configuration access command */
    status = usb7206c_run_config_reg_access();
    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, reg);
        return -1;
    }

    return 0;
}

/**
 * @brief  Run the Read Configuration Register command sequence
 *
 * Read sequence:
 * Transfer Register addr. Count here is
 * <i2c_addr + W Bit><Mem MSB><Mem LSB><Count><Read><Read Config=0x01><Nr To Read><Reg 4byte>
 *
 * Run Configuration Register Access Command
 * <i2c_addr + W Bit><0x99><0x37><0x00>
 *
 * RAM Addr here is the RAM Start + an offset which is equal to the
 * data transfer count (see above Nr To Read)
 * <i2c_addr + W Bit><RAM Addr MSB><RAM Addr LSB><Count><Data0>
 *
 * @param reg Register to be read
 * @param data Pointer which will hold the return value
 * @param count Number of bytes that will be read
 *
 * @returns -1 if failed, otherwise 0
 */
static int _usb7206c_read_config_reg(uint32_t reg,
                                     void * data,
                                     uint8_t count) {

    uint8_t register_read[9] = {
        0x00,                  // Buffer address MSB: always 0
        0x00,                  // Buffer address LSB: always 0
        0x06,                  // Number of bytes to write to command block buffer area
        0x01,                  // Direction: 0 = write, 1 = read
        (uint8_t)count,        // Number of bytes to read from register
        (uint8_t)(reg >> 24),  // Register address byte 3
        (uint8_t)(reg >> 16),  // Register address byte 2
        (uint8_t)(reg >> 8),   // Register address byte 1
        (uint8_t)(reg >> 0),   // Register address byte 0
    };

    _usb7206c_ctxt.i2c_transfer.data = register_read;
    _usb7206c_ctxt.i2c_transfer.dataSize = sizeof(register_read);
    _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _usb7206c_ctxt.i2c_transfer.direction = kI2C_Write;

    status_t status =
        I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);

    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, reg);
        return -1;
    }

    /* Run configuration access command */
    status = usb7206c_run_config_reg_access();
    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, reg);
        return -1;
    }

    uint8_t read[2] = {
        0x00,  // Buffer address MSB: always 0
        0x06,  // Buffer address LSB: 6 to skip header
    };

    _usb7206c_ctxt.i2c_transfer.data = read;
    _usb7206c_ctxt.i2c_transfer.dataSize = sizeof(read);
    _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _usb7206c_ctxt.i2c_transfer.direction = kI2C_Write;

    status =
        I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);
    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, reg);
        return -1;
    }


    _usb7206c_ctxt.i2c_transfer.direction = kI2C_Read;
    _usb7206c_ctxt.i2c_transfer.data = data;
    _usb7206c_ctxt.i2c_transfer.dataSize = 8; // max is 8
    memset(data, 0, 8);                 // !< Clean receiving buffer first
    status =
        I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);

    if (status != kStatus_Success) {
        _usb7206c_i2c_print_error_status(status, reg);
        return -1;
    }

    /*
     * LOG_DEBUG("Config Reg 0x%.4X : [0x%.2x,0x%.2x,0x%.2x,0x%.2x,0x%.2x]", reg,
     *        _usb_7206_i2c_rx_buf[0], _usb_7206_i2c_rx_buf[1],
     *        _usb_7206_i2c_rx_buf[2], _usb_7206_i2c_rx_buf[3],
     *        _usb_7206_i2c_rx_buf[4]);
     *
     * LOG_DEBUG("Config Reg 0x%.4X : [0x%.2x,0x%.2x,0x%.2x,0x%.2x]", reg + 4,
     *        _usb_7206_i2c_rx_buf[4], _usb_7206_i2c_rx_buf[5],
     *        _usb_7206_i2c_rx_buf[6], _usb_7206_i2c_rx_buf[7]);
     */

    return 0;
}

// Special Commands USB7206C_CMD_ATTACH
int usb7206c_enable_runtime() {
    _usb_7206_i2c_tx_buf[0] = 0x00; // !< Make sure first byte is 0x00 for termination

    uint8_t data[3] = {
        0xAA,
        0x55,  // !< Go to runtime with SMBus ,  0x55 = without SMBus
        0x00,
    };

    _usb7206c_ctxt.i2c_transfer.data = data;
    _usb7206c_ctxt.i2c_transfer.dataSize = sizeof(data);
    _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _usb7206c_ctxt.i2c_transfer.direction = kI2C_Write;

    status_t status =
        I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);

    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, USB7206C_CMD_ATTACH);
        _usb7206c_ctxt.in_runtime = false;
        LOG_ERROR("Failed to enable runtime!");
        return -1;
    }
    _usb7206c_ctxt.in_runtime = true;
    return status;
}

// Special Commands USB7206C_CMD_ATTACH_SMBUS
int usb7206c_enable_runtime_with_smbus() {
    _usb_7206_i2c_tx_buf[0] = 0x00; // !< Make sure first byte is 0x00 for termination

    uint8_t data[3] = {
        0xAA,
        0x56,  // !< Go to runtime with SMBus ,  0x55 = without SMBus
        0x00,
    };

    _usb7206c_ctxt.i2c_transfer.data = data;
    _usb7206c_ctxt.i2c_transfer.dataSize = sizeof(data);
    _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _usb7206c_ctxt.i2c_transfer.direction = kI2C_Write;

    status_t status =
        I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);

    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, USB7206C_CMD_ATTACH_SMBUS);
        _usb7206c_ctxt.in_runtime = false;
        LOG_ERROR("Failed to enable runtime!");
        return -1;
    }
    _usb7206c_ctxt.in_runtime = true;
    return status;
}

int usb7206c_run_config_reg_access() {

    _usb_7206_i2c_tx_buf[0] = 0x00;     // !< Make sure first byte is 0x00 for termination

    uint8_t register_access[3] = {
        0x99,
        0x37,
        0x00,  // Command completion
    };

    _usb7206c_ctxt.i2c_transfer.data = register_access;
    _usb7206c_ctxt.i2c_transfer.dataSize = sizeof(register_access);
    _usb7206c_ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _usb7206c_ctxt.i2c_transfer.direction = kI2C_Write;

    status_t status =
        I2C_MasterTransferBlocking(_usb7206c_ctxt.i2c_base, &_usb7206c_ctxt.i2c_transfer);

    if (status < 0) {
        _usb7206c_i2c_print_error_status(status, USB7206C_CMD_CONF_REG_ACCESS);
        _usb7206c_ctxt.in_runtime = false;
        LOG_ERROR("Failed to enable runtime!");
        return -1;
    }

    return status;
}

uint32_t usb7206c_retrieve_revision() {
    int err = _usb7206c_read_config_reg(USB7206C_DEV_REV,
                                        _usb_7206_i2c_rx_buf, 4);

    if (err < 0) {
        return (uint32_t)err;
    }
    uint32_t revision = (uint32_t)((_usb_7206_i2c_rx_buf[1] << 0) +
                                   (_usb_7206_i2c_rx_buf[2] << 8) +
                                   (_usb_7206_i2c_rx_buf[3] << 16) +
                                   (_usb_7206_i2c_rx_buf[4] << 24));

    return revision;
}

uint16_t usb7206c_retrieve_id() {
    int err = _usb7206c_read_config_reg(USB7206C_DEV_ID,
                                        _usb_7206_i2c_rx_buf, 2);

    if (err < 0) {
        return (uint16_t)err;
    }
    uint16_t data = (uint16_t)((_usb_7206_i2c_rx_buf[1]) +
                               (_usb_7206_i2c_rx_buf[2] << 8));


    return data;
}

uint32_t usb7206c_retrieve_configuration() {
    // HUB CONFIGURATION DATA - 3 byte
    int err = _usb7206c_read_config_reg(USB7206C_HUB_CFG,
                                        _usb_7206_i2c_rx_buf, 3);

    if (err < 0) {
        return (uint32_t)err;
    }

    uint32_t data = (uint32_t)((_usb_7206_i2c_rx_buf[1] << 0) +
                               (_usb_7206_i2c_rx_buf[2] << 8) +
                               (_usb_7206_i2c_rx_buf[3] << 16) +
                               (_usb_7206_i2c_rx_buf[4] << 24));

    return data;
}

int usb7206c_set_generic_register(uint32_t reg,
                                  uint8_t * value,
                                  uint8_t count) {
    return _usb7206c_write_config_reg(reg, value, count);
}

int usb7206c_get_generic_register(uint32_t reg,
                                  uint8_t * value,
                                  uint8_t count) {
    int ret = _usb7206c_read_config_reg(reg, _usb_7206_i2c_rx_buf, count);

    if ( ret < 0)
        return ret;
    for (int i = 0; i <= count; i++) {
        value[i] = _usb_7206_i2c_rx_buf[i + 1];
    }
    return ret;
}

int usb7206c_set_upstream_port(uint8_t port) {
    return _usb7206c_write_config_reg(
        USB7206C_HUB_CFG,
        (uint8_t *)&port, 1);
}

int usb7206c_set_flex_feature_reg(uint8_t * value) {
    return _usb7206c_write_config_reg(USB7206C_FLEX_FEATURE_REG, value, 1);
}

uint16_t usb7206c_get_flex_feature_reg() {
    _usb7206c_read_config_reg(USB7206C_FLEX_FEATURE_REG,
                              _usb_7206_i2c_rx_buf, 2);
    uint16_t data = (uint16_t)((_usb_7206_i2c_rx_buf[1]) +
                               (_usb_7206_i2c_rx_buf[2] << 8));
    return data;
}

int usb7206c_set_primary_i2c_address_reg(uint8_t * address) {
    return _usb7206c_write_config_reg(USB7206C_SMBUS_PRIMAIRY_ADR, address, 1);
}

uint8_t usb7206c_get_primary_i2c_address_reg() {
    _usb7206c_read_config_reg(USB7206C_SMBUS_PRIMAIRY_ADR, _usb_7206_i2c_rx_buf, 1);
    return _usb_7206_i2c_rx_buf[1];
}

int usb7206c_set_secondary_i2c_address_reg(uint8_t * address) {
    return _usb7206c_write_config_reg(USB7206C_SMBUS_SECOND_ADR, address, 1);
}

int usb7206c_set_i2s_audio_disabled() {
    /*
     * 00h = I2S is disabled.
     * 01h = Audio IN through microphone is enabled.
     * 02h = Audio OUT is enabled.
     * 03h = Both Audio IN are enabled
     */
    return _usb7206c_write_config_reg(USB7206C_REG_I2S_FEAT_SEL, 0x00, 1);
}

int usb7206c_set_vendor_id_reg(uint16_t value) {
    uint8_t data[2];

    data[0] = (uint8_t)value;  // LSB 1st
    data[1] = (uint8_t)(value >> 8);
    return _usb7206c_write_config_reg(
        USB7206C_VENDOR_ID,
        (uint8_t *)&data, 2);
}

uint16_t usb7206c_retrieve_usb_vid() {
    int err = _usb7206c_read_config_reg(
        USB7206C_VENDOR_ID,
        _usb_7206_i2c_rx_buf, 2);

    if (err < 0) {
        return (uint16_t)err;
    }

    uint16_t vid = (uint16_t)(_usb_7206_i2c_rx_buf[2] << 8) + _usb_7206_i2c_rx_buf[1];

    if ( vid != VENDOR_ID_MICROCHIP) {
        LOG_ERROR("Expect MicroChip Vendor ID (=0x%.4X) , got 0x%.4X", VENDOR_ID_MICROCHIP, vid);
    }

    return vid;
}

int usb7206c_set_usb_vid(uint8_t * usb_vid) {
    return _usb7206c_write_config_reg(USB7206C_VENDOR_ID, usb_vid, 2);
}

static char * _link_state_to_string(uint8_t state) {
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

int usb7206c_is_port_active(uint8_t port) {
    int err = _usb7206c_read_config_reg(
        (port < 4) ? USB7206C_USB2_LINK_STATE0_3 :
        USB7206C_USB2_LINK_STATE4_7, _usb_7206_i2c_rx_buf, 2);

    if (err < 0) {
        return err;
    }
    int portstat = _usb_7206_i2c_rx_buf[1];

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

int usb7206c_is_port_enabled(uint8_t port __attribute__((unused))) {
    /*
     * Bit 7 = 1; PHYSICAL Port 7 is disabled.
     * Bit 6 = 1; PHYSICAL Port 6 is disabled.
     * Bit 5 = 1; PHYSICAL Port 5 is disabled.
     * Bit 4 = 1; PHYSICAL Port 4 is disabled.
     * Bit 3 = 1; PHYSICAL Port 3 is disabled.
     * Bit 2 = 1; PHYSICAL Port 2 is disabled.
     * Bit 1 = 1; PHYSICAL Port 1 is disabled.
     * Bit 0 = Reserved, always = ‘0’
     */
    int ret = _usb7206c_read_config_reg(USB7206C_PORT_DIS_SELF, _usb_7206_i2c_rx_buf, 2);

    LOG_DEBUG("Port Disable Self-Powered: 0x%x, return value: %d", _usb_7206_i2c_rx_buf[1], ret);

    ret = _usb7206c_read_config_reg(USB7206C_PORT_DIS_BUS, _usb_7206_i2c_rx_buf, 2);
    LOG_DEBUG("Port Disable Bus-Powered: 0x%x, return value: %d", _usb_7206_i2c_rx_buf[1], ret);

    return ret;
}

int usb7206c_disable_port(uint8_t port) {
    (void)port;
    /* TODO: Fix this... */
    /*This is not correct, Need to use USB7206C_USB2/USB3_PRT_REMAP_X*/
    /* _usb7206c_write_config_reg(USB7206C_PDB, (uint8_t *)&port, 1); */
    /* _usb7206c_write_config_reg(USB7206C_PDS, (uint8_t *)&port, 1); */
    /* _usb7206c_ctxt.curr_port_status &= (uint8_t)(~(port)); */

    return 0;
}
