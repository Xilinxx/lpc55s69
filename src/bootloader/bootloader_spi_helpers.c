/**
 * @file bootloader_spi_helpers.c
 * @brief  Bootloader helper functions
 * @author david thio <david.thio@barco.com>
 * @version v0.1
 * @date 2022-05-06
 */

#include <stdint.h>
#include <string.h>
#include "crc.h"
#include "comm_driver.h"
#include "comm_protocol.h"
#include "bootloader_spi.h"
#include "storage.h"
#include "storage_spi_flash.h"
#include "logger.h"
#include "version.h"
#include "memory_map.h"

extern uint32_t crc32(uint32_t crc, const uint8_t * buf, size_t len);

static int memvcmp(void * memory, unsigned char val, unsigned int size) {
    unsigned char * mm = (unsigned char *)memory;

    return (*mm == val) && memcmp(mm, mm + 1, size - 1) == 0;
}

int _bootloader_spi_verify_crc(struct spi_ctxt_t * bctxt, struct
                               storage_driver_t * spidriver, bool silent) {
    uint32_t crc = storage_crc_storage(spidriver, bctxt->gowin[GOWIN_PARTITION_0].image_size);

    if (silent)
        return (crc == bctxt->gowin[GOWIN_PARTITION_0].crc);

    LOG_RAW("SPI CRC: 0x%x (%s)", bctxt->gowin[GOWIN_PARTITION_0].crc,
            crc == bctxt->gowin[GOWIN_PARTITION_0].crc ? "valid" : "invalid");

    return (crc == bctxt->gowin[GOWIN_PARTITION_0].crc);
}

void _bootloader_spi_print_info(struct spi_ctxt_t * bctxt) {
    LOG_RAW("Spi info (@%p) (size %d bytes)", (void *)&bctxt,
            sizeof(struct spi_ctxt_t));
    LOG_RAW("Nr of Partitions: %d", BTLR_SPI_NR_BIN);
    LOG_RAW("Current partition: 0x%x (%d bytes)", bctxt->part & 0x000000ff, sizeof(bctxt->part));
    LOG_RAW("Own crc: 0x%x", bctxt->crc);
    for (int i = 0; i < BTLR_SPI_NR_BIN; i++) {
        LOG_RAW("Partition #%d (%s)", i,
                bctxt->gowin[i].crc == SPI_MAGIC_CRC ? "unused" : "used");
        LOG_RAW("\t start    @ 0x%x", bctxt->gowin[i].start_addr);
        LOG_RAW("\t app size:  0x%x", bctxt->gowin[i].image_size);
        LOG_RAW("\t part size: 0x%x", bctxt->gowin[i].partition_size);
        LOG_RAW("\t crc:       0x%x", bctxt->gowin[i].crc);
        LOG_RAW("\t file size: 0x%x - %d", bctxt->gowin[i].bitfile_size,
                bctxt->gowin[i].bitfile_size);
        LOG_RAW("\t file crc:  0x%x", bctxt->gowin[i].bitfile_crc);
    }
}

void _bootloader_spi_print_info_header(struct spi_ctxt_t * bctxt,
                                       const char * header_txt) {
    LOG_RAW("%s (@%p)", header_txt, (void *)&bctxt);
    LOG_RAW("Nr of Partitions: %d", BTLR_SPI_NR_BIN);
    LOG_RAW("Current partition: 0x%x (%d bytes)", bctxt->part & 0x000000ff, sizeof(bctxt->part));
    LOG_RAW("Own crc: 0x%x", bctxt->crc);
    for (int i = 0; i < BTLR_SPI_NR_BIN; i++) {
        LOG_RAW("Partition #%d (%s)", i,
                bctxt->gowin[i].crc == SPI_MAGIC_CRC ? "unused" : "used");
        LOG_RAW("\t start    @ 0x%x", bctxt->gowin[i].start_addr);
        LOG_RAW("\t app size:  0x%x", bctxt->gowin[i].image_size);
        LOG_RAW("\t part size: 0x%x", bctxt->gowin[i].partition_size);
        LOG_RAW("\t crc:       0x%x", bctxt->gowin[i].crc);
        LOG_RAW("\t file size: 0x%x - %d", bctxt->gowin[i].bitfile_size,
                bctxt->gowin[i].bitfile_size);
        LOG_RAW("\t file crc:  0x%x", bctxt->gowin[i].bitfile_crc);
    }
}

