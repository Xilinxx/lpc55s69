/**
 * @file peripherals.c
 * @brief  Gaia300D Peripheral setup
 * Peripheral configuration for Gaia300D board.
 * @version v0.1
 */
#include "peripherals.h"
#include "fsl_crc.h"
#include "fsl_usart.h"
#include "board.h"

// FLEXCOMM - default settings
const usart_config_t FLEXCOMM_DEFAULT_USART_CONFIG = {
    .baudRate_Bps = BOARD_DEBUG_USART_BAUDRATE,
    .syncMode = kUSART_SyncModeDisabled,
    .parityMode = kUSART_ParityDisabled,
    .stopBitCount = kUSART_OneStopBit,
    .bitCountPerChar = kUSART_8BitsPerChar,
    .loopback = false,
    .txWatermark = kUSART_TxFifo0,
    .rxWatermark = kUSART_RxFifo1,
    .enableRx = true,
    .enableTx = true,
    .clockPolarity = kUSART_RxSampleOnFallingEdge,
    .enableContinuousSCLK = false};

// FLEXCOMM0/1 - Zynq UART connection settings
const usart_config_t FLEXCOMM0_config = {
    .baudRate_Bps = BOARD_MAINCPU_USART_BAUDRATE,
    .syncMode = kUSART_SyncModeDisabled,
    .parityMode = kUSART_ParityDisabled,
    .stopBitCount = kUSART_OneStopBit,
    .bitCountPerChar = kUSART_8BitsPerChar,
    .loopback = false,
    .txWatermark = kUSART_TxFifo0,
    .rxWatermark = kUSART_RxFifo1,
    .enableRx = true,
    .enableTx = true,
    .clockPolarity = kUSART_RxSampleOnFallingEdge,
    .enableContinuousSCLK = false};

// FLEXCOMM0 - Secondary CPU UART connection settings
static void FLEXCOMM0_init(void)
{
  /* Reset FLEXCOMM device */
  RESET_PeripheralReset(kFC0_RST_SHIFT_RSTn);
  USART_Init(FLEXCOMM0_PERIPHERAL, &FLEXCOMM0_config,
             FLEXCOMM0_CLOCK_SOURCE);
  USART_EnableInterrupts(FLEXCOMM0_PERIPHERAL,
                         kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);
}

// FLEXCOMM1 - Main CPU UART connection settings
static void FLEXCOMM1_init(void)
{
  /* Reset FLEXCOMM device */
  RESET_PeripheralReset(kFC1_RST_SHIFT_RSTn);
  USART_Init(FLEXCOMM1_PERIPHERAL, &FLEXCOMM0_config,
             FLEXCOMM1_CLOCK_SOURCE);
  USART_EnableInterrupts(FLEXCOMM1_PERIPHERAL,
                         kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);
}

// FLEXCOMM5 - Gowin FPGA UART connection settings
const usart_config_t FLEXCOMM5_config = {
    .baudRate_Bps = 57600,
    .syncMode = kUSART_SyncModeDisabled,
    .parityMode = kUSART_ParityDisabled,
    .stopBitCount = kUSART_OneStopBit,
    .bitCountPerChar = kUSART_8BitsPerChar,
    .loopback = false,
    .txWatermark = kUSART_TxFifo0,
    .rxWatermark = kUSART_RxFifo1,
    .enableRx = true,
    .enableTx = true,
    .clockPolarity = kUSART_RxSampleOnFallingEdge,
    .enableContinuousSCLK = false};

static void FLEXCOMM5_init(void)
{
  /* Reset FLEXCOMM device */
  RESET_PeripheralReset(kFC5_RST_SHIFT_RSTn);
  USART_Init(FLEXCOMM5_PERIPHERAL, &FLEXCOMM5_config,
             FLEXCOMM1_CLOCK_SOURCE);
  USART_EnableInterrupts(FLEXCOMM5_PERIPHERAL,
                         kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);
}

