/*********************** (C) COPYRIGHT BARCO 2021 ******************************
* File Name           : i2c.c
* Author              : Barco
* created             : 01/10/2021
* Description         : i2c
*   The i2c communication has the functionality of the byte read/write command
*   it replies only 1 byte and processes the received bytes per 2.
* History:
* 01/10/2021 : introduced in gpmcu code     - DAVTH
* 07/02/2022 : eeprom address 0x62 added    - DAVTH
*******************************************************************************/

#define _I2C_SLAVE_C_

#include "i2c_slave.h"
#include "fsl_i2c.h"
#include "run/comm_run.h"
#include "data_map.h"

#ifndef UNIT_TEST
#include <board.h>
#include "peripherals.h"
#endif

#include "logger.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define I2C_SLAVE_CLOCK_FREQUENCY    (12000000)
#define FLEXCOMM_I2C_MAINCPU_SLAVE   ((I2C_Type *)BOARD_I2C_MAINCPU_BASE)
#define FLEXCOMM_I2C_USB_SLAVE       ((I2C_Type *)BOARD_I2C_USB_USART)

#define I2C_MASTER_SLAVE_ADDR_7BIT   (0x60U)
#define I2C_MASTER_SLAVE_ADDR_BIS    (I2C_MASTER_SLAVE_ADDR_7BIT + 1)
#define I2C_MASTER_SLAVE_ADDR_EEPROM (I2C_MASTER_SLAVE_ADDR_7BIT + 2)
#define I2C_FLASH_CMD_IDENTIFIER     (0x50u)
#define I2C_CMD_LENGTH               (2) // Typical/default command length
#define I2C_DATA_LENGTH              (256) // MAX is 256
#define I2C_EMULATED_EEPROM_SIZE     (EEPROM_SIZE) // define in data_map.h

#define I2C_TOGGLE_TX_SIZE_CMD       0xff // Command for selecting page or per byte r/w

#define I2C_TIMEOUT_LOOPS            0x1FFFF // to do: make this a timer independent of clk

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

uint8_t g_slave_buff[I2C_DATA_LENGTH]; // we share the buffer for both ports
i2c_slave_handle_t g_s_handle_maincpu;
i2c_slave_handle_t g_s_handle_usb;
volatile bool g_SlaveCompletionFlag = false;

// Switch between sending whole page in blocks or per byte
volatile bool g_SlaveTxPageSize = false;
volatile bool g_SlaveTxEepromSize = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

void BOARD_I2C_MAINCPU_FLEXCOMM_IRQ(void) {
    I2C_SlaveTransferHandleIRQ(FLEXCOMM_I2C_MAINCPU_SLAVE, &g_s_handle_maincpu);
    SDK_ISR_EXIT_BARRIER;
}

void BOARD_I2C_USB_FLEXCOMM_IRQ(void) {
    I2C_SlaveTransferHandleIRQ(FLEXCOMM_I2C_USB_SLAVE, &g_s_handle_usb);
    SDK_ISR_EXIT_BARRIER;
}

