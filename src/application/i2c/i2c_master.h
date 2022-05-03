#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#include <stdint.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#undef EXTERN
#ifdef _I2C_MASTER_C_
  #define EXTERN
#else
  #define EXTERN extern
#endif

int i2c_master_setup();
int i2c_master_initialize_usb(void);

#endif  //__I2C_MASTER_H__
