/**
 * @file usb7206c.h
 * @brief  Microchip USB7206C Device driver
 * @author davth
 * @version v0.1
 *
 * @date 2022-06-03
 * @doc AN2936 Microchip
 */

#ifndef _GP_DRV_USB7206C_
#define _GP_DRV_USB7206C_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "fsl_i2c.h"

#define USB7206C_MSB                          0xff00 // !< MSB Helper
#define USB7206C_LSB                          0x00ff // !< LSB Helper

#define USB7206C_I2C_ADDR                     0x2D // !< I2C Address for USB7206C hub
#define USB7206C_I2C_SECOND_ADDR              0x2C // !< I2C Address for USB7206C hub
#define USB7206C_I2C_DISABLE_ADDR             0x0
#define USB7206C_MAX_DATALEN                  128 // !< Maximum length for a data chunk

#define VENDOR_ID_MICROCHIP                   0x0424 // !< default ID after Reset

#define USB7206C_RAM_START                    0x00 // !< RAM Address start
#define USB7206C_RAM_ADDR_OFF                 0x06 // !< RAM Read memory offset

#define USB7206C_READ_CMD                     0x01 // !< Read command
#define USB7206C_READ_CONF_REG_LEN            8  // !< Length of read command
#define USB7206C_WRITE_CMD                    0x00 // !< Write command
#define USB7206C_WRITE_CONF_REG_LEN           9  // !< Lenght of write command
#define USB7206C_WRITE_CONF_REG_CMD_BLOCK_LEN 6   // !< Lenght of write command

#define USB7206C_PORT1                        (1 << 1) // !< Offset for USB port 1
#define USB7206C_PORT2                        (1 << 2) // !< Offset for USB port 2
#define USB7206C_PORT3                        (1 << 3) // !< Offset for USB port 3
#define USB7206C_PORT4                        (1 << 4) // !< Offset for USB port 4
#define USB7206C_PORT5                        (1 << 5) // !< Offset for USB port 5
#define USB7206C_PORT6                        (1 << 6) // !< Offset for USB port 6
#define USB7206C_PORT7                        (1 << 7) // !< Offset for USB port 7



// ------------------------------------------------------------------------------
// CONFIGURATION REGISTERS MEMORY MAP
#define USB7206C_BASEADDRESS0                 0xBF800000 // !< Table1 from datasheet

#define USB7206C_DEV_REV                      0x0000 + USB7206C_BASEADDRESS0
#define USB7206C_USB2_SYS_CONFIG              0x0808 + USB7206C_BASEADDRESS0
#define USB7206C_USB3_SYS_CONFIG              0x0828 + USB7206C_BASEADDRESS0
#define USB7206C_VENDOR_ID                    0x3000 + USB7206C_BASEADDRESS0
#define USB7206C_PROD_ID                      0x3002 + USB7206C_BASEADDRESS0
#define USB7206C_DEV_ID                       0x3004 + USB7206C_BASEADDRESS0
#define USB7206C_HUB_CFG                      0x3006 + USB7206C_BASEADDRESS0

#define USB7206C_USB2_LINK_STATE0_3           0x3100 + USB7206C_BASEADDRESS0
#define USB7206C_USB2_LINK_STATE4_7           0x3101 + USB7206C_BASEADDRESS0
#define USB7206C_USB2_VER_BCD                 0x3108 + USB7206C_BASEADDRESS0

// i2c address default value 0x2D
#define USB7206C_SMBUS_PRIMAIRY_ADR           0xC804 + USB7206C_BASEADDRESS0
// i2c address default value 0x2C, disable with val 0x00
#define USB7206C_SMBUS_SECOND_ADR             0xC805 + USB7206C_BASEADDRESS0


#define USB7206C_PORT_DIS_SELF                0x300a // PORT DISABLE – SELF POWERED
#define USB7206C_PORT_DIS_BUS                 0x300b // PORT DISABLE – BUS POWERED
// ------------------------------------------------------------------------------
#define USB7206C_BASEADDRESS1                 0xBFD20000 // !< Table2 from datasheet
#define USB7206C_REG_I2S_FEAT_SEL             0x3412 + USB7206C_BASEADDRESS1
#define USB7206C_FLEX_FEATURE_REG             0x3440 + USB7206C_BASEADDRESS1


// ------------------------------------------------------------------------------
// Special Commands
#define USB7206C_CMD_ATTACH                   0xAA55 // !< Go to runtime without SMBus
#define USB7206C_CMD_ATTACH_SMBUS             0xAA56 // !< Go to runtime with SMBus
#define USB7206C_CMD_CONF_REG_ACCESS          0x9937 // !< Run memory command
#define USB7206C_CMD_OTP_PROG                 0x9933 // !< Permanently program configuration commands to the OTP
#define USB7206C_CMD_OTP_READ                 0x9934 // !< Read the values of the OTP register