static void i2c_slave_callback(__attribute__((unused)) I2C_Type * base,
                               __attribute__((unused)) volatile i2c_slave_transfer_t * xfer,
                               __attribute__((unused)) void * userData) {
    u8 identifier;
    u8 address = (xfer->receivedAddress) >> 1;
    bool read = (xfer->receivedAddress) & 0x01; // READ = 1 , WRITE = 0
    static u16 expectedBytes = 0;
    // keep verbose 'false' for SPI Flash program speed, 'true' for debug
    bool verbose = false;

    switch (xfer->event) {
        /* Transmit request  - FIRST EVENT without data */
        case kI2C_SlaveTransmitEvent:
            identifier = *((uint8_t *)userData);
            if (verbose)
                LOG_DEBUG("{0x%02X} kI2C_SlaveTransmitEvent requested byte Id(%x)@%02X %s",
                          address, identifier, address, (read) ? "READ" : "WRITE");

            if ( read ) {
                switch (address) {
                    case I2C_MASTER_SLAVE_ADDR_7BIT:
                        g_slave_buff[0] = byteReadbyId(identifier);
                        xfer->txSize = 1;
                        break;
                    case I2C_MASTER_SLAVE_ADDR_BIS: // readback spi flash data
                        if (g_SlaveTxPageSize) {
                            // This means we will need to read 256 bytes !
                            memcpy(g_slave_buff, Main.Debug.SpiFlashPage, I2C_DATA_LENGTH);
                            xfer->txSize = I2C_DATA_LENGTH;
                        } else {
                            g_slave_buff[0] = Main.Debug.SpiFlashPage[identifier];
                            xfer->txSize = 1;
                        }
                        break;
                    case I2C_MASTER_SLAVE_ADDR_EEPROM: // 24c02 eeprom behaviour
                        if (g_SlaveTxEepromSize) {
                            memcpy(g_slave_buff, Main.Debug.eeprom, I2C_EMULATED_EEPROM_SIZE);
                            xfer->txSize = I2C_EMULATED_EEPROM_SIZE;
                        } else {
                            if (identifier >= I2C_EMULATED_EEPROM_SIZE)
                                g_slave_buff[0] = 0x00;
                            else {
                                if (identifier > 0)
                                    Main.Debug.eeprom_address_cnt = identifier;

                                g_slave_buff[0] =
                                    Main.Debug.eeprom[Main.Debug.eeprom_address_cnt++];
                            }
                            if (Main.Debug.eeprom_address_cnt >= I2C_EMULATED_EEPROM_SIZE)
                                Main.Debug.eeprom_address_cnt = 0;

                            xfer->txSize = 1;
                        }
                        break;
                    default:
                        LOG_WARN("kI2C_SlaveTransmitEvent UNKNOWN ADDRESS %02X", address);
                        break;
                }
            }
            xfer->txData = &g_slave_buff[0];
            break;

        /* Setup the slave receive buffer - FIRST EVENT when data byte is received */
        case kI2C_SlaveReceiveEvent:
            if (verbose)
                LOG_DEBUG("{0x%02X} kI2C_SlaveReceiveEvent -- count[%d] expected[%d] %s",
                          address, xfer->transferredCount, (int)xfer->rxSize, (read) ? "READ" :
                          "WRITE");
            /*  Update information for received process */
            xfer->rxData = g_slave_buff;
            if ( !read ) { // WRITE
                if (verbose)
                    LOG_DEBUG("{0x%02X} kI2C_SlaveReceiveEvent -- 1st data byte [0x%02X]",
                              g_slave_buff[0]);
                switch (address) {
                    case I2C_MASTER_SLAVE_ADDR_7BIT:
                        if (expectedBytes > 0) {
                            xfer->rxSize = expectedBytes;
                            g_SlaveCompletionFlag = true; // disable timeout during eeprom flash
                        } else
                            xfer->rxSize = I2C_CMD_LENGTH;
                        break;
                    case I2C_MASTER_SLAVE_ADDR_BIS: // readback spi flash data
                        xfer->rxSize = I2C_CMD_LENGTH;
                        break;
                    case I2C_MASTER_SLAVE_ADDR_EEPROM: // eeprom behaviour
                        Main.Debug.eeprom_address_cnt = g_slave_buff[0];
                        if (g_SlaveTxEepromSize)
                            xfer->rxSize = I2C_EMULATED_EEPROM_SIZE;
                        else
                            xfer->rxSize = I2C_CMD_LENGTH;
                        break;
                    default:
                        LOG_WARN("kI2C_SlaveReceiveEvent UNKNOWN ADDRESS %02X", address);
                }
            }
            break;

        /* Address match event - 2ND EVENT */
        case kI2C_SlaveAddressMatchEvent:
            if (((xfer->receivedAddress) >> 1) != address) // <- for debug
                LOG_WARN("i2c address mismatch");
            g_SlaveCompletionFlag = false;
            if (verbose)
                LOG_DEBUG("{0x%02X} kI2C_SlaveAddressMatchEvent - %s", address, (read) ? "READ" :
                          "WRITE");
            xfer->rxData = NULL;
            xfer->rxSize = 0;
            break;

        /* The master has sent a stop transition on the bus */
        case kI2C_SlaveCompletionEvent:
            g_SlaveCompletionFlag = true;
            if ( !read ) { // WRITE
                switch (address) {
                    case I2C_MASTER_SLAVE_ADDR_EEPROM:
                        if ( xfer->transferredCount == I2C_CMD_LENGTH ) {
                            if ( g_slave_buff[0] == I2C_TOGGLE_TX_SIZE_CMD ) {
                                g_SlaveTxEepromSize = (g_slave_buff[1] == 1);
                                LOG_DEBUG("{0x%02x} Set tx to MaxSize (8byte) %s", address,
                                          ((g_SlaveTxEepromSize) ? "ON" : "OFF"));
                            }
                        }
                        if ( xfer->transferredCount > 8 )
                            LOG_WARN("{0x%02x} xfer->transferredCount %x", address,
                                     xfer->transferredCount);
                        // I2C_EMULATED_EEPROM_SIZE-byte write supported
                        if ( xfer->transferredCount == 8 ) {
                            for (int i = 0; i < I2C_EMULATED_EEPROM_SIZE; i++) {
                                Main.Debug.eeprom[i] = g_slave_buff[i];
                            }
                        }
                        // 1 byte with index write
                        else {
                            Main.Debug.eeprom[g_slave_buff[0]] = g_slave_buff[1];
                        }
                        break;
                    case I2C_MASTER_SLAVE_ADDR_BIS:
                        // we have the page transfer tx-size i2cde  disabled by default for i2cdetect
                        // Page transfer speeds up reading back the SPI flash data per 256byte
                        if ( xfer->transferredCount == I2C_CMD_LENGTH ) {
                            if (g_slave_buff[0] == I2C_TOGGLE_TX_SIZE_CMD ) {
                                g_SlaveTxPageSize = (g_slave_buff[1] == 1);
                                LOG_DEBUG("{0x%02x} Set Page tx to %s", address,
                                          ((g_SlaveTxPageSize) ? "ON" : "OFF"));
                            }
                        }
                        break;
                    case I2C_MASTER_SLAVE_ADDR_7BIT:
                        // Gateway to the SPI Flash if I2C_FLASH_CMD_IDENTIFIER
                        if (expectedBytes > 0) { // Data coming in
                            // This is the data packet following a previous command
                            expectedBytes = byteWrite2SPI(g_slave_buff, expectedBytes);
                        } else if (g_slave_buff[0] == I2C_FLASH_CMD_IDENTIFIER) {
                            // This is the 2 byte command
                            expectedBytes = byteWrite2SPI(g_slave_buff, I2C_CMD_LENGTH);
                        } else if (xfer->transferredCount == I2C_CMD_LENGTH) { // Assume a write when we receive 2 bytes
                            // Gowin UART write - data-byte (See Gowin Commands overview wiki)
                            byteWritebyId(g_slave_buff[0], g_slave_buff[1]);
                        }
                        break;
                    default:
                        LOG_WARN("{0x%02X} uncovered kI2C_SlaveCompletionEvent event", address);
                        break;
                }
            }
            if (verbose) {
                LOG_DEBUG(
                    "{0x%02X} kI2C_SlaveCompletionEvent [%x %x]%x, transfer count[%d] Status[%X] %s",
                    address,
                    g_slave_buff[0],
                    g_slave_buff[1],
                    *((uint8_t *)userData),       // = g_slave_buff[0]
                    xfer->transferredCount,
                    xfer->completionStatus,
                    (read) ? "READ" : "WRITE");
                LOG_DEBUG("--- I2C_SlaveCompletionEvent --- ");
            }
            memset(g_slave_buff, 0, sizeof(g_slave_buff));
            break;

        default:
            LOG_WARN("{0x%02X} uncovered i2c_slave_callback event(%02X)", address, xfer->event);
            g_SlaveCompletionFlag = true;
            break;
    }
}

