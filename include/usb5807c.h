/**
 * @file usb5807c.h
 * @brief  Microchip USB5807C Device driver
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 *
 * @date 2020-07-28
 */

#ifndef _GP_DRV_USB5807C_
#define _GP_DRV_USB5807C_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "fsl_i2c.h"

#define USB5807C_MSB                  0xff00    // !< MSB Helper
#define USB5807C_LSB                  0x00ff    // !< LSB Helper

#define USB5807C_I2C_ADDR             0x2D      // !< I2C Address for USB5807C hub
#define USB5807C_MAX_DATALEN          128       // !< Maximum length for a data chunk

#define USB5807C_RAM_START            0x00      // !< RAM Address start
#define USB5807C_RAM_ADDR_OFF         0x02      // !< RAM Read memory offset

#define USB5807C_READ_CMD             0x01      // !< Read command
#define USB5807C_READ_CONF_REG_LEN    4         // !< Length of read command
#define USB5807C_WRITE_CMD            0x00      // !< Write command
#define USB5807C_WRITE_CONF_REG_LEN   4         // !< Lenght of write command

#define USB5807C_REG_CONF_CL          0         // !< Buffer offset for command length
#define USB5807C_REG_CONF_RW          1         // !< Buffer offset for R/W Byte
#define USB5807C_REG_CONF_DC          2         // !< Buffer offset for data count
#define USB5807C_REG_CONF_RM          3         // !< Buffer offset for RAM Register MSB
#define USB5807C_REG_CONF_RL          4         // !< Buffer offset for RAM Register LSB
#define USB5807C_REG_CONF_D0          5         // !< Buffer offset for Data byte 0
#define USB5807C_REG_CONF_D1          6         // !< Buffer offset for Data byte 1

#define USB5807C_PORT1                (1 << 1)  // !< Offset for USB port 1
#define USB5807C_PORT2                (1 << 2)  // !< Offset for USB port 2
#define USB5807C_PORT3                (1 << 3)  // !< Offset for USB port 3
#define USB5807C_PORT4                (1 << 4)  // !< Offset for USB port 4
#define USB5807C_PORT5                (1 << 5)  // !< Offset for USB port 5
#define USB5807C_PORT6                (1 << 6)  // !< Offset for USB port 6
#define USB5807C_PORT7                (1 << 7)  // !< Offset for USB port 7

#define USB5807C_CMD_RESET            0x9936    // !< Reset command code
#define USB5807C_CMD_ATTACH           0xAA55    // !< Go to runtime without SMBus
#define USB5807C_CMD_ATTACH_SMBUS     0xAA56    // !< Go to runtime with SMBus
#define USB5807C_CMD_CONF_REG_ACCESS  0x9937    // !< Run memory command

#define USB5807C_FLEXPORT0            0x0       // !< Upstream port 0
#define USB5807C_FLEXPORT1            0x1       // !< Upstream port 1

#define USB5807C_READ_DATA_LEN_OFFSET 0         // !< Data offset from a read command
#define USB5807C_READ_DATA_OFFSET     1         // !< Data offset from a read command

/*
 * @brief USB5807C context (Runtime data)
 */
struct usb5807c_ctxt_t {
    I2C_Type * i2c_base;                        // !< Peripherial base addr
    i2c_master_handle_t * i2c_handle;           // !< I2c Connection handle
    i2c_master_transfer_t i2c_transfer;         // !< Main I2C shared transfer object

    bool in_runtime;                            // !< Is the hub in runtime mode?
    uint8_t curr_port_status;                           // !< Current port status
};

/**
 * @brief  Initialize the USB hub context
 *
 * @param base Peripherial base addr for the I2C interface used
 * @param handle I2C master handle used
 *
 * @warning This function must be run before any other functions can be used
 */
void usb5807c_init_ctxt(I2C_Type * base, i2c_master_handle_t * handle);

/**
 * @brief  Reset the USB hub
 *
 * @returns  -1 if failed otherwise 0
 */
int usb5807c_reset();

/**
 * @brief  Go to runtime mode without any configuration interfaces
 *
 * @returns  -1 if failed, otherwise 0
 */
int usb5807c_enable_runtime();

/**
 * @brief  Go to runtime mode with SMBus configuration interface
 *
 * @returns  -1 if failed, otherwise 0
 */
int usb5807c_enable_runtime_with_smbus();

/**
 * @brief  Run the configuration register access command
 *
 * @returns  -1 if failed, other 0
 */
int usb5807c_run_config_reg_access();


/**
 * @brief  Retrieve the revision of the Hub
 *
 * @returns -1 if failed, other the hub revision
 */
int usb5807c_retrieve_revision();

/**
 * @brief  Retrieve the hub id
 *
 * @returns  -1 if failed, other the hub id
 */
int usb5807c_retrieve_id();

/**
 * @brief  Retrieve the Flex Connect status
 *
 * @returns  -1 if failed, other the port to which the Upstream port is connected
 */
int usb5807c_retrieve_connect_status();

/**
 * @brief  Set the FlexConnect upstream port
 *
 * @param port USB5807C_FLEXPORT0 or USB5807C_FLEXPORT1
 *
 * @returns  -1 if failed, other 0
 */
int usb5807c_set_upstream_port(uint8_t port);

/**
 * @brief  Set the hub vendor ID
 *
 * @returns  -1 if failed, other 0
 */
int usb5807c_set_usb_vid();

/**
 * @brief  Retrieve the USB hub vendor ID
 *
 * @returns  -1 if failed, other the vendor ID
 */
int usb5807c_retrieve_usb_vid();

/**
 * @brief  Check if a given port has activity or not
 *
 * @param port Port will be checked for activity
 *
 * @returns true or false
 */
int usb5807c_is_port_active(uint8_t port);

int usb5807c_is_port_enabled(uint8_t port);

/**
 * @brief  Disable a set of USB ports
 *
 * @param port ports to be disabled using USB5807C_PORT* defines
 *
 * @returns  -1 if failed, other 0
 */
int usb5807c_disable_port(uint8_t port);

#endif /* _GP_DRV_USB5807C_ */
