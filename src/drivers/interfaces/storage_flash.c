/**
 * @file storage_flash.c
 * @brief Storage flash driver
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v2.0
 * @date 2020-10-19
 */

#include <stdlib.h>

#include "fsl_crc.h"
#include "fsl_iap.h"

#include "crc.h"
#include "logger.h"
#include "storage.h"
#include "storage_flash.h"

#include "memory_map.h"

static flash_config_t _fcfg = { 0 };

static void _storage_init_flash() {
    _fcfg.modeConfig.sysFreqInMHz = CLOCK_GetFreq(kCLOCK_CoreSysClk) /
        (1000 * 1000);
    FLASH_Init(&_fcfg);
}

static int _init_flash_storage(struct storage_driver_t * sdriver) {
    (void)sdriver;
    LOG_INFO("Running flash backed storage driver");
    _storage_init_flash();

    return 0;
}

static int _read_flash_storage(struct storage_driver_t * sdriver,
                               uint8_t * buffer,
                               size_t len) {
    LOG_INFO("Read from flash backed storage driver");
    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    LOG_DEBUG("Reading from 0x%.8X, len: %d", farea->start_addr, len);

    __DSB();
    __ISB();
    int error = FLASH_Read(&_fcfg, farea->start_addr, buffer, len);
    if (error != kStatus_Success) {
        LOG_ERROR("Flash error status: %d", error);
        return -1;
    }
    return 0;
}

static int _write_flash_storage(struct storage_driver_t * sdriver,
                                uint8_t * buffer,
                                size_t len) {
    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    LOG_DEBUG("Writing len %d to 0x%.8X + 0x%X", len,
              farea->start_addr, farea->offset);

    int err = FLASH_Program(&_fcfg, farea->start_addr +
                            farea->offset, buffer,
                            len);
    if (err != kStatus_Success) {
        LOG_ERROR("Failed to write data..");
        return -1;
    }

    farea->offset += len;
    return 1;
}

static int _flush_flash_storage(struct storage_driver_t * sdriver) {
    LOG_DEBUG("Flushing storage driver");

    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);
    LOG_OK("Application size received is: %d", farea->offset);
    farea->offset = 0;
    return 0;
}

static int _erase_flash_storage(struct storage_driver_t * sdriver) {
    LOG_INFO("Erasing flash backend");
    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    uint32_t * app_code = (uint32_t *)farea->start_addr;
    uint32_t app_size = farea->size;

    LOG_INFO("Addr + size: 0x%.8X %d", app_code, app_size);
    int status = FLASH_Erase(&_fcfg, (uint32_t)app_code,
                             app_size, kFLASH_ApiEraseKey);
    if (status != kStatus_Success) {
        LOG_ERROR("Failed to erase flash");
        return -1;
    }

    status = FLASH_VerifyErase(&_fcfg, (uint32_t)app_code,
                               app_size);
    if (status != kStatus_Success) {
        LOG_ERROR("Verifying flash erase failed!");
        return -1;
    }

    LOG_OK("Verify Erased done!");
    return 0;
}

static uint32_t _crc_flash_storage(struct storage_driver_t * sdriver,
                                   __attribute__((unused)) size_t len) { // 'len' is not used here!
    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);

    uint32_t * start_addr = (uint32_t *)farea->start_addr;

    return crc_run_crc32((uint32_t)start_addr, farea->size);
}

static int _close_flash_storage(struct storage_driver_t * sdriver) {
    (void)sdriver;
    return 0;
}

static const struct storage_ops_t fops = {
    .init  = _init_flash_storage,
    .read  = _read_flash_storage,
    .write = _write_flash_storage,
    .erase = _erase_flash_storage,
    .flush = _flush_flash_storage,
    .crc   = _crc_flash_storage,
    .close = _close_flash_storage,
};

static struct storage_driver_t fdriver = {
    .name     = "iflash",
    .type     = STORAGE_FLASH_INTERNAL,
    .ops      = &fops,
    .privdata = NULL,
};

struct storage_driver_t * storage_new_flash_driver() {
    return &fdriver;
}

struct flash_area_t storage_new_flash_area(char * name,
                                           uint32_t addr,
                                           uint32_t
                                           size) {
    struct flash_area_t area = {
        .start_addr = addr,
        .size       = size,
        .offset     = 0,
    };

    strncpy(area.area_name, name, MAX_AREA_NAME);

    LOG_DEBUG("New flash area [%s] @ 0x%.8X with size %dkB",
              area.area_name,
              area.start_addr,
              (area.size / 1024));
    return area;
}

void storage_set_flash_area(struct storage_driver_t * sdriver,
                            struct flash_area_t * farea) {
    LOG_INFO("Setting flash area %s @%p", farea->area_name, farea->start_addr);
    STORAGE_SETPRIV(sdriver, farea);
}

bool storage_is_empty_partition(struct storage_driver_t * sdriver) {
    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);
    int err = FLASH_VerifyErase(&_fcfg, farea->start_addr, farea->size);

    if (err == kStatus_Success) {
        LOG_DEBUG("Partition 0x%.8X is empty", farea->start_addr);
        return true;
    }

    LOG_DEBUG("Partition not empty 0x%.8X %x, FLASH_VerifyErase ret(0x%x)",
              farea->start_addr,
              farea->size,
              err);

    return false;
}

int storage_wipe_entire_flash(struct storage_driver_t * sdriver) {
    (void)sdriver;
    LOG_INFO("Erasing flash backend");

    // should get info from sdriver
    struct flash_area_t * farea = STORAGE_GETPRIV(sdriver);
    uint32_t start = (uint32_t)farea->start_addr;
    uint32_t size = (uint32_t)farea->size;

    LOG_INFO("Addr + size: 0x%.8X + 0x%x", start, size);

    if (size % 512 != 0) {
        LOG_ERROR("erase size alignment error");
    }

    if (start % 512 != 0) {
        LOG_ERROR("erase start alignment error");
    }

    int status = FLASH_Erase(&_fcfg, start, size, kFLASH_ApiEraseKey);
    if (status != kStatus_Success) {
        LOG_ERROR("Failed to erase flash partition0");
        return -1;
    }

    status = FLASH_VerifyErase(&_fcfg, start, size);
    if (status != kStatus_Success) {
        LOG_ERROR("Verifying flash erase failed!");
        return -1;
    }

    LOG_OK("Verify Erased done!");
    return 0;
}
