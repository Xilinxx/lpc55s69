/**
 * @file bootloader_helpers.c
 * @brief  Bootloader helper functions
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-10-27
 */

#include <stdint.h>
#include <string.h>
#include "crc.h"
#include "comm_driver.h"
#include "comm_protocol.h"
#include "bootloader.h"
#include "storage.h"
#include "storage_flash.h"
#include "logger.h"
#include "version.h"
#include "memory_map.h"

int _bootloader_verify_bootinfo_crc(struct bootloader_ctxt_t * bctxt, int silent) {
    uint32_t crc = crc_run_crc32((uint8_t *)bctxt, (sizeof(struct bootloader_ctxt_t)
                                                    - sizeof(bctxt->crc) - sizeof(bctxt->part)));

    if (silent)
        return (crc == bctxt->crc);

    LOG_RAW("BOOTINFO CRC: 0x%x (%s)", bctxt->crc, crc == bctxt->crc ? "valid" : "invalid");
    return (crc == bctxt->crc);
}

void _bootloader_print_info(struct bootloader_ctxt_t * bctxt) {
    LOG_RAW("Bootloader info (@%p) (size %d bytes)", (void *)&bctxt,
            sizeof(struct bootloader_ctxt_t));
    LOG_RAW("Version:    %s", BOOTLOADER_VERSION_GIT);
    LOG_RAW("Nr of Partitions: %d", BTLR_NR_APPS);
    if (bctxt->part == APP_PARTITION_NONE) {
        LOG_RAW("Current partition: APP_PARTITION_NONE(-1)");
        return;
    }
    LOG_RAW("Current partition: %d (size %d bytes)", bctxt->part, sizeof(bctxt->part));
    if (bctxt->part == APP_PARTITION_NONE)
        return;
    for (int i = 0; i < BTLR_NR_APPS; ++i) {
        LOG_RAW("Partition #%d (%s)", i,
                bctxt->apps[i].crc == BOOTLOADER_MAGIC_CRC ?
                "unused" : "used");
        LOG_RAW("\t start    @ 0x%x", bctxt->apps[i].start_addr);
        LOG_RAW("\t app size:  0x%x", bctxt->apps[i].application_size);
        LOG_RAW("\t part size: 0x%x", bctxt->apps[i].partition_size);
        LOG_RAW("\t crc:       0x%x", bctxt->apps[i].crc);
    }
    _bootloader_verify_bootinfo_crc(bctxt, 0);
}

void _bootloader_print_info_header(struct bootloader_ctxt_t * bctxt,
                                   const char * header_txt) {
    LOG_RAW("%s (@%p) (size %d bytes)", header_txt, (void *)&bctxt,
            sizeof(struct bootloader_ctxt_t));
    LOG_RAW("Nr of Partitions: %d", BTLR_NR_APPS);
    if (bctxt->part == APP_PARTITION_NONE) {
        LOG_RAW("Current partition: APP_PARTITION_NONE(-1)");
        return;
    }
    LOG_RAW("Current partition: %d (size %d bytes)", bctxt->part, sizeof(bctxt->part));
    for (int i = 0; i < BTLR_NR_APPS; i++) {
        LOG_RAW("Partition #%d (%s)", i,
                bctxt->apps[i].crc == BOOTLOADER_MAGIC_CRC ?
                "unused" : "used");
        LOG_RAW("\t start    @ 0x%x", bctxt->apps[i].start_addr);
        LOG_RAW("\t app size:  0x%x", bctxt->apps[i].application_size);
        LOG_RAW("\t part size: 0x%x", bctxt->apps[i].partition_size);
        LOG_RAW("\t crc:       0x%x", bctxt->apps[i].crc);
    }
    _bootloader_verify_bootinfo_crc(bctxt, 0);
}

int _bootloader_retrieve_ctxt(struct bootloader_ctxt_t * bctxt,
                              struct storage_driver_t * sdriver) {
    if (storage_is_empty_partition(sdriver)) {
        LOG_ERROR("Empty partition...");
        return -1;
    }

    LOG_INFO("Retrieving bootloader_ctxt_t from flash");
    int error = storage_read_data(sdriver, (uint8_t *)bctxt, sizeof(struct
                                                                    bootloader_ctxt_t));
    if (error < 0) {
        LOG_ERROR("Failed to retrieve bootloader info from flash");
        return error;
    }
    return error;
}

bool _bootloader_ctxt_equal(struct bootloader_ctxt_t * b1,
                            struct bootloader_ctxt_t * b2) {
    bootloader_ctxt_converter_t ctxt1, ctxt2;

    ctxt1.data = *b1;
    ctxt2.data = *b2;

    for (unsigned long i = 0; i < (sizeof(struct app_ctxt_t) * BTLR_NR_APPS); ++i) {
        if (ctxt1.bytes[i] != ctxt2.bytes[i]) {
            LOG_WARN("diff at pos %d[%x<>%x]", ctxt1.bytes[i], ctxt2.bytes[i]);
            return false;
        }
    }
/*
 *              //<! We only compare partition data
 *              if (!memcmp(b1, b2, sizeof(struct app_ctxt_t) * BTLR_NR_APPS))
 *              {
 *                      return true;
 *              }
 */
    return true;
}

