/**
 * @file comm_driver.h
 * @brief  COMM Driver API
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-10-29
 */

#ifndef _COMM_DRIVER_H_
#define _COMM_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef int (*comm_init_fn) (void *drv);
typedef int (*comm_write_fn)(void *drv, uint8_t *buffer, size_t len);
typedef int (*comm_read_fn) (void *drv, uint8_t *buffer, size_t *len);
typedef void (*comm_close_fn)(void *drv);

/**
 * @brief  Communication driver operations struct
 */
struct comm_ops_t {
	comm_init_fn	init;   //!< Initialize protocol driver
	comm_write_fn	write;  //!< Write to protocol driver
	comm_read_fn	read;   //!< Read from protocol driver
	comm_close_fn	close;  //!< Close protocol driver
};

#define MAX_COMM_NAME 64        //!< Max length for communication driver

/**
 * @brief  Communication driver struct
 */
struct comm_driver_t {
	bool				enabled;                //!< Driver is enabled
	char				name[MAX_COMM_NAME];    //!< Driver name
	const struct comm_ops_t *	ops;      //!< Communication ops
	struct storage_driver_t *	sdriver;  //!< Storage driver

	void *				priv_data;            //!< Driver private data
};

#define COMM_SETPRIV(drvr, new_data) (drvr)->priv_data = (void *)(new_data)

/**
 * @brief  Communication tranfer context
 */
struct comm_ctxt_t {
	bool				transfer_in_progress; //!< Is a transfer in progress?
	uint8_t		  part_nr;              //!< Partition number in progress
	bool				crc_received;         //!< Has the CRC of the transfer been received?
	// for WRQ
	uint16_t		last_blocknr;         //! Last data block number received

	// for RRQ
	size_t rom_readsize;  //! size to read from rom

	struct comm_driver_t *		parent;                 //!< The comm_driver_t used in this transfer

	struct storage_driver_t *	storage;                //!< Pointer to the storage driver used
};

#endif /* _COMM_DRIVER_H_ */
