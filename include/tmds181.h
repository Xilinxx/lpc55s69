/**
 * @file tmds181.h
 * @brief  TMDS181 Driver
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-09-21
 */

#ifndef _GP_DRV_TMDS181_H_
#define _GP_DRV_TMDS181_H_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "fsl_i2c.h"

#define TMDS181_I2C_ADDR   (0x5E)       // !< I2C Slave address
#define TMDS181_PWR_ACTIVE (0x00)       // !< Power status active
#define TMDS181_PWR_STNDBY (0x01)       // !< Power status standby
#define TMDS181_PWR_OFF    (0x02)       // !< Power status off


/**
 * @brief  TMDS181 main context
 */
struct tmds181_ctxt_t {
    I2C_Type * i2c_base;                        // !< I2C Base address
    i2c_master_handle_t * i2c_handle;           // !< I2C Master handle
    i2c_master_transfer_t i2c_transfer;         // !< I2C Transfer handle
};


/**
 * @brief  Init TMDS181 context
 *
 * @param base I2C Base address
 * @param handle I2C Master handle
 */
void tmds181_init_ctxt(I2C_Type * base, i2c_master_handle_t * handle);

/**
 * @brief Read the device ID
 *
 * @returns -1 if failed, otherwise 0
 */
int tmds181_read_dev_id();

/**
 * @brief  Read the current powerstate
 *
 * @returns   -1 if failed, otherwise the powerstate
 */
int tmds181_read_powerstate();

#endif /* _GP_DRV_TMDS181_H_ */
