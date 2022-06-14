/**
 * @file board.c
 * @brief  Zeus300S Board specific code
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 * History
 * June 2021 - davth - added watchdog and disable_irq
 */

#include "board.h"
#include "fsl_wwdt.h"
#if defined(SDK_I2C_BASED_COMPONENT_USED) && SDK_I2C_BASED_COMPONENT_USED
#include "fsl_i2c.h"
#endif /* SDK_I2C_BASED_COMPONENT_USED */

#define BOARD_ACCEL_I2C_BASEADDR   (I2C_Type *)BOARD_I2C_MAINCPU_BASE
#define BOARD_ACCEL_I2C_CLOCK_FREQ 12000000

#define BOARD_CODEC_I2C_BASEADDR   (I2C_Type *)BOARD_I2C_MAINCPU_BASE
#define BOARD_CODEC_I2C_CLOCK_FREQ 12000000

uint8_t BOARD_GetRevision(void) {
    return HWID;
}

const char * BOARD_GetName(void) {
    return BOARD_NAME;
}

uint8_t BOARD_GetNameSize(void) {
    return sizeof(BOARD_NAME);
}

// ------------------------------------------------------------------------------
// WDOG
// ------------------------------------------------------------------------------
void wdog_refresh(void) {
    WWDT_Refresh(WWDT);
}

// ------------------------------------------------------------------------------
// enable user defined irq's
// ------------------------------------------------------------------------------
void enable_user_irq(void) {
    EnableIRQ(FLEXCOMM0_IRQn);
    EnableIRQ(FLEXCOMM1_IRQn);
}

// ------------------------------------------------------------------------------
// disable all user defined irq's
// ------------------------------------------------------------------------------
void disable_user_irq(void) {
    DisableIRQ(FLEXCOMM0_IRQn);
    DisableIRQ(FLEXCOMM1_IRQn);
}

#if defined(SDK_I2C_BASED_COMPONENT_USED) && SDK_I2C_BASED_COMPONENT_USED
void BOARD_I2C_Init(I2C_Type * base, uint32_t clkSrc_Hz) {
    i2c_master_config_t i2cConfig = { 0 };

    I2C_MasterGetDefaultConfig(&i2cConfig);
    I2C_MasterInit(base, &i2cConfig, clkSrc_Hz);
}

status_t BOARD_I2C_Send(I2C_Type * base,
                        uint8_t deviceAddress,
                        uint32_t subAddress,
                        uint8_t subaddressSize,
                        uint8_t * txBuff,
                        uint8_t txBuffSize) {
    i2c_master_transfer_t masterXfer;

    /* Prepare transfer structure. */
    masterXfer.slaveAddress = deviceAddress;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = subAddress;
    masterXfer.subaddressSize = subaddressSize;
    masterXfer.data = txBuff;
    masterXfer.dataSize = txBuffSize;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    return I2C_MasterTransferBlocking(base, &masterXfer);
}

status_t BOARD_I2C_Receive(I2C_Type * base,
                           uint8_t deviceAddress,
                           uint32_t subAddress,
                           uint8_t subaddressSize,
                           uint8_t * rxBuff,
                           uint8_t rxBuffSize) {
    i2c_master_transfer_t masterXfer;

    /* Prepare transfer structure. */
    masterXfer.slaveAddress = deviceAddress;
    masterXfer.subaddress = subAddress;
    masterXfer.subaddressSize = subaddressSize;
    masterXfer.data = rxBuff;
    masterXfer.dataSize = rxBuffSize;
    masterXfer.direction = kI2C_Read;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    return I2C_MasterTransferBlocking(base, &masterXfer);
}

void BOARD_Accel_I2C_Init(void) {
    BOARD_I2C_Init(BOARD_ACCEL_I2C_BASEADDR, BOARD_ACCEL_I2C_CLOCK_FREQ);
}

status_t BOARD_Accel_I2C_Send(uint8_t deviceAddress, uint32_t subAddress, uint8_t subaddressSize, uint32_t txBuff) {
    uint8_t data = (uint8_t)txBuff;

    return BOARD_I2C_Send(BOARD_ACCEL_I2C_BASEADDR, deviceAddress, subAddress, subaddressSize, &data, 1);
}

status_t BOARD_Accel_I2C_Receive(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subaddressSize, uint8_t * rxBuff, uint8_t rxBuffSize) {
    return BOARD_I2C_Receive(BOARD_ACCEL_I2C_BASEADDR, deviceAddress, subAddress, subaddressSize, rxBuff, rxBuffSize);
}

void BOARD_Codec_I2C_Init(void) {
    BOARD_I2C_Init(BOARD_CODEC_I2C_BASEADDR, BOARD_CODEC_I2C_CLOCK_FREQ);
}

status_t BOARD_Codec_I2C_Send(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, const uint8_t * txBuff, uint8_t txBuffSize) {
    return BOARD_I2C_Send(BOARD_CODEC_I2C_BASEADDR, deviceAddress, subAddress, subAddressSize, (uint8_t *)txBuff,
                          txBuffSize);
}

status_t BOARD_Codec_I2C_Receive(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, uint8_t * rxBuff, uint8_t rxBuffSize) {
    return BOARD_I2C_Receive(BOARD_CODEC_I2C_BASEADDR, deviceAddress, subAddress, subAddressSize, rxBuff, rxBuffSize);
}
#endif /* SDK_I2C_BASED_COMPONENT_USED */