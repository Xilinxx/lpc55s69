/*******************************************************************************
 * is25xp.c
 * Driver for SPI-based IS25LPxx parts 32MBit and larger.
 *
 *
 ******************************************************************************/

/*******************************************************************************
 * Included Files
*******************************************************************************/
#include "is25xp.h"

#include "logger.h"

#include "fsl_spi.h"

/*******************************************************************************
 * Private Types
 ******************************************************************************/
#define BUFFER_SIZE (512)
static uint8_t srcBuff[BUFFER_SIZE];
static uint8_t destBuff[BUFFER_SIZE];

//static spi_transfer_t xfer = {srcBuff, destBuff, 0, kSPI_FrameAssert};

static struct is25xp_dev_s priv;

/*******************************************************************************
 * Name: is25xp_readid
 ******************************************************************************/

int is25xp_readid(uint8_t *productIdentification,uint8_t verbose)
{
  uint16_t manufacturer;
  uint16_t memory;
  uint16_t capacity;
  uint16_t deviceID;
  spi_transfer_t xfer;
  uint8_t productIdIndex = 0;

  /* Send the "Read ID (RDID)" command and read the first three ID bytes */
  xfer.txData = srcBuff;
  xfer.rxData = destBuff;
  xfer.configFlags = kSPI_FrameAssert;
  srcBuff[0] = IS25_RDID;
  srcBuff[1] = IS25_DUMMY; // Manuf
  srcBuff[2] = IS25_DUMMY; // Mem
  srcBuff[3] = IS25_DUMMY; // Capacity
  xfer.dataSize = 4;
  SPI_MasterTransferBlocking(SPI8, &xfer);

  manufacturer = destBuff[1];
  memory = destBuff[2];
  capacity = destBuff[3];
  productIdentification[productIdIndex++] = destBuff[1];
  productIdentification[productIdIndex++] = destBuff[2];
  productIdentification[productIdIndex++] = destBuff[3];

  /* Send the "IS25_RDMDID" command and read the Device ID */
  xfer.txData = srcBuff;
  xfer.rxData = destBuff;
  xfer.configFlags = kSPI_FrameAssert;
  srcBuff[0] = IS25_RDMDID;
  srcBuff[1] = IS25_DUMMY;
  srcBuff[2] = IS25_DUMMY;
  srcBuff[3] = 1; // Setting 1 makes the first reply to be ID instead of Manuf
  srcBuff[4] = IS25_DUMMY;
  srcBuff[5] = IS25_DUMMY;
  xfer.dataSize = 6;
  SPI_MasterTransferBlocking(SPI8, &xfer);

  deviceID = destBuff[4];
  productIdentification[productIdIndex++] = destBuff[4];
  if (verbose)
  {
  LOG_DEBUG("is25xp - manufacturer: %02x memory: %02x capacity: %02x device ID: %02x",
            manufacturer, memory, capacity, deviceID);
  }
  if (manufacturer != IS25_MANUFACTURER)
    LOG_WARN("Manuf Id is not %02X", IS25_MANUFACTURER);
  if (deviceID != IS25_DEVICE_ID)
    LOG_WARN("Device Id is not %02X", IS25_DEVICE_ID);
  if (capacity != IS25_IS25LP128_CAPACITY)
    LOG_WARN("Capacity is not %02X", IS25_IS25LP128_CAPACITY);

  /* Check for a valid manufacturer and memory type */

  if (manufacturer == IS25_MANUFACTURER && memory == IS25_MEMORY_TYPE)
  {
    /* Okay.. is it a FLASH capacity that we understand? */

    if (capacity == IS25_IS25LP064_CAPACITY)
    {
      /* Save the FLASH geometry */

      priv.sectorshift = IS25_IS25LP064_SECTOR_SHIFT;
      priv.nsectors = IS25_IS25LP064_NSECTORS;
      priv.pageshift = IS25_IS25LP064_PAGE_SHIFT;
      priv.npages = IS25_IS25LP064_NPAGES;
      return 0;
    }
    else if (capacity == IS25_IS25LP128_CAPACITY)
    {
      /* Save the FLASH geometry */

      priv.sectorshift = IS25_IS25LP128_SECTOR_SHIFT;
      priv.nsectors = IS25_IS25LP128_NSECTORS;
      priv.pageshift = IS25_IS25LP128_PAGE_SHIFT;
      priv.npages = IS25_IS25LP128_NPAGES;
      return 0;
    }
  }
  else
  {
    LOG_ERROR("is25xp - Wrong device detected");
  }

  return -ENODEV;
}

