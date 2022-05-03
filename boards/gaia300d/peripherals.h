/**
 * @file peripherals.h
 * @brief  Gaia300D Peripheral setup
 * @version v0.1
 * @date 2022-03
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include "fsl_common.h"
#include "fsl_reset.h"
#include "fsl_usart.h"
#include "fsl_clock.h"
#include "fsl_i2c.h"
#include "fsl_crc.h"

/*
 * Flexcomm Interface contains up to nine serial peripherals (Flexcomm Interface 0-7 and Flexcomm Interface 8).
 * Each Flexcomm Interface (except flexcomm 8, which is dedicated for high-speed SPI) can be selected by software
 * to be a USART, SPI, I2C, and I2S interface.
 */

//! 2nd CPU UART
#define FLEXCOMM0_PERIPHERAL ((USART_Type *)FLEXCOMM0)
#define FLEXCOMM0_CLOCK_SOURCE 12000000UL
extern const usart_config_t FLEXCOMM0_config;

//! MainCPU UART
#define FLEXCOMM1_PERIPHERAL ((USART_Type *)FLEXCOMM1)
#define FLEXCOMM1_CLOCK_SOURCE 12000000UL
extern const usart_config_t FLEXCOMM1_config;

//! I2C2 - 400 KHz
#define FLEXCOMM2_PERIPHERAL ((I2C_Type *)FLEXCOMM2)
#define FLEXCOMM2_CLOCK_SOURCE 48000000UL
// extern const i2c_master_config_t FLEXCOMM2_config;

//! Debug UART
#define FLEXCOMM3_PERIPHERAL ((USART_Type *)FLEXCOMM3)
#define FLEXCOMM3_CLOCK_SOURCE 12000000UL
extern const usart_config_t FLEXCOMM3_config;

//! I2C1 - 100 KHz - HUB1
//#define FLEXCOMM4_PERIPHERAL ((I2C_Type *)FLEXCOMM4)
//#define FLEXCOMM4_CLOCK_SOURCE 12000000UL
// extern const i2c_master_config_t FLEXCOMM4_config;

//! Gowin UART
#define FLEXCOMM5_PERIPHERAL ((USART_Type *)FLEXCOMM5)
#define FLEXCOMM5_CLOCK_SOURCE 12000000UL
extern const usart_config_t FLEXCOMM5_config;

// I2C MainCPU
#define FLEXCOMM6_PERIPHERAL ((I2C_Type *)FLEXCOMM6)
#define FLEXCOMM6_CLOCK_SOURCE 12000000UL
// extern const i2c_master_config_t FLEXCOMM6_config;

#define UART_COMM FLEXCOMM1_PERIPHERAL   //!< UART 1 for communication with MainCPU
#define UART_LOGGER FLEXCOMM3_PERIPHERAL //!< UART 3 is used for logging on this board
#define UART_GOWIN FLEXCOMM5_PERIPHERAL  //!< UART 5 for communication with Gowin

/**
 * @brief  Initialize the peripherals
 */
void BOARD_BootInitPeripherals(void);
void BOARD_BootDeinitPeripherals(void);
void BOARD_AppInitPeripherals(void);
void BOARD_AppDeinitPeripherals(void);
void CRCEngine_init(CRC_Type *base, uint32_t seed);

#endif /* _PERIPHERALS_H_ */
