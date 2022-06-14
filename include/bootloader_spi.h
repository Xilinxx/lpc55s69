/**
 * @file    bootloader_spi.h
 * @brief   Bootloader info spi flash partition
 * @author  David Thio <david.thio@barco.com>
 * @version v0.1
 * @date    2022-05-06
 */

#ifndef _BOOTLOADER_SPI_FLASH_H_
#define _BOOTLOADER_SPI_FLASH_H_

#include <stdint.h>

#define BTLR_SPI_NR_BIN 2 // !< 2 Gowin bitfiles

#define SPI_MAGIC_CRC   0xDEADBEEF

#pragma pack(push, 1)
/**
 * @brief  Partition information context
 */
typedef struct spi_partition_ctxt_t {
    uint32_t start_addr;            // !< Start address of bitfile image
    uint32_t image_size;            // !< Actual image size
    uint32_t partition_size;        // !< Size of the partition
    uint32_t crc;                   // !< CRC of the image
    uint32_t bitfile_size;          // !< Size of the uploaded bitfile
    uint32_t bitfile_crc;           // !< CRC32 on the length of the file
} spi_partition_ctxt_t;


typedef enum {
    GOWIN_PARTITION_NONE = -1,
    GOWIN_PARTITION_0,
    GOWIN_PARTITION_1,
} gowin_partition_t;

/**
 * @brief  SPI context
 */
typedef struct spi_ctxt_t {
    struct spi_partition_ctxt_t gowin[BTLR_SPI_NR_BIN];// !< contexts
    uint32_t crc;                                   // !< spi info ctxt crc
    /* Careful here for buildroot enum is 4 byte, for arm 1 byte! */
    gowin_partition_t part;                         // !< Current Gowin Partition
} spi_ctxt_t;

typedef union spi_ctxt_converter_t {
    spi_ctxt_t data;
    uint8_t bytes[sizeof(spi_ctxt_t)];  // !< The content in an array
} spi_ctxt_converter_t;

#pragma pack(pop)

#endif /* _BOOTLOADER_SPI_FLASH_H_ */
