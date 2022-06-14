/**
 * @file storage.h
 * @brief  Storage abstraction
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-10-13
 */

#ifndef _GPMCU_STORAGE_H_
#define _GPMCU_STORAGE_H_

#include <stdint.h>

struct storage_driver_t;

/** storage init typedef */
typedef int (* storage_init)(struct storage_driver_t * sdriver);

/** storage read typedef */
typedef int (* storage_read)(struct storage_driver_t * sdriver, uint8_t * data, size_t len);

/** storage write typedef */
typedef int (* storage_write)(struct storage_driver_t * sdriver, uint8_t * data, size_t len);

/** storage erase typedef */
typedef int (* storage_erase)(struct storage_driver_t * sdriver);

/** storage flush typedef */
typedef int (* storage_flush)(struct storage_driver_t * sdriver);

/** storage flush typedef */
typedef uint32_t (* storage_crc)(struct storage_driver_t * sdriver);

/** storage close typedef */
typedef int (* storage_close)(struct storage_driver_t * sdriver);

/**
 * @brief  Storage ops structure
 */
struct storage_ops_t {
    storage_init init;          // !< Init fn pointer
    storage_read read;          // !< Read fn pointer
    storage_write write;        // !< Write fn pointer
    storage_erase erase;        // !< Erase fn pointer
    storage_flush flush;        // !< Flush fn pointer
    storage_crc crc;              // !< CRC fn pointer
    storage_close close;        // !< Close fn pointer
};

/**
 * @brief  Storage type
 */
typedef enum {
    STORAGE_FLASH_INTERNAL,     // !< Internal flash
    STORAGE_SPI_EXTERNAL,       // !< External flash via SPI
}storage_type_t;

/**
 * @brief Storage driver definition
 */
struct storage_driver_t {
    char * name;                                        // !< Name of the driver
    storage_type_t type;                                // !< Storage type

    const struct storage_ops_t * ops;                   // !< Storage operations

    void * privdata;                                    // !< Driver private data
};

#define STORAGE_SETPRIV(sdr, dta) (sdr)->privdata = (dta)
#define STORAGE_GETPRIV(sdr)      (sdr)->privdata

/**
 * @brief Init a given storage driver
 *
 * @param driver The driver to be initialized
 *
 * @returns 0 or -1 if failed
 */
static inline int storage_init_storage(struct storage_driver_t * driver) {
    if (!driver) {
        return -1;
    }
    if (!driver->ops || !driver->ops->init) {
        return -1;
    }
    return driver->ops->init(driver);
}

/**
 * @brief Read some data from a storage driver
 *
 * @param driver The driver from which we'll read
 * @param data Data buffer in which we will read the data
 * @param len Length to be read
 *
 * @returns The length read or -1 if failed
 */
static inline int storage_read_data(struct storage_driver_t * driver,
                                    uint8_t * data, size_t len) {
    if (!driver) {
        return -1;
    }
    if (!driver->ops || !driver->ops->read) {
        return -1;
    }
    return driver->ops->read(driver, data, len);
}

/**
 * @brief Write some data to a given storage driver
 *
 * @param driver The driver to which we'll write
 * @param data Data to be written
 * @param len Length of the data to be written
 *
 * @returns The length written or -1 if failed
 */
static inline int storage_write_data(struct storage_driver_t * driver,
                                     uint8_t * data, size_t len) {
    if (!driver) {
        return -1;
    }
    if (!driver->ops || !driver->ops->write) {
        return -1;
    }
    return driver->ops->write(driver, data, len);
}

/**
 * @brief Flush a given storage driver
 *
 * @param driver The driver to which we'll erase
 *
 * @returns 0 or -1 if failed
 */
static inline int storage_flush_storage(struct storage_driver_t * driver) {
    if (!driver) {
        return -1;
    }
    if (!driver->ops || !driver->ops->flush) {
        return -1;
    }
    return driver->ops->flush(driver);
}

/**
 * @brief Erase a given storage driver
 *
 * @param driver The driver to which we'll erase
 *
 * @returns 0 or -1 if failed
 */
static inline int storage_erase_storage(struct storage_driver_t * driver) {
    if (!driver) {
        return -1;
    }
    if (!driver->ops || !driver->ops->erase) {
        return -1;
    }
    return driver->ops->erase(driver);
}

/**
 * @brief CRC a given storage driver
 *
 * @param driver The driver to which we'll crc
 * @param len  0: for complete partition size ,
 *             file length: for spi file crc (or 0 for all partition)
 *
 * @returns crc or -1 if failed
 */
static inline uint32_t storage_crc_storage(struct storage_driver_t * driver,
                                           __attribute__((unused)) size_t len) {
    if (!driver) {
        return 0;
    }
    if (!driver->ops || !driver->ops->crc) {
        return 0;
    }
    return driver->ops->crc(driver);
}

/**
 * @brief  Close a given storage driver
 *
 * @param driver The driver that will be closed
 *
 * @returns  -1 if failed, otherwise 0
 */
static inline int storage_close_storage(struct storage_driver_t * driver) {
    if (!driver) {
        return -1;
    }
    if (!driver->ops || !driver->ops->close) {
        return -1;
    }
    return driver->ops->close(driver);
}

#endif /* _GPMCU_STORAGE_H_ */
