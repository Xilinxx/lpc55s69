#ifndef _BOOTLOADER_HELPER_H_
#define _BOOTLOADER_HELPER_H_
/**
 * @file bootloader_helpers.h
 * @brief  Bootloader helper functions
 * @version v0.1
 * @date 2021-07-02
 */

int _bootloader_verify_bootinfo_crc(struct bootloader_ctxt_t *bctxt,int silent);

void _bootloader_print_info(struct bootloader_ctxt_t *bctxt);

void _bootloader_print_info_header(struct bootloader_ctxt_t *bctxt, const char *header_txt);

int _bootloader_retrieve_ctxt(struct bootloader_ctxt_t *bctxt, struct storage_driver_t *sdriver);

bool _bootloader_ctxt_equal(struct bootloader_ctxt_t *b1, struct bootloader_ctxt_t *b2);

int _bootloader_ctxt_validate_partition(struct bootloader_ctxt_t *bctxt, app_partition_t partition);

uint32_t _bootloader_ctxt_get_crc_partition(struct bootloader_ctxt_t *bctxt, app_partition_t partition);

int _bootloader_store_ctxt(struct bootloader_ctxt_t *bctxt, struct storage_driver_t *sdriver);

int _bootloader_initialize_ctxt(struct bootloader_ctxt_t *bctxt, struct storage_driver_t *sdriver);

void _bootloader_swap_partitions(struct bootloader_ctxt_t *bctxt);

int _bootloader_send_bootinfo(struct bootloader_ctxt_t *bctxt, struct comm_driver_t *cdriver);

int _bootloader_erase_spi_rom(struct storage_driver_t *sdriver);

#endif