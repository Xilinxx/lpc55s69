/**
 * @file i2c_hub_poc.c
 * @brief  I2C USB5807C Communication Proof-of-Concept
 *
 * This is the main source file for the I2C USB5807C Proof-of-Concept.
 * Mainly used for the implementation and testing of the I2C Driver code.
 *
 * Note; This only runs on the LPCXpresso55S69 target
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */

#include <stdio.h>
#include <stdint.h>

#include "board.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "clock_config.h"

#include "logger.h"
#include "usb5807c.h"
#include "tmds181.h"

#ifdef ENABLE_SEMIHOSTING
extern void initialise_monitor_handles(void);
#endif

#define I2C_TRANSFER_MSG_SIZE 64

struct main_context_t {
	usart_handle_t		usart_debug_handle;
	i2c_master_handle_t	i2c_mstr_handle;
};

static struct main_context_t _ctxt;

/**
 * @brief  Setup the board
 *
 * Run the setup of the selected board. BOARD_* functions change according
 * to selected target
 *
 * @returns  -1 if something fails
 */
static int _initialize_board(void)
{
	uint8_t revision = BOARD_GetRevision();

	(void)revision;                 // Probably we'll need to use this at some point

	BOARD_AppInitBootPins();        //!< Initialize Board Pinout
	BOARD_AppInitClocks();          //!< Initialize Board Clocks
	BOARD_AppInitPeripherals();     //!< Initialize Board peripherals

	logger_init();
	LOG_INFO("Running on %s", BOARD_NAME);

	return 0;
}

void _i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle,
			  status_t completionStatus, void *userData)
{
	(void)base;
	(void)handle;
	(void)completionStatus;
	(void)userData;

	return;
}

/**
 * @brief  Main function of the software
 *
 * If this function should not need more explanation that this.
 *
 * @returns Probably never...
 */
int main(void)
{
	/* Code only runs on LPCXpresso55S69 Board.. */
	assert(strcmp(BOARD_NAME, "LPCXpresso55S69") == 0);

	/* Run Board initialization */
	_initialize_board();

#ifdef ENABLE_SEMIHOSTING
	initialise_monitor_handles();
#endif

	I2C_MasterTransferCreateHandle(FLEXCOMM1_PERIPHERAL,
				       &_ctxt.i2c_mstr_handle, NULL, NULL);

	tmds181_init_ctxt(FLEXCOMM1_PERIPHERAL, &_ctxt.i2c_mstr_handle);
	usb5807c_init_ctxt(FLEXCOMM1_PERIPHERAL, &_ctxt.i2c_mstr_handle);

	usb5807c_reset();
	/* sleep(1); //!< Reset takes some time */

	/* Test write function in configuration mode */
	uint8_t vid[2] = { 0x13, 0x12 };
	usb5807c_set_usb_vid(vid);

	/* Test read functions configuration mode */
	usb5807c_retrieve_revision();
	usb5807c_retrieve_id();
	usb5807c_retrieve_usb_vid();

	usb5807c_retrieve_connect_status();

	/* Go into runtime mode */
	usb5807c_enable_runtime_with_smbus();
	/* sleep(1); */

	/* Test read functions runtime mode */
	usb5807c_retrieve_revision();
	int id = usb5807c_retrieve_id();
	int rvid = usb5807c_retrieve_usb_vid();

	LOG_INFO("id: %x vid: %x", id, rvid);

	int flexd = usb5807c_retrieve_connect_status();
	LOG_INFO("Current flex state: %d", flexd);

	usb5807c_set_upstream_port(USB5807C_FLEXPORT1); //!< Move upstream to Port 1
	flexd = usb5807c_retrieve_connect_status();
	LOG_INFO("Current flex state: %d", flexd);

/* 	sleep(5); */
	usb5807c_set_upstream_port(USB5807C_FLEXPORT0);

	int state = usb5807c_is_port_active(0);
	LOG_INFO("Port state: %x", state);

	int tmdsid = tmds181_read_dev_id();
	if (tmdsid < 0) {
		LOG_ERROR("Failed to read ID");
	}

	int pwrstate = tmds181_read_powerstate();
	if (pwrstate < 0) {
		LOG_ERROR("Failed to read powerstate");
	}

	for (;;) {
		/* usb5807c_is_port_active(0); */
		/* usb5807c_is_port_active(4); */

		/* LOG_DEBUG("Test"); */
		/* sleep(1); */
	}

	return 0; //!< Should never get here?...
}
