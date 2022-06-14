/**
 * @file pin_mux.c
 * @brief  LPCXpresso55S69 Pin Mux configuration
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 * @date 2021-07-12 - Flexcomm0 Flexcomm1 added  - davth
 * @date 2021-12-10 - GPIO_TEST_PIN1&2 clarification - davth
 */
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "fsl_iocon.h"

void BOARD_SetupFPGABootPins(void);
void BOARD_InitFlexComm0(void);
void BOARD_InitFlexComm1(void);
void BOARD_SetupUSB5807CPins(void);
void BOARD_InitI2CPins(void);
void BOARD_InitGoWINPowerPins(void);
void BOARD_InitFlexComm8(void);

/* Bootloader bootpins Init */
void BOARD_BootInitBootPins(void) {
    BOARD_InitDebugUART();
    BOARD_InitI2CPins();
    BOARD_InitGoWINPowerPins();
    BOARD_SetupFPGABootPins();
    BOARD_SetupUSB5807CPins();

    BOARD_InitFlexComm0(); // debug
    BOARD_SetSPIMux(0); // Set SPI Flash access for Gowin
    BOARD_InitFlexComm8(); // spi flash
}

/* Application bootpins Init */
void BOARD_AppInitBootPins(void) {
    BOARD_InitDebugUART();
    BOARD_InitI2CPins();

    BOARD_SetupFPGABootPins();

    BOARD_SetupUSB5807CPins();
    BOARD_InitGoWINPowerPins();

    BOARD_InitFlexComm0();
    BOARD_InitFlexComm1();
    BOARD_InitFlexComm8();
}

/* Gowin pins config */
void BOARD_InitGoWINPowerPins(void) {
    IOCON->PIO[1][17] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][16] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][15] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][13] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t outputconf = { kGPIO_DigitalOutput, 0 };

    gpio_pin_config_t inputconf = { kGPIO_DigitalInput, 0 };

    GPIO_PinInit(GPIO, 1, 17, &outputconf); // Testpin1
    GPIO_PinWrite(GPIO, 1, 17, 0);

    GPIO_PinInit(GPIO, 1, 16, &outputconf); // Testpin2
    GPIO_PinWrite(GPIO, 1, 16, 0);


    /* remark:  davth - 5/5/2022
     * Setting the pin high before configuration reduces glitch to 1us */
    GPIO_PinWrite(GPIO, 1, 15, 1); // active Low (4k7 Pull-up on PCB)

    // Careful below line triggers a reset of the Gowin (& Zynq)
    // measured to be 3us low, gowin reconfigures at 50ns glitch!
    GPIO_PinInit(GPIO, 1, 15, &outputconf); // Gowin RECONFIGURE pin
    GPIO_PinWrite(GPIO, 1, 15, 1); // active Low (4k7 Pull-up on PCB)

    GPIO_PinInit(GPIO, 1, 13, &inputconf); // ready pin
}

/* BOARD_Ready_Gowin
 * Reads the Gowin READY signal
 */
bool BOARD_Ready_Gowin() {
    uint32_t b0 = GPIO_PinRead(GPIO, 1, 13);

    return (b0 != 0);
}

/* GPIO_RECONFIG GOWIN TOGGLE
 *
 * We can Reboot the Gowin FPGA with this pin.
 * -> Trigger a bitsream read after update from the SPI Flash
 */
void BOARD_Reconfigure_Gowin(uint32_t delayTime_us) {
    if (delayTime_us < 200000)
        delayTime_us = 200000;
    GPIO_PinWrite(GPIO, 1, 15, 0);
    SDK_DelayAtLeastUs(delayTime_us, 96000000U);
    GPIO_PinWrite(GPIO, 1, 15, 1);
}

/* GPIO_TEST_PIN2 TOGGLE
 * GPIO_TEST_PIN2 in schematic = TP on Header J5 pin4
 *
 * Warning: This testpin has no PCB trace-connection on the PCB!
 */
void BOARD_PwrOff_MainCPU(uint32_t delayTime_us) { // !!! Not Functional !!!
    if (delayTime_us < 200000)
        delayTime_us = 200000;
    GPIO_PinWrite(GPIO, 1, 16, 1);
    SDK_DelayAtLeastUs(delayTime_us, 96000000U); // 200ms High pulse
    GPIO_PinWrite(GPIO, 1, 16, 0);
}