int _bootloader_spi_retrieve_ctxt(struct spi_ctxt_t * bctxt,
                                  struct storage_driver_t * sdriver) {
    LOG_INFO("Retrieving spi_ctxt_t from flash");

    // set the offset to read from!
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);
    farea->offset = SPI_PART_SIZE - FLASH_SECTOR_SIZE;

    int error = storage_read_data(sdriver, (uint8_t *)bctxt, sizeof(struct spi_ctxt_t));
    if (error < 0) {
        LOG_ERROR("Failed to retrieve bootloader info from spi flash");
        return error;
    }


    if (memvcmp(bctxt, 0xff, sizeof(struct spi_ctxt_t))) {
        LOG_WARN("Empty spi flash context");
        return -1;
    }

    return error;
}

bool _bootloader_spi_ctxt_equal(struct spi_ctxt_t * b1, struct spi_ctxt_t * b2) {
    // we only compare the partition data! , not the own crc and partition
    if (!memcmp(b1, b2, sizeof(spi_partition_ctxt_t) * BTLR_SPI_NR_BIN)) {
        return true;
    }
    return false;
}

int _bootloader_spi_ctxt_validate_partition(struct spi_ctxt_t * bctxt,
                                            struct storage_driver_t * spidriver,
                                            gowin_partition_t partition) {
    LOG_INFO("Validating partition %d", partition);
    if (bctxt->gowin[partition].crc == SPI_MAGIC_CRC) {
        LOG_DEBUG("Empty partition");
        return 0;
    }

    uint32_t crc = storage_crc_storage(spidriver, bctxt->gowin[partition].image_size);

    if (crc == bctxt->gowin[partition].crc)
        return 1;

    LOG_ERROR("Invalid crc! Expected %x got %x", crc, bctxt->gowin[partition].crc);
    return -1;
}

uint32_t _bootloader_spi_ctxt_get_crc_partition(struct spi_ctxt_t * bctxt,
                                                struct storage_driver_t * spidriver,
                                                gowin_partition_t partition) {
    LOG_DEBUG("Calculating crc partition %d at 0x%X length 0x%x",
              partition,
              bctxt->gowin[partition].start_addr, bctxt->gowin[partition].image_size);

    uint32_t crc = storage_crc_storage(spidriver, bctxt->gowin[partition].image_size);

    LOG_INFO("crc partition [%d] = %X", partition, crc);

    return crc;
}

int _bootloader_spi_store_ctxt(struct spi_ctxt_t * bctxt,
                               struct storage_driver_t * sdriver) {
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);
    struct spi_flash_area_t spi_erase_area;

    strcpy(spi_erase_area.area_name, farea->area_name);
    spi_erase_area.start_addr = farea->start_addr + SPI_PART_SIZE - MIN_ERASE_SIZE;
    spi_erase_area.size = MIN_ERASE_SIZE;
    storage_wipe_partial_spi_flash(&spi_erase_area);

    unsigned int sectors_needed = (sizeof(struct spi_ctxt_t) /
                                   FLASH_SECTOR_SIZE) + 1;
    LOG_DEBUG("Sectors needed: %d", sectors_needed);

    // now calculate own data's crc
    uint32_t crc = 0;
    spi_ctxt_converter_t ctxt;
    ctxt.data = *bctxt;
    bctxt->part = 0;     // force always 0
    // Own context crc only includes both partition info
    bctxt->crc = crc32(crc, ctxt.bytes, sizeof(spi_partition_ctxt_t) * BTLR_SPI_NR_BIN);

    uint8_t flash_buffer[sectors_needed * FLASH_SECTOR_SIZE];
    memset(flash_buffer, 0, (sectors_needed * FLASH_SECTOR_SIZE));
    memcpy(flash_buffer, bctxt, sizeof(struct spi_ctxt_t));

    farea->offset = SPI_PART_SIZE - FLASH_SECTOR_SIZE;

    int error = storage_write_data(sdriver, flash_buffer,
                                   (FLASH_SECTOR_SIZE * sectors_needed));
    if (error < 0) {
        LOG_ERROR("Failed to write spi boot context to flash");
        return error;
    }

    struct spi_ctxt_t copy = *bctxt;
    memset(bctxt, 0xaa, sizeof(struct spi_ctxt_t));
    error = _bootloader_spi_retrieve_ctxt(bctxt, sdriver);
    if (error < 0) {
        LOG_ERROR("Failed to retrieve spi boot context");
        return error;
    } else {
        LOG_OK(" spi content: ");
    }

    _bootloader_spi_print_info(bctxt);

    if (_bootloader_spi_ctxt_equal(bctxt, &copy)) {
        LOG_OK("Bootloader ctxt write successful");
    } else {
        LOG_ERROR("Readback does not match with expected");
        return error;
    }
    return error;
}