/*!
 * @brief i2c_slave_setup()
 * @info https://mcuxpresso.nxp.com/api_doc/dev/2065/a00100.html
 */
int i2c_slave_setup() {
    i2c_slave_config_t slaveConfig;
    status_t reVal = kStatus_Fail;

    memset(g_slave_buff, 0, sizeof(g_slave_buff));

    /* Set up i2c slave */
    I2C_SlaveGetDefaultConfig(&slaveConfig);

    /* Change the slave address */
    slaveConfig.address0.address = I2C_MASTER_SLAVE_ADDR_7BIT;
    /* 2nd address meant for i2cdump */
    slaveConfig.address1.address = I2C_MASTER_SLAVE_ADDR_BIS;
    slaveConfig.address1.addressDisable = false;
    /* 3th address meant for eeprom simulation */
    slaveConfig.address2.address = I2C_MASTER_SLAVE_ADDR_EEPROM;
    slaveConfig.address2.addressDisable = false;

    /* Initialize the I2C slave peripheral */
    I2C_SlaveInit(FLEXCOMM_I2C_MAINCPU_SLAVE, &slaveConfig, I2C_SLAVE_CLOCK_FREQUENCY);
    I2C_SlaveInit(FLEXCOMM_I2C_USB_SLAVE, &slaveConfig, I2C_SLAVE_CLOCK_FREQUENCY);

    /* Create the I2C handle for the non-blocking transfer */
    I2C_SlaveTransferCreateHandle(FLEXCOMM_I2C_MAINCPU_SLAVE, &g_s_handle_maincpu,
                                  i2c_slave_callback, g_slave_buff);
    I2C_SlaveTransferCreateHandle(FLEXCOMM_I2C_USB_SLAVE, &g_s_handle_usb, i2c_slave_callback,
                                  g_slave_buff);

    /* Start accepting I2C transfers on the I2C slave peripheral */
    reVal = I2C_SlaveTransferNonBlocking(FLEXCOMM_I2C_MAINCPU_SLAVE, &g_s_handle_maincpu,
                                         kI2C_SlaveAddressMatchEvent | kI2C_SlaveCompletionEvent);
    if (reVal != kStatus_Success) {
        LOG_WARN("i2c_slave_setup flexcomm maincpu failed");
        return -1;
    }

    reVal = I2C_SlaveTransferNonBlocking(FLEXCOMM_I2C_USB_SLAVE, &g_s_handle_usb,
                                         kI2C_SlaveAddressMatchEvent | kI2C_SlaveCompletionEvent);
    if (reVal != kStatus_Success) {
        LOG_WARN("i2c_slave_setup flexcomm usb failed");
        return -1;
    }

    g_SlaveCompletionFlag = true;

    LOG_DEBUG("i2c_slave_setup on address 0x%x", I2C_MASTER_SLAVE_ADDR_7BIT);

    return 0;
}

void i2c_update() {
    static int timeout = I2C_TIMEOUT_LOOPS;

    if (!g_SlaveCompletionFlag) {
        // on 0x61 we need to read 256bytes, depending on g_SlaveTxPageSize
        // on 0x62 we need to read 8 bytes
        // Otherwise we pass here.
        timeout--;
        if (timeout == 0) {
            LOG_WARN("i2c completion timeout");
            timeout = I2C_TIMEOUT_LOOPS;
            i2c_slave_disable();
            i2c_slave_setup();
        }
    } else {
        timeout = I2C_TIMEOUT_LOOPS;
    }
}

/*!
 * @brief i2c_slave_disable
 */
void i2c_slave_disable() {
    I2C_SlaveEnable(FLEXCOMM_I2C_MAINCPU_SLAVE, false);
    I2C_SlaveEnable(FLEXCOMM_I2C_USB_SLAVE, false);
}
