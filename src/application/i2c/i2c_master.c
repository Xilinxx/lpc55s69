/*********************** (C) COPYRIGHT BARCO 2021 ******************************
* File Name           : i2c_master.c
* Author              : Barco
* created             : 01/10/2021
* Description         : i2c_master
* History:
* 01/10/2021 : introduced in gpmcu code (DAVTH)  - preliminary/untested
*******************************************************************************/

#define _I2C_MASTER_C_

#include "i2c_master.h"
#include "fsl_i2c.h"

#include "usb5807c.h"
#include "si5341.h"

#ifndef UNIT_TEST
  #include <board.h>
  #include "peripherals.h"
  #include "pin_mux.h"
#endif

#include "logger.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define I2C_MASTER_CLOCK_FREQUENCY (12000000)
#define FLEXCOMM_I2C_MAINCPU_MASTER ((I2C_Type *)BOARD_I2C_MAINCPU_BASE)
#define FLEXCOMM_I2C_USB_MASTER ((I2C_Type *)BOARD_I2C_USB_USART)

#define I2C_MASTER_SLAVE_ADDR_7BIT (0x60U)
#define I2C_BAUDRATE               (100000) /* 100K is default*/
#define I2C_DATA_LENGTH            (33)     /* MAX is 256 */

/* TODO: insert other definitions and declarations here. */
// HUB0 - Flexcomm
#define I2C_USB5807C_ADDR 0x2D
#define I2C_TPA6166A2_ADDR 0x40
#define I2C_RX0_TDMS181_ADR 0x5B
#define I2C_RX1_TDMS181_ADR 0x5E
// HUB1 - Flexcomm
#define I2C_OPT3001_ADR 0x47
#define I2C_VCNL4200_ADR 0x51
#define I2C_JOG_ADR 0xff

// example code below, to be removed
#define BME280_ADDR 0x76
#define BME280_ADDR_REG 0xD0

const uint8_t g_bme280_address = BME280_ADDR;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

uint8_t g_master_txBuff[I2C_DATA_LENGTH];
uint8_t g_master_rxBuff[I2C_DATA_LENGTH];

i2c_master_handle_t g_m_handle;

volatile bool g_MasterCompletionFlag = false;

volatile bool completionFlag = false;
volatile bool nakFlag        = false;
uint8_t g_accel_addr_found   = 0x00;


/*******************************************************************************
 * @brief  usb initialisation over i2c in Master-mode
 *         device USB5807C and Clock Generator si5341
 *
 * @returns -1 on failure
 ******************************************************************************/
int i2c_master_initialize_usb(void)
{
    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);
    /* Change the default baudrate configuration */
    masterConfig.baudRate_Bps = I2C_BAUDRATE;
    /* Initialize the I2C master peripheral */
    I2C_MasterInit(BOARD_I2C_USB_PERIPHERAL, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

    I2C_MasterTransferCreateHandle(BOARD_I2C_USB_PERIPHERAL, &g_m_handle, NULL, NULL);

 	LOG_INFO("USB5807C Setting ResetN");
    BOARD_Set_USB_Reset(1);

    usb5807c_init_ctxt(BOARD_I2C_USB_PERIPHERAL, &g_m_handle);

	LOG_INFO("Adding USB5807C device");

    SDK_DelayAtLeastUs(200000, 96000000U);
    if (usb5807c_enable_runtime_with_smbus() < 0)
    {
        LOG_ERROR("usb5807c_enable_runtime_with_smbus failure");
        return -1;
    }
    SDK_DelayAtLeastUs(200000, 96000000U);
    if (usb5807c_set_upstream_port(USB5807C_FLEXPORT1) < 0)
    {
        LOG_ERROR("usb5807c_set_upstream_port failure");
        return -1;
    }
    int id = usb5807c_retrieve_id();
    if (id != -1) {
        LOG_INFO("USB ID: %d", id);
        LOG_INFO("USB VID: 0x%x", usb5807c_retrieve_usb_vid());

        int rev = usb5807c_retrieve_revision();
        LOG_INFO("USB Revision: 0x%x", rev);
    }
    else {
        LOG_ERROR("USB ID: %d", id);
        LOG_ERROR("USB VID: 0x%x", usb5807c_retrieve_usb_vid());
        return -1;
    }

    // Clk generator
    si5341_init_ctxt(BOARD_I2C_USB_PERIPHERAL, &g_m_handle);
    si5341_dump_initial_regmap();

    I2C_MasterDeinit(BOARD_I2C_USB_PERIPHERAL);
    return 0;
}

