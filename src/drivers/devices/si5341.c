#include <string.h>

#include <fsl_gpio.h>
#include "si5341.h"

#include "logger.h"


static struct si5341_ctxt_t _ctxt = {
    .i2c_handle         = NULL,
    .i2c_transfer       = {
        .slaveAddress   = SI5341_I2C_ADDR,
        .flags          = kI2C_TransferDefaultFlag,
        .subaddress     = 0,
        .subaddressSize = I2C_SUBADDR_LEN,
        .direction      = 0,
        .data           = NULL,
        .dataSize       = 0,
    },
    .page               = 0,
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
static int __attribute__((unused)) _si5341_i2c_write(uint16_t reg,
                                                     uint8_t * data,
                                                     uint16_t
                                                     count) {
    assert(_ctxt.i2c_base);
    assert(_ctxt.i2c_handle);

    uint8_t new_page = (uint8_t)((reg & 0xff00) >> 8);
    if (_ctxt.page != new_page) {
        /* LOG_DEBUG("Need to swap page"); */

        _ctxt.i2c_transfer.subaddress = (uint8_t)(0x01);
        _ctxt.i2c_transfer.data = (uint8_t *)&new_page;
        _ctxt.i2c_transfer.dataSize = count;
        _ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
        _ctxt.i2c_transfer.direction = kI2C_Write;

        status_t error =
            I2C_MasterTransferBlocking(_ctxt.i2c_base,
                                       &_ctxt.i2c_transfer);

        if (error != kStatus_Success) {
            LOG_ERROR("Write failed: %x %d", reg, error);
            return -1;
        }
        _ctxt.page = new_page;
    }

    _ctxt.i2c_transfer.subaddress = (uint8_t)(reg & 0x00ff);
    _ctxt.i2c_transfer.data = data;
    _ctxt.i2c_transfer.dataSize = count;
    _ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
    _ctxt.i2c_transfer.direction = kI2C_Write;

    status_t error =
        I2C_MasterTransferBlocking(_ctxt.i2c_base,
                                   &_ctxt.i2c_transfer);

    if (error != kStatus_Success) {
        LOG_ERROR("Write failed: %x %d", reg, error);
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
static int __attribute__((unused)) _si5341_i2c_read(uint16_t reg,
                                                    uint8_t * data,
                                                    uint8_t count) {
    assert(_ctxt.i2c_base);
    assert(_ctxt.i2c_handle);

    uint8_t new_page = (uint8_t)((reg & 0xff00) >> 8);
    if (_ctxt.page != new_page) {
        LOG_DEBUG("Need to swap page");

        _ctxt.i2c_transfer.subaddress = (uint8_t)(0x01);
        _ctxt.i2c_transfer.data = (uint8_t *)&new_page;
        _ctxt.i2c_transfer.dataSize = count;
        _ctxt.i2c_transfer.flags = kI2C_TransferDefaultFlag;
        _ctxt.i2c_transfer.direction = kI2C_Write;

        status_t error =
            I2C_MasterTransferBlocking(_ctxt.i2c_base,
                                       &_ctxt.i2c_transfer);

        if (error != kStatus_Success) {
            LOG_ERROR("Write failed: %x %d", reg, error);
            return -1;
        }
        _ctxt.page = new_page;
    }

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

int si5341_dump_initial_regmap() {
    for (int i = 0; i < SI5341_REVD_REG_CONFIG_NUM_REGS; i++) {
        uint16_t addr = (uint16_t)si5341_revd_registers[i].address;
        uint8_t val = si5341_revd_registers[i].value;

        int err = _si5341_i2c_write(addr, (uint8_t *)&val, 1);
        if (err < 0) {
            LOG_ERROR("Failed to dump regmap value: %d", i);
            /* return -1; */
        }
        if (si5341_revd_registers[i].address == 0x0B4E) {
            SDK_DelayAtLeastUs(300000, 96000000U);
        }
    }
    LOG_DEBUG("Si5341 Regmap dump done!");
    return 0;
}

void si5341_init_ctxt(I2C_Type * base,
                      i2c_master_handle_t * handle) {
    assert(base);
    assert(handle);

    _ctxt.i2c_base = base;
    _ctxt.i2c_handle = handle;

    LOG_INFO("Adding SI5341 device");
}
