/**
 * @file board.h
 * @brief  LPCXpresso55S69 Board specific code
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>

#define BOARD_NAME "LPCXpresso55S69" /**< Board name */

/**
 * @brief  Retrieve the current board revision
 *
 * @returns  Returns the revision number
 */
uint8_t BOARD_GetRevision(void);

int BOARD_RunBoardInit(void);

#endif  /* _BOARD_H_ */