/*******************************************************************************
 * Code
 ******************************************************************************/
static void i2c_master_callback(__attribute__((unused)) I2C_Type *base,
                                __attribute__((unused)) i2c_master_handle_t *handle,
                                __attribute__((unused)) status_t status,
                                __attribute__((unused)) void *userData)
{
    LOG_DEBUG("uncovered i2c_master_callback status(%d)",status);
    /* Signal transfer success when received success status. */
    if (status == kStatus_Success)
    {
        g_MasterCompletionFlag = true;
    }
}

/* example code for later use.. */
static __attribute__((unused)) bool I2C_ReadAccelWhoAmI(void)
{

    uint8_t who_am_i_reg          = BME280_ADDR_REG;
    uint8_t who_am_i_value        = 0x00;
    bool find_device              = false;

    i2c_master_transfer_t FLEXCOMM4_transfer;
    FLEXCOMM4_transfer.slaveAddress   = BME280_ADDR;
    FLEXCOMM4_transfer.direction      = kI2C_Write;
    FLEXCOMM4_transfer.subaddress     = 0;
    FLEXCOMM4_transfer.subaddressSize = 0;
    FLEXCOMM4_transfer.data           = &who_am_i_reg;
    FLEXCOMM4_transfer.dataSize       = 1;
    FLEXCOMM4_transfer.flags          = kI2C_TransferNoStopFlag;

	I2C_MasterTransferNonBlocking(BOARD_I2C_USB_PERIPHERAL, &g_m_handle, &FLEXCOMM4_transfer);

	/*  wait for transfer completed. */
	while ((!nakFlag) && (!completionFlag))
	{
	}

	nakFlag = false;

	if (completionFlag == true)
	{
		completionFlag     = false;
		find_device        = true;
		g_accel_addr_found = FLEXCOMM4_transfer.slaveAddress;
	}

    if (find_device == true)
    {
    	FLEXCOMM4_transfer.direction      = kI2C_Read;
    	FLEXCOMM4_transfer.subaddress     = 0;
    	FLEXCOMM4_transfer.subaddressSize = 0;
    	FLEXCOMM4_transfer.data           = &who_am_i_value;
    	FLEXCOMM4_transfer.dataSize       = 1;
    	FLEXCOMM4_transfer.flags          = kI2C_TransferRepeatedStartFlag;

        I2C_MasterTransferNonBlocking(BOARD_I2C_USB_PERIPHERAL, &g_m_handle, &FLEXCOMM4_transfer);

        /*  wait for transfer completed. */
        while ((!nakFlag) && (!completionFlag))
        {
        }

        nakFlag = false;

        if (completionFlag == true)
        {
                LOG_INFO("Found a device, the WhoAmI value is 0x%x\r\n", who_am_i_value);
                LOG_INFO("The device address is 0x%x. \r\n", FLEXCOMM4_transfer.slaveAddress);

        }
        else
        {
            LOG_INFO("Not a successful i2c communication \r\n");
            return false;
        }
    }
    else
    {
        LOG_INFO("\r\n Do not find an accelerometer device ! \r\n");
        return false;
    }
    return true;
}

/*!
 * @brief i2c_master_init()
 */
