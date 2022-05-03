/**
 * @file pin_mux.h
 * @brief  LPCXpresso55S69 Pin Mux configuration
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

#define IOCON_PIO_DIGITAL_EN 0x0100u    /*!<@brief Enables digital function */
#define IOCON_PIO_FUNC1 0x01u           /*!<@brief Selects pin function 1 */
#define IOCON_PIO_FUNC5 0x05u           /*!<@brief Selects pin function 5 */
#define IOCON_PIO_INV_DI 0x00u          /*!<@brief Input function is not inverted */
#define IOCON_PIO_MODE_INACT 0x00u      /*!<@brief No addition pin function */
#define IOCON_PIO_OPENDRAIN_DI 0x00u    /*!<@brief Open drain is disabled */
#define IOCON_PIO_SLEW_STANDARD 0x00u   /*!<@brief Standard mode, output slew rate control is enabled */

/*
 * @brief Calls initialization functions application
 */
void BOARD_AppInitBootPins(void);

/*
 * @brief Calls initialization functions bootloader
 */
void BOARD_BootInitBootPins(void);

/**
 * @brief  USART0 Setup interface
 */
void BOARD_InitDebugUART(void);

/*
 * @brief I2C Master interface.
 */
void BOARD_InitI2CMASTER(void);

#endif /* _PIN_MUX_H_ */
