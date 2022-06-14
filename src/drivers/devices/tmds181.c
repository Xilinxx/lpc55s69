/**
 * @file tmds181.c
 * @brief  TMDS181 Driver
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-09-21
 */

#include <string.h>

#include "logger.h"

#include "tmds181.h"
#include "tmds181_regmap.h"

#define I2C_SUBADDR_LEN 1

struct tmds181_ctxt_t _ctxt = {
    .i2c_handle         = NULL,
    .i2c_transfer       = {
        .slaveAddress   = TMDS181_I2C_ADDR,
        .flags          = kI2C_TransferDefaultFlag,
        .subaddress     = 0,
        .subaddressSize = I2C_SUBADDR_LEN,
        .direction      = 0,
        .data           = NULL,
        .dataSize       = 0,
    },
};

/*
 * @brief Write to an internal register of the Hub using I2C
 *
 * @param reg Register to be written
 * @param data Data that will be transferred
 * @param count Amount of bytes in the data buffer
 *
 * @return -1 if failed, otherwise 0
 */
static int __attribute__((unused)) _tmds181_i2c_write(uint16_t reg,
                                                      uint8_t * data, uint16_t
                                                      count) {
    assert(_ctxt.i2c_base);
    assert(_ctxt.i2c_handle);

    _ctxt.i2c_transfer.subaddress = reg;
    _ctxt.i2c_transfer.data = data;
    _ctxt.i2c_transfer.dataSize = count;
    _ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _ctxt.i2c_transfer.direction = kI2C_Write;

    status_t error =
        I2C_MasterTransferBlocking(_ctxt.i2c_base,
                                   &_ctxt.i2c_transfer);

    if (error != kStatus_Success) {
        LOG_ERROR("Write failed: %d", error);
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
static int _tmds181_i2c_read(uint16_t reg, uint8_t * data, uint8_t count) {
    assert(_ctxt.i2c_base);
    assert(_ctxt.i2c_handle);

    _ctxt.i2c_transfer.subaddress = reg;
    _ctxt.i2c_transfer.data = data;
    _ctxt.i2c_transfer.dataSize = count;
    _ctxt.i2c_transfer.direction = kI2C_Read;
    _ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;

    memset(data, 0, count);     // !< Clean receiving buffer first
    status_t error =
        I2C_MasterTransferBlocking(_ctxt.i2c_base,
                                   &_ctxt.i2c_transfer);

    if (error != kStatus_Success) {
        LOG_ERROR("Read failed: %d", error);
        return -1;
    }
    return 0;
}

void tmds181_init_ctxt(I2C_Type * base, i2c_master_handle_t * handle) {
    assert(base);
    assert(handle);

    _ctxt.i2c_base = base;
    _ctxt.i2c_handle = handle;

    LOG_INFO("Adding TMDS181 device");
}

#define DEV_ID_REV_LEN 7
int tmds181_read_dev_id() {
    uint8_t retval[DEV_ID_REV_LEN + 1] = { 0 };
    int error =
        _tmds181_i2c_read(TMDS181_DEVICE_ID0, retval, DEV_ID_REV_LEN);

    if (error < 0) {
        return -1;
    }
    LOG_DEBUG("Device id: %s", retval);
    return 0;
}

int tmds181_read_powerstate() {
    uint8_t retval;
    int error = _tmds181_i2c_read(TMDS181_RX_PAT_VCSR18, &retval, 1);

    if (error < 0) {
        return -1;
    }
    /* Powerstate 0x0 indicates activity, 0x1 and 0x2
     * 0x1 = Inactive but power'd (standby)
     * 0x2 = Off
     * indicate various levels of off...*/
    LOG_DEBUG("Power Status: %d", (retval & 0xC0) >> 6);
    return retval;
}