void i2c_master_init()
{
    i2c_master_transfer_t masterXfer = {0};
    status_t reVal                   = kStatus_Fail;

    /* attach 12 MHz clock to FLEXCOMM (I2C master) */
    CLOCK_AttachClk(BOARD_I2C_MAINCPU_CLK);
    CLOCK_AttachClk(BOARD_I2C_USB_CLK);

    /* reset FLEXCOMM for I2C */
    RESET_PeripheralReset(BOARD_I2C_MAINCPU_RST_SHIFT);
    RESET_PeripheralReset(BOARD_I2C_USB_RST_SHIFT);

    /* Set up i2c master to send data to slave*/
    /* First data in txBuff is data length of the transmiting data. */
    g_master_txBuff[0] = I2C_DATA_LENGTH - 1U;
    for (uint8_t i = 1U; i < I2C_DATA_LENGTH; i++)
    {
        g_master_txBuff[i] = (uint8_t)(i-1);
    }

    i2c_master_config_t masterConfig;

    /*
     * masterConfig.debugEnable = false;
     * masterConfig.ignoreAck = false;
     * masterConfig.pinConfig = kI2C_2PinOpenDrain;
     * masterConfig.baudRate_Bps = 100000U;
     * masterConfig.busIdleTimeout_ns = 0;
     * masterConfig.pinLowTimeout_ns = 0;
     * masterConfig.sdaGlitchFilterWidth_ns = 0;
     * masterConfig.sclGlitchFilterWidth_ns = 0;
     */
    I2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Bps = I2C_BAUDRATE;

    /* Initialize the I2C master peripheral */
    I2C_MasterInit(FLEXCOMM_I2C_MAINCPU_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);
    I2C_MasterInit(FLEXCOMM_I2C_USB_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

    /* Create the I2C handle for the non-blocking transfer */
    I2C_MasterTransferCreateHandle(FLEXCOMM_I2C_MAINCPU_MASTER, &g_m_handle, i2c_master_callback, NULL);
    I2C_MasterTransferCreateHandle(FLEXCOMM_I2C_USB_MASTER, &g_m_handle, i2c_master_callback, NULL);

    return;
    // example code below

    /* subAddress = 0x01, data = g_master_txBuff - write to slave.
      start + slaveaddress(w) + subAddress + length of data buffer + data buffer + stop*/
    uint8_t deviceAddress     = 0x01U;
    masterXfer.slaveAddress   = I2C_MASTER_SLAVE_ADDR_7BIT;
    masterXfer.direction      = kI2C_Write;
    masterXfer.subaddress     = (uint32_t)deviceAddress;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = g_master_txBuff;
    masterXfer.dataSize       = I2C_DATA_LENGTH;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    /* Send master non-blocking data to slave */
    reVal = I2C_MasterTransferNonBlocking(FLEXCOMM_I2C_MAINCPU_MASTER, &g_m_handle, &masterXfer);

    /*  Reset master completion flag to false. */
    g_MasterCompletionFlag = false;

    if (reVal != kStatus_Success)
    {
        return;
    }

    /*  Wait for transfer completed. */
    while (!g_MasterCompletionFlag)
    {
    }
    g_MasterCompletionFlag = false;

    LOG_INFO("Receive sent data from slave :");

    /* subAddress = 0x01, data = g_master_rxBuff - read from slave.
      start + slaveaddress(w) + subAddress + repeated start + slaveaddress(r) + rx data buffer + stop */
    masterXfer.slaveAddress   = I2C_MASTER_SLAVE_ADDR_7BIT;
    masterXfer.direction      = kI2C_Read;
    masterXfer.subaddress     = (uint32_t)deviceAddress;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = g_master_rxBuff;
    masterXfer.dataSize       = I2C_DATA_LENGTH - 1;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    reVal = I2C_MasterTransferNonBlocking(FLEXCOMM_I2C_MAINCPU_MASTER, &g_m_handle, &masterXfer);

    /*  Reset master completion flag to false. */
    g_MasterCompletionFlag = false;

    if (reVal != kStatus_Success)
    {
        return;
    }

    /*  Wait for transfer completed. */
    while (!g_MasterCompletionFlag)
    {
    }
    g_MasterCompletionFlag = false;



}