/* GPIO_TEST_PIN1 TOGGLE
 * GPIO_TEST_PIN1 in schematic = TP on Header J5 pin2
 * This is a toggle pulse for enabling the PowerSequence in the Gowin FPGA
 * This is a soldered wire going to Gowin with pullDown resistor on AlphaBoards
 *
 * @param delayTime_us  Delay time in unit of microsecond. Min is 200ms
 */
void BOARD_PwrOn_MainCPU(uint32_t delayTime_us) {
    if (delayTime_us < 200000)
        delayTime_us = 200000;
    GPIO_PinWrite(GPIO, 1, 17, 1);
    SDK_DelayAtLeastUs(delayTime_us, 96000000U); // 200ms High pulse
    GPIO_PinWrite(GPIO, 1, 17, 0);
}

void BOARD_SetupUSB5807CPins(void) {
    /* USB Reset Pin */
    IOCON->PIO[1][23] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* FlexCMD Pin */
    IOCON->PIO[1][24] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* FlexState Pin */
    IOCON->PIO[1][25] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t outputconf = { kGPIO_DigitalOutput, 0 };

    gpio_pin_config_t inputconf = { kGPIO_DigitalInput, };

    GPIO_PinInit(GPIO, 1, 23, &outputconf);
    GPIO_PinInit(GPIO, 1, 24, &outputconf);
    GPIO_PinInit(GPIO, 1, 25, &inputconf);
}

void BOARD_Set_USB_Reset(uint8_t set) {
    GPIO_PinWrite(GPIO, 1, 23, set);
}


void BOARD_SetupFPGABootPins(void) {
    /* FPGA Boot mode pins */
    IOCON->PIO[1][22] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][21] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][20] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][19] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t outputconf = { kGPIO_DigitalOutput, 0 };

    GPIO_PinInit(GPIO, 1, 22, &outputconf);
    GPIO_PinInit(GPIO, 1, 21, &outputconf);
    GPIO_PinInit(GPIO, 1, 20, &outputconf);
    GPIO_PinInit(GPIO, 1, 19, &outputconf);

    /* Set boot mode to JTAG */
    /* NOTE: This is temporary! needed for initial QSPI write via Xilinx-tool */
    // GPIO_PinWrite(GPIO, 1, 22, 0);
    // GPIO_PinWrite(GPIO, 1, 21, 0);
    // GPIO_PinWrite(GPIO, 1, 20, 0);
    // GPIO_PinWrite(GPIO, 1, 19, 0);

    /* Set boot mode to QSPI32 */
    GPIO_PinWrite(GPIO, 1, 22, 0);
    GPIO_PinWrite(GPIO, 1, 21, 0);
    GPIO_PinWrite(GPIO, 1, 20, 1);
    GPIO_PinWrite(GPIO, 1, 19, 0);
}

/* BOARD_read_fgpa_boot_mode */
uint8_t BOARD_read_fgpa_boot_mode(void) {
    uint32_t b0 = GPIO_PinRead(GPIO, 1, 22);
    uint32_t b1 = GPIO_PinRead(GPIO, 1, 21);
    uint32_t b2 = GPIO_PinRead(GPIO, 1, 20);
    uint32_t b3 = GPIO_PinRead(GPIO, 1, 19);
    uint8_t boot_mode = (uint8_t)((b0 << 3) + (b1 << 2) + (b2 << 1) + b3);

    return boot_mode;
}

/* BOARD_InitDebugUART
 * - {pin_num: '83', peripheral: FLEXCOMM3, signal: RXD_SDA_MOSI_DATA, pin_signal: PIO0_3/FC3_RXD_SDA_MOSI_DATA/CTIMER0_MAT1/SCT0_OUT1/SCT_GPI3/SECURE_GPIO0_3}
 * - {pin_num: '81', peripheral: FLEXCOMM3, signal: TXD_SCL_MISO_WS, pin_signal: PIO0_2/FC3_TXD_SCL_MISO_WS/CT_INP1/SCT0_OUT0/SCT_GPI2/SECURE_GPIO0_2}
 */