uint8_t is25xp_read_status_register(uint8_t verbose)
{
  static uint8_t status;
  spi_transfer_t xfer;


  xfer.txData = srcBuff;
  xfer.rxData = destBuff;
  xfer.configFlags = kSPI_FrameAssert;
  srcBuff[0] = IS25_RDSR;
  srcBuff[1] = IS25_DUMMY;
  xfer.dataSize = 2;
  SPI_MasterTransferBlocking(SPI8, &xfer);

  if (status != destBuff[1])
  {
    //LOG_DEBUG("is25xp - Status Reg change: %02x->%02x", status, destBuff[1]);
    status = destBuff[1];
  }

  if (verbose)
    LOG_DEBUG("is25xp - Status Reg: %02x", status);

  return status;
}

/*******************************************************************************
 * Name: is25xp_waitwritecomplete
 ******************************************************************************/

void is25xp_waitwritecomplete()
{
  uint8_t status;
  uint32_t delay = 0;

  /* Loop as long as the memory is busy with a write cycle */
  do
  {
    status = is25xp_read_status_register(false);

    /* Given that writing could take up to few tens of milliseconds, and erasing
       * could take more.  The following short delay in the "busy" case will allow
       * other peripherals to access the SPI bus.
       */

    if ((status & IS25_SR_WIP) != 0)
    {
      SDK_DelayAtLeastUs(100000, 96000000U);
      delay++;
    }
  } while ((status & IS25_SR_WIP) != 0);

  priv.lastwaswrite = false;

  if (delay)
    LOG_DEBUG("is25xp_waitwritecomplete Complete (%dx100ms)", delay);
}

/*******************************************************************************
 * Name:  is25xp_writeenable
 ******************************************************************************/

void is25xp_writeenable()
{

  /* Send "Write Enable (WREN)" command */
  SPI_WriteData(SPI8, (uint16_t)(IS25_WREN), kSPI_FrameAssert);

  //LOG_DEBUG("is25xp_write Enabled");
}

/*******************************************************************************
 * Name: is25xp_unprotect
 ******************************************************************************/

void is25xp_unprotect()
{
  /* Make writeable */

  is25xp_writeenable();

  /* Send "Write status (WRSR)" */

  SPI_WriteData(SPI8, (uint16_t)(IS25_WRSR), kSPI_FrameAssert);

  /* Followed by the new status value */
  SPI_WriteData(SPI8, (uint16_t)(0), kSPI_FrameAssert);
}

/*******************************************************************************
 * Name:  is25xp_sectorerase
 ******************************************************************************/

void is25xp_sectorerase(off_t sector, uint8_t type)
{
  off_t offset;

  offset = sector << priv.sectorshift; //12 shifts for 4k areas

  LOG_DEBUG("sector: %08lx - type(%02X)", (long)sector, type);

  /* Wait for any preceding write to complete.  We could simplify things by
   * perform this wait at the end of each write operation (rather than at
   * the beginning of ALL operations), but have the wait first will slightly
   * improve performance.
   */

  is25xp_waitwritecomplete();

  /* Send write enable instruction */

  is25xp_writeenable();

  /* Send the "Sector Erase (SE)" or Sub-Sector Erase (SSE) instruction
   * that was passed in as the erase type.
   */
  srcBuff[0] = type;
  srcBuff[1] = (uint8_t)((offset >> 16) & 0xff);
  srcBuff[2] = (uint8_t)((offset >> 8) & 0xff);
  srcBuff[3] = (uint8_t)(offset & 0xff);

  spi_transfer_t xfer;
  xfer.txData = srcBuff;
  xfer.rxData = destBuff;
  xfer.configFlags = kSPI_FrameAssert;
  xfer.dataSize = 4;
  SPI_MasterTransferBlocking(SPI8, &xfer);

  priv.lastwaswrite = true;

  is25xp_waitwritecomplete();

  LOG_DEBUG("Erased sector %08lx", sector);
}

