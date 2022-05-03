/**
 * @file peripherals.h
 * @brief  LPCXpresso55S69 Peripheral setup
 *
 * Peripheral configuration for LPCXpresso55S69 board.
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include "fsl_common.h"
#include "fsl_reset.h"
#include "fsl_clock.h"
#include "fsl_usart.h"
#include "fsl_i2c.h"
#include "fsl_crc.h"

#define FLEXCOMM0_PERIPHERAL ((USART_Type *)FLEXCOMM0)  //!< Flexcomm0 peripheral define
#define FLEXCOMM0_CLOCK_SOURCE 12000000UL               //!< Flexcomm0 clockspeed
extern const usart_config_t FLEXCOMM0_config;           //!< Flexcomm0 uart config

#define FLEXCOMM1_PERIPHERAL ((I2C_Type *)FLEXCOMM1)    //!< Flexcomm1 peripheral define
#define FLEXCOMM1_CLOCK_SOURCE 48000000UL               //!< Flexcomm1 clockspeed
extern const i2c_master_config_t FLEXCOMM1_config;      //!< Flexcomm1 i2c config

#define FLEXCOMM2_PERIPHERAL ((USART_Type *)FLEXCOMM2)  //!< Flexcomm2 peripheral define
#define FLEXCOMM2_CLOCK_SOURCE 12000000UL               //!< Flexcomm2 clockspeed
extern const usart_config_t FLEXCOMM2_config;           //!< Flexcomm2 uart config

#define UART_LOGGER FLEXCOMM0_PERIPHERAL                //!< UART 0 is used for logging on this board
#define UART_COMM  FLEXCOMM2_PERIPHERAL                 //!< And UART 2 for communication with main CPU

/**
 * @brief  Initialize the peripherals application side
 */
void BOARD_AppInitPeripherals(void);
void BOARD_AppDeinitPeripherals(void);

/**
 * @brief  Initialize the peripherals bootloader side
 */
void BOARD_BootInitPeripherals(void);
void CRCEngine_init(CRC_Type *base, uint32_t seed);
void BOARD_BootDeinitPeripherals(void);

#endif /* _PERIPHERALS_H_ */