#define USB7206C_FLEXPORT0                    0x0 // !< Upstream port 0
#define USB7206C_FLEXPORT1                    0x1 // !< Upstream port 1

#define USB7206C_READ_DATA_LEN_OFFSET         0   // !< Data offset from a read command
#define USB7206C_READ_DATA_OFFSET             1   // !< Data offset from a read command

/*
 * @brief USB7206C context (Runtime data)
 */
struct usb7206c_ctxt_t {
    I2C_Type * i2c_base;                          // !< Peripherial base addr
    i2c_master_handle_t * i2c_handle;             // !< I2c Connection handle
    i2c_master_transfer_t i2c_transfer;           // !< Main I2C shared transfer object

    bool in_runtime;                              // !< Is the hub in runtime mode?
    uint8_t curr_port_status;                     // !< Current port status
};

/**
 * @brief  Initialize the USB hub context
 *
 * @param base Peripherial base addr for the I2C interface used
 * @param handle I2C master handle used
 *
 * @warning This function must be run before any other functions can be used
 */
void usb7206c_init_ctxt(I2C_Type * base, i2c_master_handle_t * handle, uint8_t i2c_address);

/**
 * @brief  Reset the USB hub
 *
 * @returns  -1 if failed otherwise 0
 */
// int usb7206c_reset();

/**
 * @brief  Go to runtime mode without any configuration interfaces
 *
 * @returns  -1 if failed, otherwise 0
 */
int usb7206c_enable_runtime();

/**
 * @brief  Go to runtime mode with SMBus configuration interface
 *
 * @returns  -1 if failed, otherwise 0
 */
int usb7206c_enable_runtime_with_smbus();

/**
 * @brief  Run the configuration register access command
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_run_config_reg_access();

/**
 * @brief  Retrieve the revision of the Hub
 *
 * @returns -1 if failed, other the hub revision
 */
uint32_t usb7206c_retrieve_revision();

/**
 * @brief  Retrieve the hub id
 *
 * @returns  -1 if failed, other the hub id
 */
uint16_t usb7206c_retrieve_id();

/**
 * @brief  Retrieve the Flex Config
 *
 * @returns  -1 if failed, other the port to which the Upstream port is connected
 */
uint32_t usb7206c_retrieve_configuration();

/**
 * @brief  Set the FlexConnect upstream port
 *
 * @param port USB7206C_FLEXPORT0 or USB7206C_FLEXPORT1
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_set_upstream_port(uint8_t port);

/**
 * @brief  Set the hub vendor ID
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_set_usb_vid();

/**
 * @brief  Retrieve the USB hub vendor ID
 *
 * @returns  -1 if failed, other the vendor ID (2byte) = 0x2404 MicroChip
 */
uint16_t usb7206c_retrieve_usb_vid();

/**
 * @brief  Check if a given port has activity or not
 *
 * @param port Port will be checked for activity
 *
 * @returns true or false
 */
int usb7206c_is_port_active(uint8_t port);

int usb7206c_is_port_enabled(uint8_t port);

/**
 * @brief  Disable a set of USB ports
 *
 * @param port ports to be disabled using USB7206C_PORT* defines
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_disable_port(uint8_t port);

int usb7206c_set_flex_feature_reg(uint8_t * value);
uint16_t usb7206c_get_flex_feature_reg();

/**
 * @brief  Set the primary i2c address (only in soc_config mode)
 *         Default after reset is 0x2d
 *
 * @param address
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_set_primary_i2c_address_reg(uint8_t * address);

/**
 * @brief  Get the primary i2c address
 *         Default after reset is 0x2d
 *
 * @param address 1 byte
 *
 * @returns  -1 if failed, otherwise the address
 */
uint8_t usb7206c_get_primary_i2c_address_reg();

/**
 * @brief  Set the secondary i2c address (only in soc_config mode)
 *         Default after reset is 0x2c
 *
 * @param address  (=0 -> disabled)
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_set_secondary_i2c_address_reg(uint8_t * address);

/**
 * @brief  Disable i2s audio feature
 *
 * @param address  (=0 -> disabled)
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_set_i2s_audio_disabled();


/**
 * @brief  Generic way to set a register
 *
 * @param reg  32bit register value
 * @param value uint8 array
 * @param count nr of bytes to tx
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_set_generic_register(uint32_t reg, uint8_t * value, uint8_t count);


/**
 * @brief  Generic way to read value of a register
 *
 * @param reg  32bit register value
 * @param value uint8 array with the fecthed values = return val
 * @param count nr of bytes to tx
 *
 * @returns  -1 if failed, other 0
 */
int usb7206c_get_generic_register(uint32_t reg, uint8_t * value, uint8_t count);

#endif /* _GP_DRV_USB7206C_ */
