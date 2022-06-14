/**
 * @file clock_config.h
 * @brief  Clock configuration for Gaia300D board
 * @author David Thio <david.thio@barco.com>
 * @version v0.1
 * @date 2022-03
 */

#ifndef _CLOCK_CONFIG_H_
#define _CLOCK_CONFIG_H_

#include "fsl_common.h"
#include "fsl_wwdt.h"

/*******************************************************************************
 * Definitions for BOARD_BootClockconfiguration
 ******************************************************************************/
#define BOARD_BOOTCLOCKFRO12M_CORE_CLOCK   12000000U    /*!< Core clock frequency: 12000000Hz */
#define BOARD_BOOTCLOCKFROHF96M_CORE_CLOCK 96000000U    /*!< Core clock frequency: 96000000Hz */
#define BOARD_BOOTCLOCKPLL100M_CORE_CLOCK  100000000U   /*!< Core clock frequency: 100000000Hz */
#define BOARD_BOOTCLOCKPLL150M_CORE_CLOCK  150000000U   /*!< Core clock frequency: 150000000Hz */

enum bootClock {
    FRO12,
    FRO96,
    PLL100,
    PLL150
};
/*!
 * @brief Initialize the board clocks application
 */
void BOARD_AppInitClocks(enum bootClock Clk, bool verbose);

/*!
 * @brief Initialize the board clocks bootloader
 */
void BOARD_BootInitClocks(void);

/*******************************************************************************
 * API for BOARD_WWDT_Init... configuration
 ******************************************************************************/
/*!
 * @brief This function initializes the watchdog configuration
 *
 */
void BOARD_WWDT_init(wwdt_config_t * config);

/*!
 * @brief This function uninitializes the watchdog configuration
 *
 */
void BOARD_WWDT_deinit();

/*!
 * @brief This function disables the watchdog
 *
 */
void BOARD_WWDT_disable(wwdt_config_t * config);

/*!
 * @brief This function eanbles the watchdog
 *
 */
void BOARD_WWDT_enable(wwdt_config_t * config);

#endif /* _CLOCK_CONFIG_H_ */
