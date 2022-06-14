/**
 * @file storage_spi_flash.c
 * @brief Storage spi flash driver
 * @author David Thio <david.thio@barco.com>
 * @version v1.0
 * @date 2022-03-30
 */

#include <stdlib.h>

#include "is25xp.h"
#include "fsl_crc.h"
#include "fsl_spi.h"

#include "crc.h"
#include "logger.h"
#include "storage.h"
#include "storage_spi_flash.h"

#include <board.h>
#include <pin_mux.h>
#include "peripherals.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define SPI_MASTER            SPI8
#define SPI_MASTER_IRQ        LSPI_HS_IRQn // = FLEXCOMM8_IRQn
#define SPI_MASTER_CLK_SRC    kCLOCK_Flexcomm8
#define SPI_MASTER_CLK_FREQ   CLOCK_GetHsLspiClkFreq()
#define SPI_SSEL              0
#define SPI_MASTER_IRQHandler FLEXCOMM8_IRQHandler
#define SPI_SPOL              kSPI_SpolActiveAllLow

/*******************************************************************************
 * Variables
 ******************************************************************************/
// Identification , 4byte: manufacturer, memory, capacity, device ID
static uint8_t SpiFlash_Identification[5];

/*******************************************************************************
 * external code - crc.c
 ******************************************************************************/

extern uint32_t crc32(uint32_t crc, const uint8_t * buf, size_t len);

/*******************************************************************************
 * Code
 ******************************************************************************/
static void _storage_init_spi_flash() {
    /* SPI init */
    uint32_t srcClock_Hz = 0U;
    spi_master_config_t masterConfig;

    srcClock_Hz = SPI_MASTER_CLK_FREQ;

    SPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.sselNum = (spi_ssel_t)SPI_SSEL;
    masterConfig.sselPol = (spi_spol_t)SPI_SPOL;
#if defined(SPI_BAUDRATE)
    masterConfig.baudRate_Bps = SPI_BAUDRATE;
#else
    masterConfig.baudRate_Bps = 25000000U;
#endif
    int error = SPI_MasterInit(SPI_MASTER, &masterConfig, srcClock_Hz);
    if (error != kStatus_Success) {
        LOG_ERROR("SPI Master Init failed with code %d", error);
        return;
    }

    BOARD_SetSPIMux(1);

    // initialisation of SPI Flash struct
    is25xp_readid(SpiFlash_Identification, 0);

    LOG_DEBUG("is25xp - manufacturer: %02x memory: %02x capacity: %02x device ID: %02x",
              SpiFlash_Identification[0],
              SpiFlash_Identification[1],
              SpiFlash_Identification[2],
              SpiFlash_Identification[3]);

    LOG_DEBUG("spi_master_setup on flexcomm8");

    BOARD_SetSPIMux(0);
}

static int _init_spi_flash_storage(struct storage_driver_t * sdriver) {
    (void)sdriver;
    LOG_INFO("Running spi flash backed storage driver");
    _storage_init_spi_flash();
    return 0;
}

static int _read_spi_flash_storage(struct storage_driver_t * sdriver,
                                   uint8_t * buffer,
                                   size_t len) {
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    if (farea->offset == 0)
        LOG_INFO("Reading from[%s]at 0x%x, offset: %d len: %d", farea->area_name,
                 farea->start_addr, farea->offset, len);
    BOARD_SetSPIMux(1);
    int ret = is25xp_read((off_t)farea->start_addr + (off_t)farea->offset, len, buffer, 1);
    BOARD_SetSPIMux(0);
    // LOG_DEBUG("is25xp_read startblock: %04lx returned [%d] bytes", (long)farea->start_addr, ret);

    if (ret <= 0) {
        LOG_ERROR("Flash read error");
        return -1;
    }
    farea->offset += len;
    return ret;
}

static int _write_spi_flash_storage(struct storage_driver_t * sdriver,
                                    uint8_t * buffer, size_t len) {
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    off_t startPage = (off_t)farea->start_addr / IS25_IS25XP_BYTES_PER_PAGE;
    off_t blockNr = (off_t)farea->offset / IS25_IS25XP_BYTES_PER_PAGE;

    BOARD_SetSPIMux(1);

    is25xp_pagewrite(buffer, startPage + blockNr++);
    // transfer is per 512 bytes we need a 2nd page-write
    if (len > IS25_IS25XP_BYTES_PER_PAGE) {
        is25xp_pagewrite(buffer + IS25_IS25XP_BYTES_PER_PAGE, startPage + blockNr++);
    }

    // LOG_INFO("Spi write len %d to 0x%x per 256 bytes (blockNr:%d)", len,
    //        farea->offset, blockNr, pages);

    BOARD_SetSPIMux(0);

    farea->offset += len;
    return 1;
}

static int _flush_spi_flash_storage(struct storage_driver_t * sdriver) {
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    LOG_DEBUG("Flushing spi storage driver");
    int ret = (int)farea->offset;
    LOG_OK("Flush Spi - size received was: %d (%s)", farea->offset, farea->area_name);
    farea->offset = 0;

    return ret;
}

