/**
 * @file si5341.h
 * @brief  SI5341 Driver
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-09-21
 */

#ifndef _GP_DRV_SI5341_H_
#define _GP_DRV_SI5341_H_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "fsl_i2c.h"

#include "si5341_regmap.h"

/**
 * @brief  SI5341 main context
 */
struct si5341_ctxt_t {
    I2C_Type * i2c_base;                        // !< I2C Base address
    i2c_master_handle_t * i2c_handle;           // !< I2C Master handle
    i2c_master_transfer_t i2c_transfer;         // !< I2C Transfer handle
    uint8_t page;
};

/**
 * @brief  Init SI5341 context
 *
 * @param base I2C Base address
 * @param handle I2C Master handle
 */
void si5341_init_ctxt(I2C_Type * base, i2c_master_handle_t * handle);

int si5341_dump_initial_regmap();

#endif /* _GP_DR_SI5341_H_ */
