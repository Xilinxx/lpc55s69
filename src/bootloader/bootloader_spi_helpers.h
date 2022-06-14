#ifndef _BOOTLOADER_SPI_HELPER_H_
#define _BOOTLOADER_SPI_HELPER_H_
/**
 * @file bootloader_spi_helpers.h
 * @brief  Bootloader spi helper functions
 * @version v0.1
 * @date 2022-05-06
 */

int _bootloader_spi_verify_crc(struct spi_ctxt_t * bctxt, struct storage_driver_t * spidriver, bool silent);

void _bootloader_spi_print_info(struct spi_ctxt_t * bctxt);

void _bootloader_spi_print_info_header(struct spi_ctxt_t * bctxt, const char * header_txt);

int _bootloader_spi_retrieve_ctxt(struct spi_ctxt_t * bctxt, struct storage_driver_t * sdriver);

bool _bootloader_spi_ctxt_equal(struct spi_ctxt_t * b1, struct spi_ctxt_t * b2);

int _bootloader_spi_ctxt_validate_partition(struct spi_ctxt_t * bctxt, struct storage_driver_t * spidriver, gowin_partition_t partition);

uint32_t _bootloader_spi_ctxt_get_crc_partition(struct spi_ctxt_t * bctxt, struct storage_driver_t * spidriver, gowin_partition_t partition);

int _bootloader_spi_store_ctxt(struct spi_ctxt_t * bctxt, struct storage_driver_t * sdriver);

void _bootloader_spi_initialize_ctxt(struct spi_ctxt_t * bctxt, gowin_partition_t partition);

int _bootloader_spi_send_spiinfo(struct spi_ctxt_t * bctxt, struct comm_driver_t * cdriver);

int _bootloader_spi_erase_spi_rom(struct storage_driver_t * sdriver);

#endif