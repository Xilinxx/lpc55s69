#define _FLASH_HELPER_C_
/*********************** (C) COPYRIGHT BARCO **********************************
* File Name           : flash_helper.c
* Author              : Barco , David Thio <david.thio@barco.com>
* created             : June 2021
* Description         : Helper functions for accessing/modif bootloadeer
* History:
* 05/07/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/
#include "flash_helper.h"

#include "logger.h"

#include "storage_flash.h"
#include "storage.h"
#include "board.h"
#include "../bootloader/bootloader_helpers.h"

#include "memory_map.h"


struct flash_area_t bootinfo;
struct flash_area_t approm0;
struct flash_area_t approm1;
struct storage_driver_t *sdriver;

struct bootloader_ctxt_t bctxt;

/** fix_partition()
 * @brief  When a partition is flashed with a programmer the crc stored in
 *         the bootcontext will be wrong. We calculate and update so we
 *         get a valid-partition for the bootloader.
 *         For calculation we use the entire flash area and not the size.
 * @returns  bool succes/failure
 */
bool fix_partition(app_partition_t partition, uint32_t size)
{
  bctxt.apps[partition].application_size = size;
  bctxt.apps[partition].crc = _bootloader_ctxt_get_crc_partition(&bctxt, partition);
  if (bctxt.apps[partition].crc != 0)
  {
    storage_set_flash_area(sdriver, &bootinfo);
    _bootloader_store_ctxt(&bctxt, sdriver);
    LOG_OK("Approm%d CRC fixed",partition);
    _bootloader_print_info(&bctxt);
  }
  else
  {
    LOG_WARN("Approm%d CRC is zero???",partition);
    return false;
  }
  return true;
}

/** verify_partition()
 * @brief  boot context verification takes place here for both partitions
 *         the bctxt global var is fetched here for later use in global data_map
 *
 * @returns  bool succes/failure
 */
bool verify_partition(app_partition_t partition)
{
  bool ret = true;
  storage_set_flash_area(sdriver, &bootinfo);

  if (_bootloader_retrieve_ctxt(&bctxt, sdriver) >= 0)
  {
    //_bootloader_print_info(&bctxt);
    if (partition == APP_PARTITION_0)
      storage_set_flash_area(sdriver, &approm0);
    else if(partition == APP_PARTITION_1)
      storage_set_flash_area(sdriver, &approm1);
    else
    {
      LOG_WARN("Invalid partition selected [%d]",partition);
      return false;
    }

    if (_bootloader_ctxt_validate_partition(&bctxt, partition) != 1)
    {
      if (storage_is_empty_partition(sdriver))
      {
        LOG_DEBUG("Approm%d partition is empty, going to fix that...", partition);
      }
      if (bctxt.part == partition) // only fix own partition
      {
        fix_partition(partition, 0x10000);
      }
    }
  }
  return ret;
}

/** init_flash()
 * @brief  flash acces initialisation and partition verification
 *
 * @returns  bool succes/failure
 */

bool init_flash()
{
  // Fetch partitions info set by bootloader
  bootinfo = storage_new_flash_area(FLASH_HELPER(bootinfo));
  approm0 = storage_new_flash_area(FLASH_HELPER(approm0));
  approm1 = storage_new_flash_area(FLASH_HELPER(approm1));

  bool ret = true;

  sdriver = (struct storage_driver_t *) storage_new_flash_driver();
  if (!sdriver) {
		LOG_ERROR("Flash storage not accessible...");
		return false;
	}

	int err = storage_init_storage(sdriver);
	if (err < 0) {
		LOG_ERROR("Init Flash storage NOK");
		return false;
	}

  bool p1 = verify_partition(APP_PARTITION_0);
  bool p2 = verify_partition(APP_PARTITION_1);
  if (p1 | p2)
  {
   LOG_OK("bootloader fetched succesfully");
  }
  else
  {
    LOG_WARN("bootloader info invalid");
    ret = false;
  }

  LOG_OK("Succesfully read bootcontext from flash");
  return ret;
}

/** struct bootloader_ctxt_t get_bootloader_context()
()
 * @brief  context getter
 *
 * @returns  bootloader context
 */
struct bootloader_ctxt_t get_bootloader_context()
{
  return bctxt;
}

/** switch_boot_partition()
 * @brief  Select which application partition should boot (0 or 1)
 * Will modify bootcode-structure from 1kb of reserved data 0x97c00 - 0x98000
 * and write back the changes
 * @returns  the selected partition
 */
app_partition_t switch_boot_partition()
{
  storage_set_flash_area(sdriver, &bootinfo);
  LOG_DEBUG("Swapping partition in bootloader context");
  _bootloader_swap_partitions(&bctxt);
  // recalculation of bootcode crc takes place here.
  _bootloader_store_ctxt(&bctxt, sdriver);
  _bootloader_print_info(&bctxt);
  return bctxt.part;
}
