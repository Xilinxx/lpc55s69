#ifndef __SPI_IS25XP_H__
#define __SPI_IS25XP_H__

/*******************************************************************************
 * is25xp.h
 * Driver for SPI-based IS25LPxx parts 32MBit and larger.
 * Derived from,
 * https://github.com/robbie-cao/nuttx/blob/master/drivers/mtd/is25xp.c
 ******************************************************************************/

/*******************************************************************************
 * Included Files
 ******************************************************************************/

#include <sys/types.h>
#include <errno.h>

/*******************************************************************************
 * Pre-processor Definitions
 ******************************************************************************/

#define CONFIG_MTD_BYTE_WRITE       1

/* Configuration **************************************************************/
/* Per the data sheet, IS25xP parts can be driven with either SPI mode 0 (CPOL=0 and
 * CPHA=0) or mode 3 (CPOL=1 and CPHA=1).  So you may need to specify
 * CONFIG_IS25XP_SPIMODE to select the best mode for your device.  If
 * CONFIG_IS25XP_SPIMODE is not defined, mode 0 will be used.
 */

#ifndef CONFIG_IS25XP_SPIMODE
#define CONFIG_IS25XP_SPIMODE       SPIDEV_MODE0
#endif

/* SPI Frequency.  May be up to 50MHz. */

#ifndef CONFIG_IS25XP_SPIFREQUENCY
#define CONFIG_IS25XP_SPIFREQUENCY  20000000
#endif

/* IS25 Registers *************************************************************/
/* Indentification register values */

#define IS25_MANUFACTURER           0x9d
#define IS25_MEMORY_TYPE            0x60
#define IS25_DEVICE_ID              0x17

/*  IS25LP064 capacity is 8,388,608 bytes:
 *  (2,048 sectors) * (4,096 bytes per sector)
 *  (32,768 pages) * (256 bytes per page)
 */

#define IS25_IS25LP064_CAPACITY     0x17
#define IS25_IS25LP064_SECTOR_SHIFT 12      /* Sector size 1 << 12 = 4,096 */
#define IS25_IS25LP064_NSECTORS     2048
#define IS25_IS25LP064_PAGE_SHIFT   8       /* Page size 1 << 8 = 256 */
#define IS25_IS25LP064_NPAGES       32768

/*  IS25LP128 capacity is 16,777,216 bytes:
 *  (4,096 sectors) * (4,096 bytes per sector)
 *  (65,536 pages) * (256 bytes per page)
 */

#define IS25_IS25LP128_CAPACITY     0x18
#define IS25_IS25LP128_SECTOR_SHIFT 12      /* Sector size 1 << 12 = 4,096 */
#define IS25_IS25LP128_NSECTORS     4096
#define IS25_IS25LP128_PAGE_SHIFT   8       /* Page size 1 << 8 = 256 */
#define IS25_IS25LP128_NPAGES       65536
#define IS25_IS25XP_BYTES_PER_PAGE  256

/* Instructions */
/*      Command        Value      N Description             Addr Dummy  Data  */
#define IS25_WREN                   0x06 /* 1 Write Enable              0   0     0     */
#define IS25_WRDI                   0x04 /* 1 Write Disable             0   0     0     */
#define IS25_RDID                   0x9f /* 1 Read Identification       0   0     1-3   */
#define IS25_RDMDID                 0x90 /* 1 Read Manuf & Device id    0   0     1-2   */
#define IS25_RDSR                   0x05 /* 1 Read Status Register      0   0     >=1   */
// #define IS25_EWSR    0x50    /* 1 Write enable status       0   0     0     */
#define IS25_WRSR                   0x01 /* 1 Write Status Register     0   0     1     */
#define IS25_READ                   0x03 /* 1 Read Data Bytes           3   0     >=1   */
#define IS25_FAST_READ              0x0b /* 1 Higher speed read         3   1     >=1   */
#define IS25_PP                     0x02 /* 1 Page Program              3   0     1-256 */
#define IS25_SE                     0x20 /* 1 Sector Erase              3   0     0     */
#define IS25_SE_                    0xd7 /* 1 Sector Erase              3   0     0     */
#define IS25_BE32                   0x52 /* 2 32K Block Erase           3   0     0     */
#define IS25_BE64                   0xD8 /* 2 64K Block Erase           3   0     0     */
#define IS25_CER                    0xC7 /* 1 Chip Erase                0   0     0     */
#define IS25_DP                     0xB9 /* 1 Deep Power Down           0   0     0     */
#define IS25_RDID_RDPD              0xAB /* 1 RELEASE DEEP POWER DOWN   0   0     0     */
#define IS25_RDUID                  0x4B /* Read unique id number */