void _bootloader_spi_initialize_ctxt(struct spi_ctxt_t * bctxt, gowin_partition_t partition) {
    LOG_INFO("Creating new bootloader context for spi%d (%d bytes)", partition,
             sizeof(struct spi_ctxt_t));

    if (partition == GOWIN_PARTITION_NONE)
        memset(bctxt, 0, sizeof(struct spi_ctxt_t));

    if (partition == GOWIN_PARTITION_0 || partition == GOWIN_PARTITION_NONE) {
        bctxt->gowin[GOWIN_PARTITION_0].start_addr = (uint32_t)0x0;
        bctxt->gowin[GOWIN_PARTITION_0].partition_size = (uint32_t)SPI_PART_SIZE;
        // Last 4k is reserved for partition info
        bctxt->gowin[GOWIN_PARTITION_0].image_size = (uint32_t)SPI_PART_SIZE - MIN_ERASE_SIZE;
        bctxt->gowin[GOWIN_PARTITION_0].crc = SPI_MAGIC_CRC;
        bctxt->gowin[GOWIN_PARTITION_0].bitfile_crc = SPI_MAGIC_CRC;
        bctxt->gowin[GOWIN_PARTITION_0].bitfile_size = 0;
    }

    if (partition == GOWIN_PARTITION_1 || partition == GOWIN_PARTITION_NONE) {
        bctxt->gowin[GOWIN_PARTITION_1].start_addr = (uint32_t)SPI_PART_SIZE;
        bctxt->gowin[GOWIN_PARTITION_1].partition_size =        (uint32_t)SPI_PART_SIZE;
        // Last 4k is reserved for partition info
        bctxt->gowin[GOWIN_PARTITION_1].image_size = (uint32_t)SPI_PART_SIZE - MIN_ERASE_SIZE;
        bctxt->gowin[GOWIN_PARTITION_1].crc = SPI_MAGIC_CRC;
        bctxt->gowin[GOWIN_PARTITION_1].bitfile_crc = SPI_MAGIC_CRC;
        bctxt->gowin[GOWIN_PARTITION_1].bitfile_size = 0;
    }

    bctxt->part = GOWIN_PARTITION_NONE;
}

int _bootloader_spi_send_spiinfo(struct spi_ctxt_t * bctxt, struct comm_driver_t * cdriver) {
    uint8_t out_buffer[COMMP_CMD_PACKET_SIZE + sizeof(struct spi_ctxt_t)] = { 0 };

    memset(out_buffer, 0, sizeof(out_buffer));
    memcpy(&out_buffer[COMMP_CMD_PACKET_SIZE], bctxt, sizeof(struct spi_ctxt_t));

    out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0x00;
    out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
    out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
    out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_INFO_SPI;

    return comm_protocol_write_data(cdriver, out_buffer, sizeof(out_buffer));
}

int _bootloader_spi_erase_spi_rom(struct storage_driver_t * sdriver) {
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    farea->offset = 0;
    return storage_wipe_entire_spi_flash(sdriver);
}
