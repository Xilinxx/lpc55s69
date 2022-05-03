#ifndef _FLASH_HELPER_H_
#define _FLASH_HELPER_H_

/*********************** (C) COPYRIGHT BARCO **********************************
* File Name           : flash_helper.h
* Author              : Barco , David Thio <david.thio@barco.com>
* created             : June 2021
* Description         : Helper functions for accessing/modif bootloadeer
* History:
* 05/07/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/
#include <stdbool.h>
#include "bootloader.h"

/**
 * @brief  init_flash
 *         Flash initialisation, check if the partition info was initialised.
 * @returns void
 */
extern bool init_flash();

/**
 * @brief  switch_boot_partition
 *         when we boot from Appli0 -> Appli1 , Appli1 -> Appli0
 * @returns void
 */
extern app_partition_t switch_boot_partition();


extern struct bootloader_ctxt_t get_bootloader_context();

#endif