static int _erase_spi_flash_storage(struct storage_driver_t * sdriver) {
    LOG_INFO("Erasing spi flash backend");
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);
    // translate into is25x blocks / total mem is 128Mbit or 16Mbyte

    // 1 block/sector is 4Kb
    const long unsigned int SECTOR_SIZE = (1024 * 4);

    off_t startBlock = (off_t)farea->start_addr / (off_t)SECTOR_SIZE;
    size_t nBlocks = (size_t)farea->size / SECTOR_SIZE;

    LOG_INFO("spi erase: Addr 0x%x size: 0x%X (Nr of blocks = %d)", farea->start_addr,
             farea->size, nBlocks);

    BOARD_SetSPIMux(1);
    size_t erasedBlocks = (size_t)is25xp_erase(startBlock, nBlocks);
    BOARD_SetSPIMux(0);

    if (erasedBlocks != nBlocks) {
        LOG_ERROR("Erased blocks don't match %d<>%d", erasedBlocks, nBlocks);
        return -1;
    }

    LOG_OK("Verify Erased done!");
    return 0;
}

static uint32_t _crc_spi_flash_storage(struct storage_driver_t * sdriver, size_t len) {
    struct spi_flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    // the idea is to only check the supplied file length = len
    if (len == 0)
        len = farea->size;

    farea->offset = 0; // start reading from 0

    LOG_INFO("spi calculate crc length of %X..%X", farea->start_addr, farea->start_addr + len);

    const uint32_t BLOCKSIZE = FLASH_SECTOR_SIZE;
    uint8_t buffer[BLOCKSIZE];
    uint32_t crc = 0;
    uint32_t blocks = len / BLOCKSIZE;
    uint8_t rest = (uint8_t)(len % BLOCKSIZE);

    LOG_DEBUG("spi crc calculation blocks of 512: %d rest: %d", blocks, rest);

    for (uint32_t i = 0; i < blocks; i++) {
        _read_spi_flash_storage(sdriver, buffer, BLOCKSIZE);
        crc = crc32(crc, buffer, BLOCKSIZE);
    }
    if (rest) {
        _read_spi_flash_storage(sdriver, buffer, rest);
        crc = crc32(crc, buffer, rest);
    }

    return crc;
}

static int _close_spi_flash_storage(struct storage_driver_t * sdriver) {
    (void)sdriver;
    return 0;
}

static const struct storage_ops_t fops = {
    .init  = _init_spi_flash_storage,
    .read  = _read_spi_flash_storage,
    .write = _write_spi_flash_storage,
    .erase = _erase_spi_flash_storage,
    .flush = _flush_spi_flash_storage,
    .crc   = _crc_spi_flash_storage,
    .close = _close_spi_flash_storage,
};

static struct storage_driver_t fdriver = {
    .name     = "spiflash",
    .type     = STORAGE_SPI_EXTERNAL,
    .ops      = &fops,
    .privdata = NULL,
};

struct storage_driver_t * storage_new_spi_flash_driver() {
    return &fdriver;
}

struct spi_flash_area_t storage_new_spi_flash_area(char * name, uint32_t addr, uint32_t size) {
    struct spi_flash_area_t area = {
        .start_addr = addr,
        .size       = size,
        .offset     = 0,
    };

    strncpy(area.area_name, name, MAX_AREA_NAME);

    LOG_DEBUG("New spi flash area [%s] @ 0x%.8x with size %dkB",
              area.area_name,
              area.start_addr,
              (area.size / 1024));
    return area;
}

void storage_set_spi_flash_area(struct storage_driver_t * sdriver,
                                struct spi_flash_area_t * farea) {
    LOG_INFO("Setting spi flash area %s @0x%p", farea->area_name, farea->start_addr);
    STORAGE_SETPRIV(sdriver, farea);
}

int storage_wipe_entire_spi_flash(struct storage_driver_t * sdriver) {
    // LOG_INFO("Wipe spi flash");
    _erase_spi_flash_storage(sdriver);
    LOG_OK("Erased spi partition");
    return 0;
}

int storage_wipe_partial_spi_flash(struct spi_flash_area_t * farea) {
    LOG_INFO("Wipe %s flash area from %X size:%X", farea->area_name,
             farea->start_addr, farea->size);

    const size_t SECTOR_SIZE = (1024 * 4);

    // 1 block is 4Kb = minimale erase length
    off_t startBlock = (off_t)farea->start_addr / (off_t)SECTOR_SIZE;
    size_t nBlocks = (size_t)farea->size / SECTOR_SIZE;

    LOG_INFO("spi erase: Addr 0x%x size: 0x%X (number of blocks %d)",
             farea->start_addr, farea->size, nBlocks);

    BOARD_SetSPIMux(1);
    size_t erasedBlocks = (size_t)is25xp_erase(startBlock, nBlocks);
    BOARD_SetSPIMux(0);

    if (erasedBlocks != nBlocks) {
        LOG_ERROR("Number of erased blocks don't match %d<>%d", erasedBlocks,
                  (int)(farea->size / SECTOR_SIZE));
        return -1;
    }

    LOG_OK("Partial spi erase done");

    return 0;
}
