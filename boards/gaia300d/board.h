/**
 * @file board.h
 * @brief  Gaia300D Board specific code
 * https://wiki.barco.com/display/p900/Gaia+300D+-+Green+Power+MCU
 * @author David Thio <david.thio@barco.com>
 * @version v0.1
 * @date 2022-03-08
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>
#include "fsl_common.h"
#include "LPC55S69_cm33_core0.h"

/**< Board name */
#define BOARD_NAME                     "Gaia300D"
#define BOARD_GAIA                     1

/**< Communication Protocol Address */
#define DEVICE_ADDRESS                 0x10

/**< Hardware ID */
#define HWID                           0

#ifndef BOARD_DEBUG_USART_BAUDRATE
#define BOARD_DEBUG_USART_BAUDRATE     115200U
#endif /* BOARD_DEBUG_USART_BAUDRATE */

#ifndef BOARD_MAINCPU_USART_BAUDRATE
#define BOARD_MAINCPU_USART_BAUDRATE   230400U
#endif /* BOARD_MAINCPU_USART_BAUDRATE */

/**< CPU2 UART Settings */
#ifndef BOARD_CPU2_FLEXCOMM_IRQ
#define BOARD_CPU2_FLEXCOMM_IRQ        FLEXCOMM0_IRQHandler
#endif

#ifndef BOARD_CPU2_USART
#define BOARD_CPU2_USART               USART0
#endif

/**< MainCPU UART Settings */
#ifndef BOARD_MAINCPU_FLEXCOMM_IRQ
#define BOARD_MAINCPU_FLEXCOMM_IRQ     FLEXCOMM1_IRQHandler
#endif

#ifndef BOARD_MAINCPU_USART
#define BOARD_MAINCPU_USART            USART1
#endif

/**< Gowin UART Settings */
#ifndef BOARD_GOWIN_FLEXCOMM_IRQ
#define BOARD_GOWIN_FLEXCOMM_IRQ       FLEXCOMM5_IRQHandler
#endif

#ifndef BOARD_GOWIN_USART
#define BOARD_GOWIN_USART              USART5
#endif

/**< I2C Settings to MainCPU */
#ifndef BOARD_I2C_MAINCPU_FLEXCOMM_IRQ
#define BOARD_I2C_MAINCPU_FLEXCOMM_IRQ FLEXCOMM6_IRQHandler
#endif

#ifndef BOARD_I2C_MAINCPU_USART
#define BOARD_I2C_MAINCPU_USART        USART6
#endif

#ifndef BOARD_I2C_MAINCPU_BASE
#define BOARD_I2C_MAINCPU_BASE         I2C6_BASE
#endif

#ifndef BOARD_I2C_MAINCPU_PERIPHERAL
#define BOARD_I2C_MAINCPU_PERIPHERAL   FLEXCOMM6_PERIPHERAL
#endif

#ifndef BOARD_I2C_MAINCPU_CLK
#define BOARD_I2C_MAINCPU_CLK          kFRO12M_to_FLEXCOMM6
#endif

#ifndef BOARD_I2C_MAINCPU_RST_SHIFT
#define BOARD_I2C_MAINCPU_RST_SHIFT    kFC6_RST_SHIFT_RSTn
#endif

/**< I2C Settings to USB HUB0&1 */
#ifndef BOARD_I2C_USB_FLEXCOMM_IRQ
#define BOARD_I2C_USB_FLEXCOMM_IRQ     FLEXCOMM2_IRQHandler
#endif

#ifndef BOARD_I2C_USB_USART
#define BOARD_I2C_USB_USART            USART2
#endif

#ifndef BOARD_I2C_USB_BASE
#define BOARD_I2C_USB_BASE             I2C2_BASE
#endif

#ifndef BOARD_I2C_USB_PERIPHERAL
#define BOARD_I2C_USB_PERIPHERAL       FLEXCOMM2_PERIPHERAL
#endif

#ifndef BOARD_I2C_USB_CLK
#define BOARD_I2C_USB_CLK              kFRO12M_to_FLEXCOMM2
#endif

#ifndef BOARD_I2C_USB_RST_SHIFT
#define BOARD_I2C_USB_RST_SHIFT        kFC2_RST_SHIFT_RSTn
#endif

/**< SPI */
#ifndef BOARD_SPI_FLEXCOMM_IRQ
#define BOARD_SPI_FLEXCOMM_IRQ         FLEXCOMM8_IRQHandler
#endif

#ifndef BOARD_SPI_USART
#define BOARD_SPI_USB_USART            USART8
#endif

/**
 * @brief  Retrieve the current board revision
 *
 * @returns  Returns the revision number
 */
uint8_t BOARD_GetRevision(void);

/**
 * @brief  Retrieve the board Name
 *
 * @returns  Returns the board Name
 */
const char * BOARD_GetName(void);

/**
 * @brief  Retrieve the board Name size
 *
 * @returns  size of the boardname
 */
uint8_t BOARD_GetNameSize(void);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*
 * @brief Watchdog refresh
 *
 * @param -
 */
void wdog_refresh(void);

/*
 * @brief Enable user interrupts on UART Flexcomm
 *
 * @param -
 */
void enable_user_irq(void);

/*
 * @brief Disable user interrupts on UART Flexcomm
 *
 * @param -
 */
void disable_user_irq(void);

#if defined(SDK_I2C_BASED_COMPONENT_USED) && SDK_I2C_BASED_COMPONENT_USED
void BOARD_I2C_Init(I2C_Type * base, uint32_t clkSrc_Hz);
status_t BOARD_I2C_Send(I2C_Type * base, uint8_t deviceAddress, uint32_t subAddress, uint8_t
                        subaddressSize, uint8_t * txBuff, uint8_t txBuffSize);
status_t BOARD_I2C_Receive(I2C_Type * base, uint8_t deviceAddress, uint32_t subAddress, uint8_t
                           subaddressSize, uint8_t * rxBuff, uint8_t rxBuffSize);
void BOARD_Accel_I2C_Init(void);
status_t BOARD_Accel_I2C_Send(uint8_t deviceAddress, uint32_t subAddress, uint8_t subaddressSize,
                              uint32_t txBuff);
status_t BOARD_Accel_I2C_Receive(uint8_t deviceAddress, uint32_t subAddress, uint8_t subaddressSize,
                                 uint8_t * rxBuff, uint8_t rxBuffSize);
void BOARD_Codec_I2C_Init(void);
status_t BOARD_Codec_I2C_Send(uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize,
                              const uint8_t * txBuff, uint8_t txBuffSize);
status_t BOARD_Codec_I2C_Receive(uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize,
                                 uint8_t * rxBuff, uint8_t rxBuffSize);
#endif  /* SDK_I2C_BASED_COMPONENT_USED */

#endif  /* _BOARD_H_ */
