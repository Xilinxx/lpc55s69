#ifndef _SPI_FLASH_HELPER_H_
#define _SPI_FLASH_HELPER_H_

/*********************** (C) COPYRIGHT BARCO **********************************
 * File Name           : spi_flash_helper.h
 * Author              : Barco , David Thio <david.thio@barco.com>
 * created             : June 2022
 * Description         : Helper functions for accessing spi rom
 * History:
 * 04/07/2022 : introduced in gpmcu code (DAVTH)
 *******************************************************************************/
#include <stdbool.h>
#include "bootloader_spi.h"

/**
 * @brief  init_spi_rom
 *
 * @returns void
 */
extern bool init_spi_rom();

extern struct spi_ctxt_t get_spi_rom_context();

#endif