void BOARD_InitDebugUART(void) {
    CLOCK_EnableClock(kCLOCK_Iocon);

    IOCON->PIO[0][2] = ((IOCON->PIO[0][2] &
                        /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT02 (pin 81) is configured as FC3_TXD_SCL_MISO_WS. */
                        | IOCON_PIO_FUNC(PIO0_2_FUNC_ALT1)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(PIO0_2_DIGIMODE_DIGITAL));

    IOCON->PIO[0][3] = ((IOCON->PIO[0][3] &
                        /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT03 (pin 83) is configured as FC3_RXD_SDA_MOSI_DATA. */
                        | IOCON_PIO_FUNC(PIO0_3_FUNC_ALT1)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(PIO0_3_DIGIMODE_DIGITAL));
}

/* BOARD_InitFlexComm0
 * {pin_num: '92', peripheral: FLEXCOMM0, signal: RXD_SDA_MOSI_DATA, pin_signal: PIO0_29/FC0_RXD_SDA_MOSI_DATA/SD1_D2/CTIMER2_MAT3/SCT0_OUT8/CMP0_OUT/PLU_OUT2/SECURE_GPIO0_29}
 * {pin_num: '94', peripheral: FLEXCOMM0, signal: TXD_SCL_MISO_WS, pin_signal: PIO0_30/FC0_TXD_SCL_MISO_WS/SD1_D3/CTIMER0_MAT0/SCT0_OUT9/SECURE_GPIO0_30}
 *
 * Towards Xilinx Zynq MainCPU
 */
void BOARD_InitFlexComm0(void) {
    IOCON->PIO[0][29] = ((IOCON->PIO[0][29] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT029 (pin 92) is configured as FC0_RXD_SDA_MOSI_DATA. */
                         | IOCON_PIO_FUNC(PIO0_29_FUNC_ALT1)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_29_DIGIMODE_DIGITAL));

    IOCON->PIO[0][30] = ((IOCON->PIO[0][30] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT030 (pin 94) is configured as FC0_TXD_SCL_MISO_WS. */
                         | IOCON_PIO_FUNC(PIO0_30_FUNC_ALT1)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_30_DIGIMODE_DIGITAL));
}

/* BOARD_InitFlexComm1
 * {pin_num: '71', peripheral: FLEXCOMM1, signal: RXD_SDA_MOSI_DATA, pin_signal: PIO0_13/FC1_CTS_SDA_SSEL0/UTICK_CAP0/CT_INP0/SCT_GPI0/FC1_RXD_SDA_MOSI_DATA/PLU_IN0/SECURE_GPIO0_13}
 * {pin_num: '72', peripheral: FLEXCOMM1, signal: TXD_SCL_MISO_WS, pin_signal: PIO0_14/FC1_RTS_SCL_SSEL1/UTICK_CAP1/CT_INP1/SCT_GPI1/FC1_TXD_SCL_MISO_WS/PLU_IN1/SECURE_GPIO0_14}
 *  Towards Gowin FPGA
 */
void BOARD_InitFlexComm1(void) {
    IOCON->PIO[0][13] = ((IOCON->PIO[0][13] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT013 (pin 71) is configured as FC1_RXD_SDA_MOSI_DATA. */
                         | IOCON_PIO_FUNC(PIO0_13_FUNC_ALT5)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_13_DIGIMODE_DIGITAL));

    IOCON->PIO[0][14] = ((IOCON->PIO[0][14] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT014 (pin 72) is configured as FC1_TXD_SCL_MISO_WS. */
                         | IOCON_PIO_FUNC(PIO0_14_FUNC_ALT6)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_14_DIGIMODE_DIGITAL));
}

/*
 * BOARD_InitI2CPins
 * Flexcomm4 on (HUB1 - CH2) , SCL: PIO0_16 , SDA: PIO1_9
 * Flexcomm5 on (HUB0 - CH4) , SCL: PIO0_9 , SDA: PIO1_14
 * {pin_num: '57', peripheral: FLEXCOMM5, signal: CTS_SDA_SSEL0, pin_signal: PIO1_14/UTICK_CAP2/CTIMER1_MAT2/FC5_CTS_SDA_SSEL0/USB0_LEDN/SD1_CMD/ACMP0_D}
 * {pin_num: '55', peripheral: FLEXCOMM5, signal: TXD_SCL_MISO_WS, pin_signal: PIO0_9/FC3_SSEL2/SD0_POW_EN/FC5_TXD_SCL_MISO_WS/SECURE_GPIO0_9/ACMP0_B}
 * {pin_num: '10', peripheral: FLEXCOMM4, signal: CTS_SDA_SSEL0, pin_signal: PIO1_9/FC1_SCK/CT_INP4/SCT0_OUT2/FC4_CTS_SDA_SSEL0/ADC0_12}
 * {pin_num: '14', peripheral: FLEXCOMM4, signal: TXD_SCL_MISO_WS, pin_signal: PIO0_16/FC4_TXD_SCL_MISO_WS/CLKOUT/CT_INP4/SECURE_GPIO0_16/ADC0_8}
 */
void BOARD_InitI2CPins(void) {
    CLOCK_EnableClock(kCLOCK_Iocon);

    /* FLEXCOMM4 */
    // SCLK
    IOCON->PIO[0][16] = ((IOCON->PIO[0][16] &
                         /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT016 (pin 14) is configured as FC4_TXD_SCL_MISO_WS. */
                         | IOCON_PIO_FUNC(PIO0_16_FUNC_ALT1)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_16_DIGIMODE_DIGITAL));

    // SDA
    if (Chip_GetVersion() == 1) {
        IOCON->PIO[1][9] = ((IOCON->PIO[1][9] &
                         /* Mask bits to zero which are setting */
                             (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT19 (pin 10) is configured as FC4_CTS_SDA_SSEL0. */
                            | IOCON_PIO_FUNC(PIO1_9_FUNC_ALT5)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                            | IOCON_PIO_DIGIMODE(PIO1_9_DIGIMODE_DIGITAL));
    } else {
        IOCON->PIO[1][9] = ((IOCON->PIO[1][9] &
                         /* Mask bits to zero which are setting */
                             (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT19 (pin 10) is configured as FC4_CTS_SDA_SSEL0. */
                            | IOCON_PIO_FUNC(PIO1_9_FUNC_ALT5)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                            | IOCON_PIO_DIGIMODE(PIO1_9_DIGIMODE_DIGITAL));
    }

    /* FLEXCOMM5 */
    // SCLK
    if (Chip_GetVersion() == 1) {
        IOCON->PIO[0][9] = ((IOCON->PIO[0][9] &
                             /* Mask bits to zero which are setting */
                             (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT09 (pin 55) is configured as FC5_TXD_SCL_MISO_WS. */
                            | IOCON_PIO_FUNC(PIO0_9_FUNC_ALT3)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                            | IOCON_PIO_DIGIMODE(PIO0_9_DIGIMODE_DIGITAL));
    } else {
        IOCON->PIO[0][9] = ((IOCON->PIO[0][9] &
                         /* Mask bits to zero which are setting */
                             (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT09 (pin 55) is configured as FC5_TXD_SCL_MISO_WS. */
                            | IOCON_PIO_FUNC(PIO0_9_FUNC_ALT3)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                            | IOCON_PIO_DIGIMODE(PIO0_9_DIGIMODE_DIGITAL));
    }
    // SDA
    IOCON->PIO[1][14] = ((IOCON->PIO[1][14] &
                         /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT114 (pin 57) is configured as FC5_CTS_SDA_SSEL0. */
                         | IOCON_PIO_FUNC(PIO1_14_FUNC_ALT4)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO1_14_DIGIMODE_DIGITAL));
}

/* FlexComm8 ***********************************************************************************************************
 *
 * Function Name : BOARD_InitFlexComm8
 * Description   : Configures pin routing and optionally pin electrical features for SPI
 *
 * - pin_list:
 * - {pin_num: '60', peripheral: FLEXCOMM8, signal: HS_SPI_MOSI, pin_signal: PIO0_26/FC2_RXD_SDA_MOSI_DATA/CLKOUT/CT_INP14/SCT0_OUT5/USB0_IDVALUE/FC0_SCK/HS_SPI_MOSI/SECURE_GPIO0_26}
 * - {pin_num: '61', peripheral: FLEXCOMM8, signal: HS_SPI_SCK, pin_signal: PIO1_2/CTIMER0_MAT3/SCT_GPI6/HS_SPI_SCK/USB1_PORTPWRN/PLU_OUT5}
 * - {pin_num: '62', peripheral: FLEXCOMM8, signal: HS_SPI_MISO, pin_signal: PIO1_3/SCT0_OUT4/HS_SPI_MISO/USB0_PORTPWRN/PLU_OUT6}
 * - {pin_num: '74', peripheral: FLEXCOMM8, signal: HS_SPI_SSEL0, pin_signal: PIO0_20/FC3_CTS_SDA_SSEL0/CTIMER1_MAT1/CT_INP15/SCT_GPI2/FC7_RXD_SDA_MOSI_DATA/HS_SPI_SSEL0/PLU_IN5/SECURE_GPIO0_20/FC4_TXD_SCL_MISO_WS}
 *
 * - {pin_num: '91', peripheral: GPIO, signal: 'PIO1, 31', pin_signal: PIO1_31/MCLK/SD1_CLK/CTIMER0_MAT2/SCT0_OUT6/PLU_IN0}
 *
 * PIO1_31 is the Mux_Select for switching between GP/Gowin Access
 * END ****************************************************************************************************************/
void BOARD_InitFlexComm8(void) {

    /* Enables the clock for the I/O controller.: Enable Clock. */
    CLOCK_EnableClock(kCLOCK_Iocon);

    IOCON->PIO[0][20] = ((IOCON->PIO[0][20] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT020 (pin 74) is configured as HS_SPI_SSEL0. */
                         | IOCON_PIO_FUNC(0x08u)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_20_DIGIMODE_DIGITAL));

    IOCON->PIO[0][26] = ((IOCON->PIO[0][26] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT026 (pin 60) is configured as HS_SPI_MOSI. */
                         | IOCON_PIO_FUNC(0x09u)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_26_DIGIMODE_DIGITAL));

    IOCON->PIO[1][2] = ((IOCON->PIO[1][2] &
                        /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT12 (pin 61) is configured as HS_SPI_SCK. */
                        | IOCON_PIO_FUNC(PIO1_2_FUNC_ALT6)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(PIO1_2_DIGIMODE_DIGITAL));

    IOCON->PIO[1][3] = ((IOCON->PIO[1][3] &
                        /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT13 (pin 62) is configured as HS_SPI_MISO. */
                        | IOCON_PIO_FUNC(PIO1_3_FUNC_ALT6)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(PIO1_3_DIGIMODE_DIGITAL));

    // PIO1_31 is the Mux_Select
    IOCON->PIO[1][31] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t outputconf = { kGPIO_DigitalOutput, 0 };

    GPIO_PinInit(GPIO, 1, 31, &outputconf);
}

/*  GPIO_SPI_MUX output config for Flexcomm8
 *  PIO1_31 is the Mux_Select for switching between GP/Gowin Access
 *  param : 0 = Gowin Access  , 1 = GP Access
 */
void BOARD_SetSPIMux(uint8_t on) {
    GPIO_PinWrite(GPIO, 1, 31, on);
}

/* For future use - Schematic change  16Mhz MClk for Gowin on 96Mhz Clk
 * https://itrack.barco.com/browse/MI0604-271
 * pinmux 1.31 -> 1.29
 */
void BOARD_SetGowinClk() {
    IOCON->PIO[1][31] = ((IOCON->PIO[1][31] &
                      /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                      /* Selects pin function.
                       * : PORT131 (pin 91) is configured as MCLK. */
                         | IOCON_PIO_FUNC(PIO1_31_FUNC_ALT1)

                      /* Select Digital mode.
                       * : Enable Digital mode.
                       * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO1_31_DIGIMODE_DIGITAL));

    SYSCON->MCLKIO = ((SYSCON->MCLKIO &
                      /* Mask bits to zero which are setting */
                       (~(SYSCON_MCLKIO_MCLKIO_MASK)))

                      /* MCLK control.
                       * : output mode. */
                      | SYSCON_MCLKIO_MCLKIO(MCLKIO_MCLKIO_OUTPUT));
}
