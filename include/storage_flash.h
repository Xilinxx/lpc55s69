/**
 * @file storage_flash.h
 * @brief  Flash based storage driver
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v2.0
 * @date 2020-11-05
 */

#ifndef _STORAGE_FLASH_H_
#define _STORAGE_FLASH_H_

#include <stdint.h>

#define FLASH_SECTOR_SIZE 512   //!< The sector size of the flash
#define MAX_AREA_NAME 32        //!< Max define for the farea name

/**
 * @brief  Flash area definition
 */
struct flash_area_t {
	char		area_name[MAX_AREA_NAME + 1];   //!< Name of the flash area
	uint32_t	start_addr;                     //!< Start address of the area
	uint32_t	size;                           //!< Size of the area
	uint32_t	offset;                         //!< Current write offset
};

/**
 * @brief  Create new flash based storage driver
 *
 * @returns An allocated structure or NULL on failure
 */
struct storage_driver_t *storage_new_flash_driver();

/**
 * @brief  Create a new flash area
 *
 * @param name The name of the area
 * @param addr The start addres
 * @param size The size
 *
 * @returns  A struct (copy) containing the new flash area
 */
struct flash_area_t storage_new_flash_area(char *name, uint32_t addr, uint32_t
					   size);

/**
 * @brief Set the current flash area for the storage driver
 *
 * @param sdriver The storage driver of which we set the area
 * @param farea The new flash area
 */
void storage_set_flash_area(struct storage_driver_t *sdriver, struct
			    flash_area_t *farea);

/**
 * @brief Check if the partition is empty
 *
 * @param sdriver The storage driver which we'll check for
 *
 * @returns  true or false
 */
bool storage_is_empty_partition(struct storage_driver_t *sdriver);


/**
 * @brief  Wipe the application0/1 flash storage
 *
 * @param sdriver The storage driver used to wipe
 *
 * @returns  -1 if failed otherwise 0
 * @warning This uses defines from the linkerscript and is not linked to the current flash area
 */
int storage_wipe_entire_flash(struct storage_driver_t *sdriver);

#endif /* _STORAGE_FLASH_H_ */