/* I2C Bus USB HUB 0 & 1 */
static void FLEXCOMM2_init(void)
{
  i2c_master_config_t FLEXCOMM_config;
  RESET_PeripheralReset(kFC2_RST_SHIFT_RSTn);
  I2C_MasterGetDefaultConfig(&FLEXCOMM_config);
  // Initialization function
  I2C_MasterInit(FLEXCOMM2_PERIPHERAL, &FLEXCOMM_config, FLEXCOMM2_CLOCK_SOURCE);
}

const usart_config_t FLEXCOMM3_config = FLEXCOMM_DEFAULT_USART_CONFIG;
// DEBUG UART
static void FLEXCOMM3_init(void)
{
  /* Reset FLEXCOMM device */
  RESET_PeripheralReset(kFC3_RST_SHIFT_RSTn);
  USART_Init(FLEXCOMM3_PERIPHERAL, &FLEXCOMM3_config, FLEXCOMM3_CLOCK_SOURCE);
}

/* Flexcomm4 NOT in use
static void FLEXCOMM4_init(void)
{
  i2c_master_config_t FLEXCOMM_config;
  RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);
  I2C_MasterGetDefaultConfig(&FLEXCOMM_config);
  // Initialization function
  I2C_MasterInit(FLEXCOMM4_PERIPHERAL, &FLEXCOMM_config, FLEXCOMM4_CLOCK_SOURCE);
}
*/

const usart_config_t FLEXCOMM6_config = FLEXCOMM_DEFAULT_USART_CONFIG;
/* i2c to MainCPU */
static void FLEXCOMM6_init(void)
{
  i2c_master_config_t FLEXCOMM_config;
  RESET_PeripheralReset(kFC6_RST_SHIFT_RSTn);
  I2C_MasterGetDefaultConfig(&FLEXCOMM_config);
  /* Initialization function */
  I2C_MasterInit(FLEXCOMM6_PERIPHERAL, &FLEXCOMM_config, FLEXCOMM6_CLOCK_SOURCE);
}

void CRCEngine_init(CRC_Type *base, uint32_t seed)
{
  crc_config_t config;

  config.polynomial = kCRC_Polynomial_CRC_32;
  config.reverseIn = true;
  config.complementIn = false;
  config.reverseOut = true;
  config.complementOut = true;
  config.seed = seed;
  CRC_Init(base, &config);
}

/* APPLICATION INIT */

void BOARD_AppInitPeripherals(void)
{
  /* Initialize components */
  FLEXCOMM0_init(); //!< Second CPU
  FLEXCOMM1_init(); //!< MainCPU
  // FLEXCOMM2_init(); //!< I2C USB HUB, initialzed in i2c code
  FLEXCOMM3_init(); //!< Debug uart
  FLEXCOMM5_init(); //!< Gowin
  // FLEXCOMM6_init(); //!< I2C Maincpu, initialzed in i2c code
  // FLEXCOMM8_init(); //!< Gowin SPI Flash , initialzed in spi_master_setup()
}

void BOARD_AppDeinitPeripherals(void)
{
  USART_Deinit(FLEXCOMM0_PERIPHERAL);
  USART_Deinit(FLEXCOMM1_PERIPHERAL);
  USART_Deinit(FLEXCOMM3_PERIPHERAL);
  USART_Deinit(FLEXCOMM5_PERIPHERAL);
}

/* BOOTLOADER INIT */

void BOARD_BootInitPeripherals(void)
{
  /* Initialize components */
  FLEXCOMM0_init(); //!< Second CPU
  FLEXCOMM1_init(); //!< MainCPU
  FLEXCOMM3_init(); //!< Debug uart
  FLEXCOMM2_init(); //!< I2C MASTER, USB HUB
  FLEXCOMM6_init(); //!< I2C MASTER, USB HUB

  CRCEngine_init(CRC_ENGINE, 0xFFFFFFFFU);
}

void BOARD_BootDeinitPeripherals(void)
{
  USART_Deinit(FLEXCOMM0_PERIPHERAL);
  USART_Deinit(FLEXCOMM1_PERIPHERAL);
  USART_Deinit(FLEXCOMM3_PERIPHERAL);
  CRC_Deinit(CRC_ENGINE);
}