int _bootloader_ctxt_validate_partition(struct bootloader_ctxt_t * bctxt,
                                        app_partition_t partition) {
    LOG_INFO("Validating partition %d", partition);
    if (bctxt->apps[partition].crc == BOOTLOADER_MAGIC_CRC) {
        LOG_DEBUG("Empty partition");
        return 0;
    }
    uint32_t crc = crc_run_crc32(
        (uint8_t *)bctxt->apps[partition].start_addr,
        bctxt->apps[partition].partition_size);
    if (crc == 0) {     // retry
        LOG_WARN("Got CRC=0, retry reading");
        crc = crc_run_crc32(
            (uint8_t *)bctxt->apps[partition].start_addr,
            bctxt->apps[partition].partition_size);
    }
    if (crc == bctxt->apps[partition].crc) {
        return 1;
    }
    LOG_ERROR("Invalid crc! Expected %x got %x", crc, bctxt->apps[partition].crc);
    return -1;
}

uint32_t _bootloader_ctxt_get_crc_partition(struct bootloader_ctxt_t * bctxt,
                                            app_partition_t partition) {
    LOG_DEBUG("Calculating crc partition %d at 0x%X length 0x%x",
              partition,
              bctxt->apps[partition].start_addr, bctxt->apps[partition].application_size);
    uint32_t crc = crc_run_crc32(
        (uint8_t *)bctxt->apps[partition].start_addr,
        bctxt->apps[partition].partition_size);
    LOG_INFO("crc partition [%d] = %X", partition, crc);
    return crc;
}

int _bootloader_store_ctxt(struct bootloader_ctxt_t * bctxt, struct
                           storage_driver_t * sdriver) {
    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    farea->offset = 0;
    storage_erase_storage(sdriver);

    bctxt->crc = crc_run_crc32((uint8_t *)bctxt, (sizeof(struct
                                                         bootloader_ctxt_t) - sizeof(bctxt->crc) - sizeof(bctxt->part)));
    LOG_DEBUG("Bootloader CRC: %x", bctxt->crc);
    LOG_DEBUG("Application CRC: 0[%x] 1[%x]", bctxt->apps[0].crc, bctxt->apps[1].crc);
    LOG_DEBUG("Sizeof bootloader ctxt: %d", sizeof(struct bootloader_ctxt_t));

    unsigned int sectors_needed = (sizeof(struct bootloader_ctxt_t) /
                                   FLASH_SECTOR_SIZE) + 1;
    LOG_DEBUG("Sectors needed: %d", sectors_needed);

    uint8_t flash_buffer[sectors_needed * FLASH_SECTOR_SIZE];
    memset(flash_buffer, 0, (sectors_needed * FLASH_SECTOR_SIZE));
    memcpy(flash_buffer, bctxt, sizeof(struct bootloader_ctxt_t));

    int error = storage_write_data(sdriver, flash_buffer,
                                   (FLASH_SECTOR_SIZE * sectors_needed));
    if (error < 0) {
        LOG_ERROR("Failed to write boot context to flash");
        return error;
    }

    struct bootloader_ctxt_t copy = *bctxt;
    error = _bootloader_retrieve_ctxt(bctxt, sdriver);
    if (error < 0) {
        LOG_ERROR("Failed to retrieve boot context from flash");
        return error;
    }

    if (_bootloader_ctxt_equal(bctxt, &copy)) {
        LOG_OK("Bootloader ctxt write successful");
    } else {
        LOG_ERROR("Readback does not match with expected");
        return error;
    }
    return error;
}

int _bootloader_initialize_ctxt(struct bootloader_ctxt_t * bctxt, struct
                                storage_driver_t * sdriver) {
    LOG_INFO("Creating new bootloader context (%d bytes)", sizeof(struct bootloader_ctxt_t));
    memset(bctxt, 0, sizeof(struct bootloader_ctxt_t));
    bctxt->apps[APP_PARTITION_0].start_addr = (uint32_t)&__approm0_start__;
    bctxt->apps[APP_PARTITION_0].partition_size = (uint32_t)&__approm0_size__;
    bctxt->apps[APP_PARTITION_0].application_size = 0;
    bctxt->apps[APP_PARTITION_0].crc = BOOTLOADER_MAGIC_CRC;

    bctxt->apps[APP_PARTITION_1].start_addr = (uint32_t)&__approm1_start__;
    bctxt->apps[APP_PARTITION_1].partition_size =   (uint32_t)&__approm1_size__;
    bctxt->apps[APP_PARTITION_1].application_size = 0;
    bctxt->apps[APP_PARTITION_1].crc = BOOTLOADER_MAGIC_CRC;

    bctxt->part = APP_PARTITION_NONE;
    return _bootloader_store_ctxt(bctxt, sdriver);
}

void _bootloader_swap_partitions(struct bootloader_ctxt_t * bctxt) {
    if (bctxt->part == APP_PARTITION_0) {
        bctxt->part = APP_PARTITION_1;
    } else {
        bctxt->part = APP_PARTITION_0;
    }
}

int _bootloader_send_bootinfo(struct bootloader_ctxt_t * bctxt, struct comm_driver_t * cdriver) {
    uint8_t out_buffer[COMMP_CMD_PACKET_SIZE + sizeof(struct bootloader_ctxt_t)] = { 0 };

    memset(out_buffer, 0, sizeof(out_buffer));
    memcpy(&out_buffer[COMMP_CMD_PACKET_SIZE], bctxt, sizeof(struct bootloader_ctxt_t));

    out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0x00;
    out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
    out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
    out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_BOOTINFO;

    return comm_protocol_write_data(cdriver, out_buffer, sizeof(out_buffer));
}