/*******************************************************************************
 * Name:  is25xp_bulkerase
 ******************************************************************************/

int is25xp_bulkerase()
{
  /* Wait for any preceding write to complete.  We could simplify things by
   * perform this wait at the end of each write operation (rather than at
   * the beginning of ALL operations), but have the wait first will slightly
   * improve performance.
   */

  is25xp_waitwritecomplete();

  /* Send write enable instruction */

  is25xp_writeenable();

  /* Send the "Chip Erase (CER)" instruction */

  SPI_WriteData(SPI8, (uint16_t)IS25_CER, kSPI_FrameAssert);

  is25xp_waitwritecomplete();

  LOG_DEBUG("SPI Flash Erased\n");
  return 0;
}

/*******************************************************************************
 * Name:  is25xp_pagewrite
 * Parameters:
 *      buffer    - Pointer to a buffer storing the bytes to be written.
 *      page      - The memory address where data will be written
 *
 * The Page Program (PP) instruction allows up to 256 bytes data to be
 * programmed into memory in a single operation
 ******************************************************************************/

void is25xp_pagewrite(uint8_t *buffer, off_t page)
{
  off_t offset = page << priv.pageshift;

  //LOG_DEBUG("page: %08lx offset: %08lx\n", (long)page, (long)offset);

  /* Wait for any preceding write to complete.  We could simplify things by
   * perform this wait at the end of each write operation (rather than at
   * the beginning of ALL operations), but have the wait first will slightly
   * improve performance.
   */

  is25xp_waitwritecomplete();

  /* Enable the write access to the FLASH */

  is25xp_writeenable();

  srcBuff[0] = IS25_PP;
  srcBuff[1] = (uint8_t)((offset >> 16) & 0xff);
  srcBuff[2] = (uint8_t)((offset >> 8) & 0xff);
  srcBuff[3] = (uint8_t)(offset & 0xff);

  spi_transfer_t xfer;
  xfer.txData = srcBuff;
  xfer.rxData = destBuff;
  xfer.configFlags = kSPI_FrameAssert;
  xfer.dataSize = 4 + 256;
  for (int i = 0; i < 256; i++)
  {
    srcBuff[4 + i] = buffer[i];
  }
  SPI_MasterTransferBlocking(SPI8, &xfer);

  priv.lastwaswrite = true;

  //LOG_DEBUG("Page Written\n");
}

/*******************************************************************************
 * Name:  is25xp_bytewrite
 ******************************************************************************/

#ifdef CONFIG_MTD_BYTE_WRITE
static inline void is25xp_bytewrite(const uint8_t *buffer, off_t offset, uint16_t count)
{
  LOG_DEBUG("offset: %08lx  count:%d\n", (long)offset, count);

  /* Wait for any preceding write to complete.  We could simplify things by
   * perform this wait at the end of each write operation (rather than at
   * the beginning of ALL operations), but have the wait first will slightly
   * improve performance.
   */

  is25xp_waitwritecomplete();

  /* Enable the write access to the FLASH */

  is25xp_writeenable();

  srcBuff[0] = IS25_PP;
  srcBuff[1] = (uint8_t)((offset >> 16) & 0xff);
  srcBuff[2] = (uint8_t)((offset >> 8) & 0xff);
  srcBuff[3] = (uint8_t)(offset & 0xff);

  spi_transfer_t xfer;
  xfer.txData = srcBuff;
  xfer.rxData = destBuff;
  xfer.configFlags = kSPI_FrameAssert;
  xfer.dataSize = (size_t)(4 + count);
  for (int i = 0; i < count; i++)
  {
    srcBuff[4 + i] = buffer[i];
  }
  SPI_MasterTransferBlocking(SPI8, &xfer);

  priv.lastwaswrite = true;

  LOG_DEBUG("%d Written\n", count);
}
#endif

