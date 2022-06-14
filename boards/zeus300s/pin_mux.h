/**
 * @file pin_mux.h
 * @brief  LPCXpresso55S69 Pin Mux configuration
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 * @date 2022-01-10 | davth | added gowin control pins
 */

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

#define IOCON_PIO_DIGITAL_EN         0x0100u /*!<@brief Enables digital function */
#define IOCON_PIO_FUNC0              0x00u /*!<@brief Selects pin function 0 */
#define IOCON_PIO_FUNC1              0x01u /*!<@brief Selects pin function 1 */
#define IOCON_PIO_FUNC3              0x03u /*!<@brief Selects pin function 3 */
#define IOCON_PIO_FUNC4              0x04u /*!<@brief Selects pin function 4 */
#define IOCON_PIO_FUNC5              0x05u /*!<@brief Selects pin function 5 */
#define IOCON_PIO_FUNC6              0x06u /*!<@brief Selects pin function 6 */
#define IOCON_PIO_FUNC9              0x09u /*!<@brief Selects pin function 9 */
#define IOCON_PIO_INV_DI             0x00u /*!<@brief Input function is not inverted */
#define IOCON_PIO_MODE_INACT         0x00u /*!<@brief No addition pin function */
#define IOCON_PIO_MODE_PULLUP        0x20u /*!<@brief Selects pull-up function */
#define IOCON_PIO_OPENDRAIN_DI       0x00u /*!<@brief Open drain is disabled */
#define IOCON_PIO_SLEW_STANDARD      0x00u /*!<@brief Standard mode, output slew rate control is enabled */
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_2_DIGIMODE_DIGITAL      0x01u
/*!
 * @brief Selects pin function.: Alternative connection 1. */
#define PIO0_2_FUNC_ALT1             0x01u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_3_DIGIMODE_DIGITAL      0x01u
/*!
 * @brief Selects pin function.: Alternative connection 1. */
#define PIO0_3_FUNC_ALT1             0x01u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_13_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 5. */
#define PIO0_13_FUNC_ALT5            0x05u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_14_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 6. */
#define PIO0_14_FUNC_ALT6            0x06u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_29_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 1. */
#define PIO0_29_FUNC_ALT1            0x01u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_30_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 1. */
#define PIO0_30_FUNC_ALT1            0x01u

/*! @name PIO0_29 (number 92), P8[2]/U6[13]/FC0_USART_RXD
 * @{ */
#define BOARD_DEBUG_UART_RX_PORT     0U          /*!<@brief PORT peripheral base pointer */
#define BOARD_DEBUG_UART_RX_PIN      29U         /*!<@brief PORT pin number */
#define BOARD_DEBUG_UART_RX_PIN_MASK (1U << 29U) /*!<@brief PORT pin mask */
                                                 /* @} */

/*! @name PIO0_30 (number 94), P8[3]/U6[12]/FC0_USART_TXD
 * @{ */
#define BOARD_DEBUG_UART_TX_PORT     0U          /*!<@brief PORT peripheral base pointer */
#define BOARD_DEBUG_UART_TX_PIN      30U         /*!<@brief PORT pin number */
#define BOARD_DEBUG_UART_TX_PIN_MASK (1U << 30U) /*!<@brief PORT pin mask */

/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_13_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 5. */
#define PIO0_13_FUNC_ALT5            0x05u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_14_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 6. */
#define PIO0_14_FUNC_ALT6            0x06u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_9_DIGIMODE_DIGITAL      0x01u
/*!
 * @brief Selects pin function.: Alternative connection 3. */
#define PIO0_9_FUNC_ALT3             0x03u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_14_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 4. */
#define PIO1_14_FUNC_ALT4            0x04u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_9_DIGIMODE_DIGITAL      0x01u
/*!
 * @brief Selects pin function.: Alternative connection 5. */
#define PIO1_9_FUNC_ALT5             0x05u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_16_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 1. */
#define PIO0_16_FUNC_ALT1            0x01u

/************************* FlexComm8 as SPI ***********************************/
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_20_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO0_26_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_2_DIGIMODE_DIGITAL      0x01u
/*!
 * @brief Selects pin function.: Alternative connection 6. */
#define PIO1_2_FUNC_ALT6             0x06u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_3_DIGIMODE_DIGITAL      0x01u
/*!
 * @brief Selects pin function.: Alternative connection 6. */
#define PIO1_3_FUNC_ALT6             0x06u

/* MUX Select for SPI on Flexcomm8 */
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_31_DIGIMODE_DIGITAL     0x01u
/*!
 * @brief Selects pin function.: Alternative connection 0. */
#define PIO1_31_FUNC_ALT0            0x00u

/* for future use: MClk for Gowin , pio1_31 mux -> pio1_29 */
/*!
 * @brief MCLK control.: output mode. */
#define MCLKIO_MCLKIO_OUTPUT         0x01u
/*!
 * @brief Selects pin function.: Alternative connection 1. */
#define PIO1_31_FUNC_ALT1            0x01u
/*! @name PIO1_31 (number 91), MCLK
 * @{ */
#define BOARD_INITPINS_MCLK_PORT     1U          /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_MCLK_PIN      31U         /*!<@brief PORT pin number */
#define BOARD_INITPINS_MCLK_PIN_MASK (1U << 31U) /*!<@brief PORT pin mask */
                                                 /* @} */

/*
 * @brief Calls initialization functions for application-code
 */
void BOARD_AppInitBootPins(void);

/*
 * @brief Calls initialization functions bootloader-code
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

/*
 * @brief fpga boot-mode pins log
 */
uint8_t BOARD_read_fgpa_boot_mode(void);

/*
 * @brief Read Signal for Gowin-READY
 *        GPIO 1, 13 / GOWIN READY
 */
bool BOARD_Ready_Gowin();

/*
 * @brief Reconfigure-sequence Signal for Gowin.
 *        GPIO 1, 15 / GPIO_RECONFIGURE_GOWIN
 * @param delayTime_us  Delay time in unit of microsecond. Min is 200ms
 */
void BOARD_Reconfigure_Gowin(uint32_t delayTime_us);

/*
 * @brief Power Up-sequence Signal for Gowin.
 *        GPIO 1, 17 / GPIO_TEST_PIN2
 * @param delayTime_us  Delay time in unit of microsecond. Min is 200ms
 */
void BOARD_PwrOn_MainCPU(uint32_t delayTime_us);

/*
 * @brief Power Down-sequence Signal for Gowin.
 *        GPIO 1, 16 / GPIO_TEST_PIN1
 * @param delayTime_us  Delay time in unit of microsecond. Min is 200ms
 */
void BOARD_PwrOff_MainCPU(uint32_t delayTime_us);


/*
 * @brief USB Reset Signal
 *        GPIO 1, 23
 * @param set: 1 or 0
 */
void BOARD_Set_USB_Reset(uint8_t set);

/*
 * @brief Mux Selection for SPI access to SPI Flash
 *        GPIO 1, 21
 * @param 0 = Gowin Access  , 1 = GP Access
 */
void BOARD_SetSPIMux(uint8_t on);

/* @brief Setup the GowinClk
 * For future use - Schematic change  16Mhz MClk for Gowin on 96Mhz Clk
 * https://itrack.barco.com/browse/MI0604-271
 * pinmux 1.31 -> 1.29
 */
void BOARD_SetGowinClk();

#endif /* _PIN_MUX_H_ */
