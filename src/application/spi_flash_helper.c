#define _SPI_FLASH_HELPER_C_
/*********************** (C) COPYRIGHT BARCO **********************************
 * File Name           : spi_flash_helper.c
 * Author              : Barco , David Thio <david.thio@barco.com>
 * created             : June 2022
 * Description         : Helper functions for accessing spi rom
 * History:
 * 05/07/2022 : introduced in gpmcu code (DAVTH)
 *******************************************************************************/
#include "spi_flash_helper.h"

#include "logger.h"

#include "storage_spi_flash.h"
#include "storage.h"
#include "board.h"
#include "../bootloader/bootloader_spi_helpers.h"

#include "memory_map.h"


struct spi_ctxt_t spictxt;

/** init_spi_rom()
 * @brief  flash acces initialisation and partition verification
 *
 * @returns  bool succes/failure
 */

bool init_spi_rom() {
    // Fetch partitions info set by bootloader
    struct spi_flash_area_t spiflash0 = storage_new_spi_flash_area("spi0", SPI0_START_ADDR,
                                                                   SPI_PART_SIZE);
    struct storage_driver_t * sdriver = storage_new_spi_flash_driver();

    bool ret = true;

    if (!sdriver) {
        LOG_ERROR("Spi rom storage not accessible...");
        return false;
    }

    int err = storage_init_storage(sdriver);
    if (err < 0) {
        LOG_ERROR("Init spi rom NOK");
        return false;
    }

    storage_set_spi_flash_area(sdriver, &spiflash0);
    _bootloader_spi_retrieve_ctxt(&spictxt, sdriver);

    LOG_OK("Succesfully read spi context from flash");
    return ret;
}

/** struct spi_ctxt_t get_bootloader_context()
 * ()
 * @brief  context getter
 *
 * @returns  spi rom context
 */
struct spi_ctxt_t get_spi_rom_context() {
    return spictxt;
}

