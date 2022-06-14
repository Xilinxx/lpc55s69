/**
 * @file peripherals.c
 * @brief  LPCXpresso55S69 Peripheral setup
 *
 * Peripheral configuration for LPCXpresso55S69 board.
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */
#include "peripherals.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_crc.h"
#include "fsl_usart.h"

const usart_config_t FLEXCOMM0_config = {
    .baudRate_Bps         = 115200,
    .syncMode             = kUSART_SyncModeDisabled,
    .parityMode           = kUSART_ParityDisabled,
    .stopBitCount         = kUSART_OneStopBit,
    .bitCountPerChar      = kUSART_8BitsPerChar,
    .loopback             = false,
    .txWatermark          = kUSART_TxFifo0,
    .rxWatermark          = kUSART_RxFifo1,
    .enableRx             = true,
    .enableTx             = true,
    .clockPolarity        = kUSART_RxSampleOnFallingEdge,
    .enableContinuousSCLK = false
};

void FLEXCOMM0_init(void) {
    RESET_PeripheralReset(kFC0_RST_SHIFT_RSTn);
    USART_Init(FLEXCOMM0_PERIPHERAL, &FLEXCOMM0_config,
               FLEXCOMM0_CLOCK_SOURCE);
}

const i2c_master_config_t FLEXCOMM1_config = {
    .enableMaster  = true,
    .baudRate_Bps  = 100000,
    .enableTimeout = false
};

void FLEXCOMM1_init(void) {
    RESET_PeripheralReset(kFC1_RST_SHIFT_RSTn);
    I2C_MasterInit(FLEXCOMM1_PERIPHERAL, &FLEXCOMM1_config,
                   FLEXCOMM1_CLOCK_SOURCE);
}

const usart_config_t FLEXCOMM2_config = {
    .baudRate_Bps         = 230400,
    .syncMode             = kUSART_SyncModeDisabled,
    .parityMode           = kUSART_ParityDisabled,
    .stopBitCount         = kUSART_OneStopBit,
    .bitCountPerChar      = kUSART_8BitsPerChar,
    .loopback             = false,
    .txWatermark          = kUSART_TxFifo0,
    .rxWatermark          = kUSART_RxFifo1,
    .enableRx             = true,
    .enableTx             = true,
    .clockPolarity        = kUSART_RxSampleOnFallingEdge,
    .enableContinuousSCLK = false
};

void FLEXCOMM2_init(void) {
    RESET_PeripheralReset(kFC2_RST_SHIFT_RSTn);
    USART_Init(FLEXCOMM2_PERIPHERAL, &FLEXCOMM2_config,
               FLEXCOMM2_CLOCK_SOURCE);
}

void CRCEngine_init(CRC_Type * base, uint32_t seed) {
    crc_config_t config;

    config.polynomial = kCRC_Polynomial_CRC_32;
    config.reverseIn = true;
    config.complementIn = false;
    config.reverseOut = true;
    config.complementOut = true;
    config.seed = seed;
    CRC_Init(base, &config);
}

void BOARD_AppInitPeripherals(void) {
    FLEXCOMM0_init();
    FLEXCOMM2_init();

    CRCEngine_init(CRC_ENGINE, 0xFFFFFFFFU);
}

void BOARD_AppDeinitPeripherals(void) {
    USART_Deinit(FLEXCOMM0_PERIPHERAL);
    USART_Deinit(FLEXCOMM2_PERIPHERAL);
    CRC_Deinit(CRC_ENGINE);
}

void BOARD_BootInitPeripherals(void) {
    FLEXCOMM0_init();
    FLEXCOMM2_init();

    CRCEngine_init(CRC_ENGINE, 0xFFFFFFFFU);
}

void BOARD_BootDeinitPeripherals(void) {
    USART_Deinit(FLEXCOMM0_PERIPHERAL);
    USART_Deinit(FLEXCOMM2_PERIPHERAL);
    CRC_Deinit(CRC_ENGINE);
}