/*******************************************************************************
 * Name: is25xp_erase
 ******************************************************************************/

int is25xp_erase(off_t startblock, size_t nblocks)
{
  size_t blocksleft = nblocks;

  LOG_DEBUG("startblock: %08lx nblocks: %d\n", (long)startblock, (int)nblocks);

  /* Lock access to the SPI bus until we complete the erase */
  while (blocksleft > 0)
  {
    size_t sectorboundry;
    size_t blkper;

    is25xp_waitwritecomplete();

    /* We will erase in either 4K sectors or 32K or 64K blocks depending
       * on the largest unit we can use given the startblock and nblocks.
       * This will reduce erase time (in the event we have partitions
       * enabled and are doing a bulk erase which is translated into
       * a block erase operation).
       */

    /* Test for 64K alignment */

    blkper = 64 / 4;
    sectorboundry = ((size_t)startblock + blkper - 1) / blkper;
    sectorboundry *= blkper;

    /* If we are on a sector boundry and have at least a full sector
       * of blocks left to erase, then we can do a full sector erase.
       */

    if ((size_t)startblock == sectorboundry && blocksleft >= blkper)
    {
      /* Do a 64k block erase */

      is25xp_sectorerase(startblock, IS25_BE64);
      startblock += (off_t)blkper;
      blocksleft -= blkper;
      continue;
    }

    /* Test for 32K block alignment */

    blkper = 32 / 4;
    sectorboundry = ((size_t)startblock + blkper - 1) / blkper;
    sectorboundry *= blkper;

    if ((size_t)startblock == sectorboundry && blocksleft >= blkper)
    {
      /* Do a 32k block erase */

      is25xp_sectorerase(startblock, IS25_BE32);
      startblock += (off_t)blkper;
      blocksleft -= blkper;
      continue;
    }
    else
    {
      /* Just do a sector erase */
      is25xp_sectorerase(startblock, IS25_SE);
      startblock++;
      blocksleft--;
      continue;
    }
  }

  return (int)nblocks;
}

/*******************************************************************************
 * Name: is25xp_bread
 ******************************************************************************/

ssize_t is25xp_bread(off_t startblock, size_t nblocks, uint8_t *buffer,uint8_t verbose)
{
  ssize_t nbytes;

  if (priv.pageshift != 8)
    LOG_ERROR("Initialisation error , IS25LP128 requires 8");
  if (verbose)
    LOG_DEBUG("startblock: %08lx nblocks: %d (pageshift=%d)", (long)startblock, (int)nblocks, priv.pageshift);

  /* On this device, we can handle the block read just like the byte-oriented read */
  nbytes = is25xp_read(startblock << priv.pageshift, nblocks << priv.pageshift, buffer, verbose);
  if (nbytes > 0)
  {
    return nbytes >> priv.pageshift;
  }

  return (int)nbytes;
}

/*******************************************************************************
 * Name: is25xp_bwrite
 ******************************************************************************/

ssize_t is25xp_bwrite(off_t startblock, size_t nblocks, uint8_t *buffer, uint8_t verbose)
{
  size_t blocksleft = nblocks;
  size_t pagesize = (size_t)(1 << priv.pageshift);

  if (verbose)
    LOG_DEBUG("startblock: %08lx nblocks: %d", (long)startblock, (int)nblocks);

  /* Lock the SPI bus and write each page to FLASH */

  while (blocksleft-- > 0)
  {
    is25xp_pagewrite(buffer, startblock);
    buffer += pagesize;
    startblock++;
  }

  return (ssize_t)nblocks;
}

// void masterCallback(SPI_Type *base, spi_master_handle_t *masterHandle, status_t status, void *userData)
// {
//   LOG_DEBUG("masterCallback");
// }

