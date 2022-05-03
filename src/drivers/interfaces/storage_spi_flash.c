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

#define SPI_MASTER SPI8
#define SPI_MASTER_IRQ LSPI_HS_IRQn // = FLEXCOMM8_IRQn
#define SPI_MASTER_CLK_SRC kCLOCK_Flexcomm8
#define SPI_MASTER_CLK_FREQ CLOCK_GetHsLspiClkFreq()
#define SPI_SSEL 0
#define SPI_MASTER_IRQHandler FLEXCOMM8_IRQHandler
#define SPI_SPOL kSPI_SpolActiveAllLow

/*******************************************************************************
 * Variables
 ******************************************************************************/
//Identification , 4byte: manufacturer, memory, capacity, device ID
static uint8_t SpiFlash_Identification[5];

/*******************************************************************************
 * Code
 ******************************************************************************/
static void _storage_init_spi_flash()
{
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
  if (error != kStatus_Success)
  {
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

static int _init_spi_flash_storage(struct storage_driver_t *sdriver)
{
  (void)sdriver;
  LOG_INFO("Running spi flash backed storage driver");
  _storage_init_spi_flash();
  return 0;
}

static int _read_spi_flash_storage(struct storage_driver_t *sdriver,
                                   uint8_t *buffer,
                                   size_t len)
{
  //LOG_INFO("Read from spi flash backed storage driver");
  struct spi_flash_area_t *farea = STORAGE_GETPRIV(sdriver);

  //LOG_DEBUG("Reading from %x, offset: %d len: %d", farea->start_addr, farea->offset, len);
  BOARD_SetSPIMux(1);
  int ret = is25xp_read((off_t)farea->start_addr + (off_t)farea->offset, len, buffer, 1);
  BOARD_SetSPIMux(0);
  //LOG_DEBUG("is25xp_read startblock: %04lx returned [%d] bytes", (long)farea->start_addr, ret);

  if (ret <= 0)
  {
    LOG_ERROR("Flash read error");
    return -1;
  }
  farea->offset += len;
  return ret;
}

static int _write_spi_flash_storage(struct storage_driver_t *sdriver,
                                    uint8_t *buffer, size_t len)
{
  struct spi_flash_area_t *farea = STORAGE_GETPRIV(sdriver);

  LOG_DEBUG("Writing len %d to 0x%x + 0x%x per 256 bytes", len,
            farea->start_addr, farea->offset);

  int pages = (int)len / 256;
  int blockNr = (int)farea->offset / 256;
  BOARD_SetSPIMux(1);
  // int err = FLASH_Program(&_fcfg, farea->start_addr + farea->offset, buffer, len);
  for (int i = 0; i < pages; i++)
  {
    is25xp_pagewrite(buffer+(i*256), blockNr+i); // per 256 bytes
  }
  BOARD_SetSPIMux(0);

  farea->offset += len;
  return 1;
}

static int _flush_spi_flash_storage(struct storage_driver_t *sdriver)
{
  LOG_DEBUG("Flushing storage driver");

  struct spi_flash_area_t *farea = STORAGE_GETPRIV(sdriver);
  LOG_OK("Application size received is: %d", farea->offset);
  farea->offset = 0;
  return 0;
}

static int _erase_spi_flash_storage(struct storage_driver_t *sdriver)
{
  LOG_INFO("Erasing spi flash backend");
  struct spi_flash_area_t *farea = STORAGE_GETPRIV(sdriver);
  //translate into is25x blocks / total mem is 128Mbit or 16Mbyte

  const long unsigned int SECTOR_SIZE = (1024 * 4);

  uint32_t *app_code = (uint32_t *)farea->start_addr;
  uint32_t app_size = farea->size;
  // 1 sector is 4Kb
  //off_t sectorNr = (off_t)farea->start_addr / (off_t)SECTOR_SIZE;
  off_t sectorNr = 0; // erase from start

  LOG_INFO("spi erase: Addr 0x%x size: 0x%X (sectors %d)", app_code, app_size,
    app_size / SECTOR_SIZE);
  BOARD_SetSPIMux(1);
  int erasedBlocks = is25xp_erase(sectorNr, app_size/SECTOR_SIZE);
  BOARD_SetSPIMux(0);

  if (erasedBlocks != (int)(app_size/SECTOR_SIZE))
  {
    LOG_ERROR("Erased blocks don't match %d<>%d",erasedBlocks,(int)(app_size/SECTOR_SIZE));
    return -1;
  }

  LOG_OK("Verify Erased done!");
  return 0;
}

static uint32_t _crc_spi_flash_storage(struct storage_driver_t *sdriver)
{
  struct spi_flash_area_t *farea = STORAGE_GETPRIV(sdriver);

  uint32_t *start_addr = (uint32_t *)farea->start_addr;

  return crc_run_crc32((uint8_t *)start_addr, farea->size);
}

static int _close_spi_flash_storage(struct storage_driver_t *sdriver)
{
  (void)sdriver;
  return 0;
}

static const struct storage_ops_t fops = {
    .init = _init_spi_flash_storage,
    .read = _read_spi_flash_storage,
    .write = _write_spi_flash_storage,
    .erase = _erase_spi_flash_storage,
    .flush = _flush_spi_flash_storage,
    .crc = _crc_spi_flash_storage,
    .close = _close_spi_flash_storage,
};

static struct storage_driver_t fdriver = {
    .name = "spiflash",
    .type = STORAGE_SPI_EXTERNAL,
    .ops = &fops,
    .privdata = NULL,
};

struct storage_driver_t *storage_new_spi_flash_driver()
{
  return &fdriver;
}

struct spi_flash_area_t storage_new_spi_flash_area(char *name, uint32_t addr, uint32_t size)
{
  struct spi_flash_area_t area = {
      .start_addr = addr,
      .size = size,
      .offset = 0,
  };
  strncpy(area.area_name, name, MAX_AREA_NAME);

  LOG_DEBUG("New spi flash area [%s] @ 0x%.8x with size %dkB",
            area.area_name,
            area.start_addr,
            (area.size / 1024));
  return area;
}

void storage_set_spi_flash_area(struct storage_driver_t *sdriver,
                                struct spi_flash_area_t *farea)
{
  LOG_INFO("Setting spi flash area %s @%p", farea->area_name, farea->start_addr);
  STORAGE_SETPRIV(sdriver, farea);
}

int storage_wipe_entire_spi_flash(struct storage_driver_t *sdriver)
{
  LOG_INFO("Wipe spi flash");
  _erase_spi_flash_storage(sdriver);
  LOG_OK("Erased done!");
  return 0;
}
