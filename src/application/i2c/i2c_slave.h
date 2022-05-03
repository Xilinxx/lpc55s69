#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#undef EXTERN
#ifdef _I2C_SLAVE_C_
#define EXTERN
#else
#define EXTERN extern
#endif

/*
 * @brief i2c_slave_setup
 * setup and initialize Flexomm4 and Flexcomm5
 * @return void
 */
int i2c_slave_setup();

/*
 * @brief i2c_slave_disable
 * disable i2c slave
 * @return void
 */
void i2c_slave_disable();

/*
 * @brief i2c_update
 * update
 * @return void
 */
void i2c_update();

#endif //__I2C_SLAVE_H__