/*******************************************************************************
 * Name: is25xp_read
 ******************************************************************************/

ssize_t is25xp_read(off_t offset, size_t nbytes, uint8_t *buffer, uint8_t verbose)
{
  if (verbose)
    LOG_DEBUG("offset: %08lx nbytes: %d", (long)offset, (int)nbytes);

  /* Wait for any preceding write to complete.  We could simplify things by
   * perform this wait at the end of each write operation (rather than at
   * the beginning of ALL operations), but have the wait first will slightly
   * improve performance.
   */

  if (priv.lastwaswrite)
  {
    is25xp_waitwritecomplete();
  }

  srcBuff[0] = IS25_READ;
  srcBuff[1] = (uint8_t)((offset >> 16) & 0xff); //Address bytes
  srcBuff[2] = (uint8_t)((offset >> 8) & 0xff);
  srcBuff[3] = (uint8_t)(offset & 0xff);

  spi_transfer_t xfer;
  xfer.txData = srcBuff;
  xfer.rxData = destBuff;
  xfer.configFlags = kSPI_FrameAssert;
  xfer.dataSize = nbytes + 4;
  SPI_MasterTransferBlocking(SPI8, &xfer);
  for (size_t i = 0; i < nbytes; i++)
  {
    buffer[i] = destBuff[4 + i];
  }

  if (verbose)
    LOG_DEBUG("return nbytes: %d", (int)nbytes);

  return (ssize_t)nbytes;
}

/*******************************************************************************
 * Name: is25xp_write
 ******************************************************************************/

#ifdef CONFIG_MTD_BYTE_WRITE
ssize_t is25xp_write(off_t offset, size_t nbytes, const uint8_t *buffer)
{
  struct is25xp_dev_s *_priv = &priv;
  int startpage;
  int endpage;
  int count;
  int index;
  int pagesize;
  int bytestowrite;

  LOG_DEBUG("offset: %08lx nbytes: %d\n", (long)offset, (int)nbytes);

  /* We must test if the offset + count crosses one or more pages
   * and perform individual writes.  The devices can only write in
   * page increments.
   */

  startpage = offset / (1 << _priv->pageshift);
  endpage = (offset + (int)nbytes) / (1 << _priv->pageshift);

  if (startpage == endpage)
  {
    /* All bytes within one programmable page.  Just do the write. */

    is25xp_bytewrite(buffer, offset, (uint16_t)nbytes);
  }
  else
  {
    /* Write the 1st partial-page */

    count = (int)nbytes;
    pagesize = (1 << _priv->pageshift);
    bytestowrite = pagesize - (offset & (pagesize - 1));
    is25xp_bytewrite(buffer, offset, (uint16_t)bytestowrite);

    /* Update offset and count */

    offset += bytestowrite;
    count -= bytestowrite;
    index = bytestowrite;

    /* Write full pages */

    while (count >= pagesize)
    {
      is25xp_bytewrite(&buffer[index], offset, (uint16_t)pagesize);

      /* Update offset and count */

      offset += pagesize;
      count -= pagesize;
      index += pagesize;
    }

    /* Now write any partial page at the end */

    if (count > 0)
    {
      is25xp_bytewrite(&buffer[index], offset, (uint16_t)count);
    }

    _priv->lastwaswrite = true;
  }

  return (ssize_t)nbytes;
}
#endif // CONFIG_MTD_BYTE_WRITE

/***	is25xp_ReleasePowerDownGetDeviceID
**
**	Description:
**		This function implements the Release from Deep-Power-Down
*/
void is25xp_ReleasePowerDownGetDeviceID()
{
  SPI_WriteData(SPI8, (uint16_t)IS25_RDID_RDPD, kSPI_FrameAssert);
}

/*** is25xp_PowerDown
 **
 **	Description:
 **   The Enter Deep Power-down (DP) instruction is for setting the device on
 **   the minimizing the power consumption.
 */
void is25xp_PowerDown()
{
  SPI_WriteData(SPI8, (uint16_t)IS25_DP, kSPI_FrameAssert);
}
