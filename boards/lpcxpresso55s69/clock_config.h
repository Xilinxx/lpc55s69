/**
 * @file clock_config.h
 * @brief  Clock configuration for LPCXpresso55S69 board
 *
 * Clock configuration for LPCXpresso55S69 board.
 *
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */

#ifndef _CLOCK_CONFIG_H_
#define _CLOCK_CONFIG_H_

#include "fsl_common.h"

#define BOARD_XTAL32K_CLK_HZ               32768U       /**< Board xtal32K frequency in Hz */
#define BOARD_BOOTCLOCKRUN_CORE_CLOCK      150000000U   /**< Core clock frequency: 150000000Hz */
#define BOARD_BOOTCLOCKFROHF96M_CORE_CLOCK 96000000U    /**< FROHF96M clock frequency */

/**
 * @brief Initialize the board clocks application
 */
void BOARD_AppInitClocks(void);

/**
 * @brief Initialize the board clocks bootloader
 */
void BOARD_BootInitClocks(void);

#endif /* _CLOCK_CONFIG_H_ */