/* NOTE 1: All parts.
 * NOTE 2: In IS25XP terminology, 0x52 and 0xd8 are block erase and 0x20
 *         is a sector erase.  Block erase provides a faster way to erase
 *         multiple 4K sectors at once.
 */

/* Status register bit definitions */

#define IS25_SR_WIP                 (1 << 0)             /* Bit 0: Write in progress bit */
#define IS25_SR_WEL                 (1 << 1)             /* Bit 1: Write enable latch bit */
#define IS25_SR_BP_SHIFT            (2)                  /* Bits 2-5: Block protect bits */
#define IS25_SR_BP_MASK             (15 << IS25_SR_BP_SHIFT)
#  define IS25_SR_BP_NONE           (0 << IS25_SR_BP_SHIFT) /* Unprotected */
#  define IS25_SR_BP_UPPER128th     (1 << IS25_SR_BP_SHIFT) /* Upper 128th */
#  define IS25_SR_BP_UPPER64th      (2 << IS25_SR_BP_SHIFT) /* Upper 64th */
#  define IS25_SR_BP_UPPER32nd      (3 << IS25_SR_BP_SHIFT) /* Upper 32nd */
#  define IS25_SR_BP_UPPER16th      (4 << IS25_SR_BP_SHIFT) /* Upper 16th */
#  define IS25_SR_BP_UPPER8th       (5 << IS25_SR_BP_SHIFT) /* Upper 8th */
#  define IS25_SR_BP_UPPERQTR       (6 << IS25_SR_BP_SHIFT) /* Upper quarter */
#  define IS25_SR_BP_UPPERHALF      (7 << IS25_SR_BP_SHIFT) /* Upper half */
#  define IS25_SR_BP_ALL            (8 << IS25_SR_BP_SHIFT) /* All sectors */
#define IS25_SR_QE                  (1 << 6)             /* Bit 6: Quad (QSPI) enable bit */
#define IS25_SR_SRWD                (1 << 7)             /* Bit 7: Status register write protect */

#define IS25_DUMMY                  0xa5

/*******************************************************************************
 * Private Types
 ******************************************************************************/
// This struct is initialized by reading the id from the chip
struct is25xp_dev_s {
    uint8_t sectorshift; /* 12 */
    uint8_t pageshift;  /* 8 */
    uint16_t nsectors;  /* 2,048 or 4,096 */
    uint32_t npages;    /* 32,768 or 65,536 */
    uint8_t lastwaswrite; /* Indicates if last operation was write */
};

/*******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

/* Helpers */

/*
 * @brief is25xp_readid
 * For setting up the memory structure we start with reading the device id
 * @return 0:succes , -ENODEV: for unrecognized device
 */
int is25xp_readid(uint8_t * productIdentification, uint8_t verbose);

/*
 * @brief is25xp_read_status_register
 * SPI's status register
 * @return status register
 */
uint8_t is25xp_read_status_register(uint8_t verbose);

void is25xp_waitwritecomplete();
void is25xp_writeenable();
void is25xp_sectorerase(off_t offset, uint8_t type);

/*
 * @brief is25xp_bulkerase
 * Erase whole SPI Flash chip
 * @param none
 */
int is25xp_bulkerase();

/*
 * @brief is25xp_pagewrite
 * Spi page write
 * @param data - uint8_t * buffer
 * @param offset - offset
 */
void is25xp_pagewrite(uint8_t * buffer, off_t offset);

/*
 * @brief is25xp_erase
 * Spi page erase
 * @param startblock - start block
 * @param nblocks - number of blocks
 */
int is25xp_erase(off_t startblock, size_t nblocks);

ssize_t is25xp_bread(off_t startblock, size_t nblocks, uint8_t * buffer, uint8_t verbose);
ssize_t is25xp_bwrite(off_t startblock, size_t nblocks, uint8_t * buf, uint8_t verbose);
ssize_t is25xp_read(off_t offset, size_t nbytes, uint8_t * buffer, uint8_t verbose);
#ifdef CONFIG_MTD_BYTE_WRITE
ssize_t is25xp_write(off_t offset, size_t nbytes, const uint8_t * buffer);
#endif
void is25xp_ReleasePowerDownGetDeviceID();
void is25xp_PowerDown();

#endif // __SPI_IS25XP_H__