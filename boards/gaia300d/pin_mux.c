/**
 * @file pin_mux.c
 * @brief  Gaia300D Pin Mux configuration
 * @version v0.1
 * @date 2022-03-09
 */
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "fsl_iocon.h"

void BOARD_SetupFPGABootPins(void);
void BOARD_InitFlexComm0(void);
void BOARD_InitFlexComm1(void);
void BOARD_InitFlexComm5(void);
void BOARD_SetupUsbPins(void);
void BOARD_InitI2CPins(void);
void BOARD_InitGoWINPowerPins(void);
void BOARD_InitFlexComm8(void);
void BOARD_InitDebugPins(void);
void BOARD_InitSensePins(void);

/* Bootloader bootpins Init */
void BOARD_BootInitBootPins(void) {
    BOARD_InitDebugUART();
    BOARD_InitI2CPins();
    BOARD_InitGoWINPowerPins();
    BOARD_SetupFPGABootPins();
    BOARD_SetupUsbPins();

    BOARD_InitFlexComm1();  // Main Zynq
    BOARD_SetSPIMux(0);     // SPI Flash access is Gowin
    BOARD_InitFlexComm8();  // SPI
    BOARD_InitDebugPins();
}

/* Application bootpins Init */
void BOARD_AppInitBootPins(void) {
    BOARD_InitDebugUART();  // FlexComm3
    BOARD_InitI2CPins();    // FlexComm2 & FlexComm6

    BOARD_SetupFPGABootPins();

    BOARD_SetupUsbPins();
    BOARD_InitGoWINPowerPins();

    BOARD_InitFlexComm1();  // Main Zynq
    BOARD_InitFlexComm0();  // 2nd Zynq
    BOARD_InitFlexComm5();  // Gowin
    BOARD_InitFlexComm8();  // SPI

    BOARD_SetSPIMux(0);     // SPI Flash access is Gowin

    BOARD_InitDebugPins();
    BOARD_InitSensePins(); // 24v/48v sense inputs
}

/* Gowin pins config */
void BOARD_InitGoWINPowerPins(void) {
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

    gpio_pin_config_t outputconf = {
        kGPIO_DigitalOutput, 0
    };

    gpio_pin_config_t inputconf = {
        kGPIO_DigitalInput, 0
    };

    GPIO_PinInit(GPIO, 1, 16, &outputconf); // GpMcu_FpgaPowerOn
    GPIO_PinWrite(GPIO, 1, 16, 0);

    GPIO_PinInit(GPIO, 1, 15, &outputconf); // Gowin RECONFIGURE pin
    /* remark:  davth - 5/5/2022
     * Setting the pin high before configuration reduces glitch to 1us */
    GPIO_PinWrite(GPIO, 1, 15, 1);          // active Low (Pull up on pcb)

    GPIO_PinInit(GPIO, 1, 13, &inputconf);  // Gowin-ready pin
}

/* BOARD_Ready_Gowin
 * Read the Gowin READY signal
 */
bool BOARD_Ready_Gowin() {
    uint32_t b0 = GPIO_PinRead(GPIO, 1, 13);

    return b0 != 0;
}

/* BOARD_InitDebugPins
 * Configured as input for now
 */
void BOARD_InitDebugPins(void) {
    IOCON->PIO[0][17] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[0][18] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    // gpio_pin_config_t outputconf = { kGPIO_DigitalOutput, 0 };

    gpio_pin_config_t inputconf = { kGPIO_DigitalInput, 0 };

    GPIO_PinInit(GPIO, 0, 17, &inputconf); // Debug[0]
    GPIO_PinWrite(GPIO, 0, 17, 1);

    GPIO_PinInit(GPIO, 1, 18, &inputconf); // Debug[1]
    GPIO_PinWrite(GPIO, 1, 18, 1);
}

/* BOARD_InitSensePins
 * Configured as input
 */
void BOARD_InitSensePins(void) {
    IOCON->PIO[1][0] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][8] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][9] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t inputconf = { kGPIO_DigitalInput, 0 };

    GPIO_PinInit(GPIO, 1, 0, &inputconf);   // PowerDetectBuf: (Buffered): Power Detect from power input +48V main power supply

    GPIO_PinInit(GPIO, 1, 8, &inputconf);   // P48VSense: Connected to ADC0_4

    GPIO_PinInit(GPIO, 1, 9, &inputconf);   // P24VSense: Connected to ADC0_12
}

/* BOARD_Ready_P48VSense
 */
bool BOARD_Ready_P48VSense() {
    uint32_t b0 = GPIO_PinRead(GPIO, 1, 8);

    return b0 != 0;
}

/* BOARD_Ready_P48VSense
 */
bool BOARD_Ready_P24VSense() {
    uint32_t b0 = GPIO_PinRead(GPIO, 1, 9);

    return b0 != 0;
}

/* GPIO_RECONFIG GOWIN TOGGLE
 *
 * We can Reboot the Gowin FPGA with this pin.
 * -> Trigger the bitsream read after update SPI Flash
 */
void BOARD_Reconfigure_Gowin(uint32_t delayTime_us) {
    if (delayTime_us < 200000) {
        delayTime_us = 200000;
    }

    GPIO_PinWrite(GPIO, 1, 15, 0);
    SDK_DelayAtLeastUs(delayTime_us, 96000000U);
    GPIO_PinWrite(GPIO, 1, 15, 1);
}

/* FPGA_POWER_ON TOGGLE */
void BOARD_PwrOff_MainCPU(uint32_t delayTime_us __attribute__((unused))) {
    /*
     * if (delayTime_us < 200000) {
     *  delayTime_us = 200000;
     * }
     *
     * GPIO_PinWrite(GPIO, 1, 16, 1);
     * SDK_DelayAtLeastUs(delayTime_us, 96000000U); // 200ms High pulse
     * GPIO_PinWrite(GPIO, 1, 16, 0);
     */
}


/* GPIO_TEST_PIN1 TOGGLE
 *
 * @param delayTime_us  Delay time in unit of microsecond. Min is 200ms
 */
void BOARD_PwrOn_MainCPU(uint32_t delayTime_us) {
    if (delayTime_us < 200000) {
        delayTime_us = 200000;
    }

    GPIO_PinWrite(GPIO, 1, 16, 1);
    SDK_DelayAtLeastUs(delayTime_us, 96000000U); // 200ms High pulse
    GPIO_PinWrite(GPIO, 1, 16, 0);
}

void BOARD_SetupUsbPins(void) {
    /* UsbHub0_nVbusUp Pin */
    IOCON->PIO[1][28] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* USB Hub0 Reset Pin */
    IOCON->PIO[0][29] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* USB Hub1 Reset Pin */
    IOCON->PIO[0][30] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* UlpiM_Rst -  Connected to reset of TUSB1210 USB transceiver for Main FPGA */
    IOCON->PIO[1][14] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* UIpiS_Rst: Connected to reset of TUSB1210 USB transceiver for Secondary FPGA. */
    IOCON->PIO[1][1] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* UsbUp0_nVbus: Controls USB VBus of USB UP 0. */
    IOCON->PIO[0][22] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI |
        IOCON_PIO_MODE_PULLUP;

    /* UsbUp1_nVbus: Controls USB VBus of USB UP 1. */
    IOCON->PIO[0][27] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI |
        IOCON_PIO_MODE_PULLUP;

    /* UsbHub0_nVBusUp: Controls USB VBus of USB Hub 0. */
    // GPMCU has to set the its UsbHub0_nVBusUp high to disable Usb Vbus of USB HUB 0.
    // Note the USB HUB 1 is always active.
    IOCON->PIO[0][28] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t outputconf = { kGPIO_DigitalOutput, 0 };

    gpio_pin_config_t inputconf = { kGPIO_DigitalInput, 0 };

    GPIO_PinInit(GPIO, 0, 29, &outputconf); // Hub0 Reset
    GPIO_PinInit(GPIO, 0, 30, &outputconf); // Hub1 Reset
    GPIO_PinInit(GPIO, 1, 14, &outputconf); // UlpiM_Rst: Connected to reset of TUSB1210 USB transceiver for Main FPGA.
    GPIO_PinInit(GPIO, 1, 1, &outputconf);  // UIpiS_Rst: Connected to  reset of TUSB1210 USB transceiver for Secondary FPGA.
    GPIO_PinInit(GPIO, 0, 22, &inputconf);  // UsbUp0_nVbus: Controls USB VBus of USB UP 0.
    GPIO_PinInit(GPIO, 0, 27, &inputconf);  // UsbUp1_nVbus: Controls USB VBus of USB UP 1.
    GPIO_PinInit(GPIO, 0, 28, &outputconf); // UsbHub0_nVBusUp: Controls USB VBus of USB Hub 0

    // Set the output pins to 1/0..
    GPIO_PinWrite(GPIO, 0, 29, 1);  // Reset=1 , keep in Reset for they are both on same bus
    GPIO_PinWrite(GPIO, 0, 30, 1);  // Reset=1
    GPIO_PinWrite(GPIO, 1, 14, 0);  // Reset=1
    GPIO_PinWrite(GPIO, 1, 1,  0);  // Reset=1
    // GPMCU has to set the UsbHub0_nVBusUp high to disable Usb Vbus of USB HUB 0. Note the USB HUB 1 is always active.
    GPIO_PinWrite(GPIO, 0, 28, 0);  // UsbHub0_nVBusUp controlled via FET
}

/* BOARD_read_UsbUp0_nVbus  - '0' if cable is connected
 */
bool BOARD_read_UsbUp0_nVbus() {
    uint32_t b0 = GPIO_PinRead(GPIO, 0, 22);

    return b0 != 0;
}

/* BOARD_read_UsbUp1_nVbus - '0' if cable is connected
 */
bool BOARD_read_UsbUp1_nVbus() {
    uint32_t b0 = GPIO_PinRead(GPIO, 0, 27);

    return b0 != 0;
}

/* UsbHub0_Rst  -> Reset = '1'
 * For naming consistency with Zeus we call this USB_Reset and not USB0_Reset
 */
void BOARD_Set_USB_Reset(uint8_t set) {
    GPIO_PinWrite(GPIO, 0, 29, set);
}

/* UsbHub1_Rst  -> Reset = '1'
 */
void BOARD_Set_USB1_Reset(uint8_t set) {
    GPIO_PinWrite(GPIO, 0, 30, set);
}

/* UsbHub0_nVBusUp  -> default is 0
 * This signal should toggle when a change is detected on the cable
 */
void BOARD_toggle_USB0_VbusUp(uint32_t delayTime_us) {
    if (delayTime_us < 200000) {
        delayTime_us = 200000;
    }

    GPIO_PinWrite(GPIO, 0, 28, 1);
    SDK_DelayAtLeastUs(delayTime_us, 96000000U);
    GPIO_PinWrite(GPIO, 0, 28, 0);
}

void BOARD_SetupFPGABootPins(void) {
    // OutputEnable: Controls the Output Enable of the buffer used for Fpga_Ps signals
    // (to go from Green power to non green power domain).
    IOCON->PIO[1][20] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* FPGA Boot mode pins */
    IOCON->PIO[1][21] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][22] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][23] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    IOCON->PIO[1][24] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t outputconf = {
        kGPIO_DigitalOutput, 0
    };

    GPIO_PinInit(GPIO, 1, 21, &outputconf);
    GPIO_PinInit(GPIO, 1, 22, &outputconf);
    GPIO_PinInit(GPIO, 1, 23, &outputconf);
    GPIO_PinInit(GPIO, 1, 24, &outputconf);

    /* Set boot mode to JTAG */
    /* NOTE: This is temporary! needed for initial QSPI write via Xilinx-tool */
    // GPIO_PinWrite(GPIO, 1, 21, 0);
    // GPIO_PinWrite(GPIO, 1, 22, 0);
    // GPIO_PinWrite(GPIO, 1, 23, 0);
    // GPIO_PinWrite(GPIO, 1, 24, 0);

    /* Set boot mode to Dual Quad SPI, QSPI24 */

    /* Set eMMC18 boot 0110 */
    /* NOTE: This is the default setting */
    GPIO_PinWrite(GPIO, 1, 21, 0);  // Fpga_PsModeTi[3]
    GPIO_PinWrite(GPIO, 1, 22, 1);  // Fpga_PsModeTi[2]
    GPIO_PinWrite(GPIO, 1, 23, 1);  // Fpga_PsModeTi[1]
    GPIO_PinWrite(GPIO, 1, 24, 0);  // Fpga_PsModeTi[0]

    /* Set USB boot    0111 */

    GPIO_PinInit(GPIO, 1, 20, &outputconf);
    GPIO_PinWrite(GPIO, 1, 20, 0); // OutputEnable Level translator, default=0
}

/* BOARD_read_fgpa_boot_mode */
uint8_t BOARD_read_fgpa_boot_mode(void) {
    uint32_t b0 = GPIO_PinRead(GPIO, 1, 24);
    uint32_t b1 = GPIO_PinRead(GPIO, 1, 23);
    uint32_t b2 = GPIO_PinRead(GPIO, 1, 22);
    uint32_t b3 = GPIO_PinRead(GPIO, 1, 21);
    uint8_t boot_mode = (uint8_t)((b3 << 3) + (b2 << 2) + (b1 << 1) + b0);

    return boot_mode;
}

/* BOARD_InitDebugUART - Flexcomm3
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

/* BOARD_InitFlexComm0 :
 *
 * {pin_num: '70', peripheral: FLEXCOMM0, signal: RXD_SDA_MOSI_DATA, pin_signal: PIO0_24/FC0_RXD_SDA_MOSI_DATA/SD0_D0/CT_INP8/SCT_GPI0/SECURE_GPIO0_24}
 * {pin_num: '79', peripheral: FLEXCOMM0, signal: TXD_SCL_MISO_WS, pin_signal: PIO0_25/FC0_TXD_SCL_MISO_WS/SD0_D1/CT_INP9/SCT_GPI1/SECURE_GPIO0_25}
 *
 * Towards 2nd Zynq MainCPU
 */
void BOARD_InitFlexComm0(void) {
    IOCON->PIO[0][24] = ((IOCON->PIO[0][24] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT024 (pin 70) is configured as FC0_RXD_SDA_MOSI_DATA. */
                         | IOCON_PIO_FUNC(PIO0_24_FUNC_ALT1)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_24_DIGIMODE_DIGITAL));

    IOCON->PIO[0][25] = ((IOCON->PIO[0][25] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT025 (pin 79) is configured as FC0_TXD_SCL_MISO_WS. */
                         | IOCON_PIO_FUNC(PIO0_25_FUNC_ALT1)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO0_25_DIGIMODE_DIGITAL));
}

/* BOARD_InitFlexComm1:
 *
 * {pin_num: '93', peripheral: FLEXCOMM1, signal: TXD_SCL_MISO_WS, pin_signal: PIO1_11/FC1_TXD_SCL_MISO_WS/CT_INP5/USB0_VBUS}
 * {pin_num: '40', peripheral: FLEXCOMM1, signal: RXD_SDA_MOSI_DATA, pin_signal: PIO1_10/FC1_RXD_SDA_MOSI_DATA/CTIMER1_MAT0/SCT0_OUT3}
 *
 *  Towards Primary FPGA
 */
void BOARD_InitFlexComm1(void) {
    IOCON->PIO[1][10] = ((IOCON->PIO[1][10] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT110 (pin 40) is configured as FC1_RXD_SDA_MOSI_DATA. */
                         | IOCON_PIO_FUNC(PIO1_10_FUNC_ALT2)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO1_10_DIGIMODE_DIGITAL));

    IOCON->PIO[1][11] = ((IOCON->PIO[1][11] &
                        /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT111 (pin 93) is configured as FC1_TXD_SCL_MISO_WS. */
                         | IOCON_PIO_FUNC(PIO1_11_FUNC_ALT2)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO1_11_DIGIMODE_DIGITAL));
}

/* BOARD_InitFlexComm5:
 *
 * {pin_num: '26', peripheral: FLEXCOMM5, signal: RXD_SDA_MOSI_DATA, pin_signal: PIO0_8/FC3_SSEL3/SD0_CMD/FC5_RXD_SDA_MOSI_DATA/SWO/SECURE_GPIO0_8}
 * {pin_num: '55', peripheral: FLEXCOMM5, signal: TXD_SCL_MISO_WS, pin_signal: PIO0_9/FC3_SSEL2/SD0_POW_EN/FC5_TXD_SCL_MISO_WS/SECURE_GPIO0_9/ACMP0_B}
 *
 * Towards Gowin FPGA
 */
void BOARD_InitFlexComm5(void) {
    /* Enables the clock for the I/O controller.: Enable Clock. */
    CLOCK_EnableClock(kCLOCK_Iocon);

    IOCON->PIO[0][8] = ((IOCON->PIO[0][8] &
                        /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT08 (pin 26) is configured as FC5_RXD_SDA_MOSI_DATA. */
                        | IOCON_PIO_FUNC(PIO0_8_FUNC_ALT3)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(PIO0_8_DIGIMODE_DIGITAL));

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
}

/*
 * BOARD_InitI2CPins
 * Flexcomm2 on (USB HUB 0 & 1) , SCL: PIO1_25 , SDA: PIO1_26
 * Flexcomm6 on (Main CPU) , SCL: PIO1_17 , SDA: PIO0_15
 *
 * {pin_num: '68', peripheral: FLEXCOMM2, signal: CTS_SDA_SSEL0, pin_signal: PIO1_26/FC2_CTS_SDA_SSEL0/SCT0_OUT3/CT_INP3/UTICK_CAP1/HS_SPI_SSEL3/PLU_IN5}
 * {pin_num: '77', peripheral: FLEXCOMM2, signal: TXD_SCL_MISO_WS, pin_signal: PIO1_25/FC2_TXD_SCL_MISO_WS/SCT0_OUT2/SD1_D0/UTICK_CAP0/PLU_CLKIN}
 * {pin_num: '22', peripheral: FLEXCOMM6, signal: CTS_SDA_SSEL0, pin_signal: PIO0_15/FC6_CTS_SDA_SSEL0/UTICK_CAP2/CT_INP16/SCT0_OUT2/SD0_WR_PRT/SECURE_GPIO0_15/ADC0_2}
 * {pin_num: '43', peripheral: FLEXCOMM6, signal: RTS_SCL_SSEL1, pin_signal: PIO1_17/FC6_RTS_SCL_SSEL1/SCT0_OUT4/SD1_CARD_INT_N/SD1_CARD_DET_N}
 */
void BOARD_InitI2CPins(void) {
    CLOCK_EnableClock(kCLOCK_Iocon);

    // Auto Generated code from MCU Expresso uses this Chip_GetVersion()
    if (Chip_GetVersion() == 1) {
        IOCON->PIO[0][15] = ((IOCON->PIO[0][15] &
                         /* Mask bits to zero which are setting */
                              (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT015 (pin 22) is configured as FC6_CTS_SDA_SSEL0. */
                             | IOCON_PIO_FUNC(PIO0_15_FUNC_ALT1)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                             | IOCON_PIO_DIGIMODE(PIO0_15_DIGIMODE_DIGITAL));
    } else {
        IOCON->PIO[0][15] = ((IOCON->PIO[0][15] &
                         /* Mask bits to zero which are setting */
                              (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT015 (pin 22) is configured as FC6_CTS_SDA_SSEL0. */
                             | IOCON_PIO_FUNC(PIO0_15_FUNC_ALT1)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                             | IOCON_PIO_DIGIMODE(PIO0_15_DIGIMODE_DIGITAL));
    }

    IOCON->PIO[1][17] = ((IOCON->PIO[1][17] &
                         /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT117 (pin 43) is configured as FC6_RTS_SCL_SSEL1. */
                         | IOCON_PIO_FUNC(PIO1_17_FUNC_ALT3)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO1_17_DIGIMODE_DIGITAL));

    IOCON->PIO[1][25] = ((IOCON->PIO[1][25] &
                         /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT125 (pin 77) is configured as FC2_TXD_SCL_MISO_WS. */
                         | IOCON_PIO_FUNC(PIO1_25_FUNC_ALT1)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO1_25_DIGIMODE_DIGITAL));

    IOCON->PIO[1][26] = ((IOCON->PIO[1][26] &
                         /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT126 (pin 68) is configured as FC2_CTS_SDA_SSEL0. */
                         | IOCON_PIO_FUNC(PIO1_26_FUNC_ALT1)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(PIO1_26_DIGIMODE_DIGITAL));
}

/* FlexComm8 ***********************************************************************************************************
 *
 * Function Name : BOARD_InitFlexComm8
 * Description   : Configures pin routing and optionally pin electrical features for SPI
 *
 * pin_list:
 * {pin_num: '60', peripheral: FLEXCOMM8, signal: HS_SPI_MOSI, pin_signal: PIO0_26/FC2_RXD_SDA_MOSI_DATA/CLKOUT/CT_INP14/SCT0_OUT5/USB0_IDVALUE/FC0_SCK/HS_SPI_MOSI/SECURE_GPIO0_26}
 * {pin_num: '61', peripheral: FLEXCOMM8, signal: HS_SPI_SCK, pin_signal: PIO1_2/CTIMER0_MAT3/SCT_GPI6/HS_SPI_SCK/USB1_PORTPWRN/PLU_OUT5}
 * {pin_num: '62', peripheral: FLEXCOMM8, signal: HS_SPI_MISO, pin_signal: PIO1_3/SCT0_OUT4/HS_SPI_MISO/USB0_PORTPWRN/PLU_OUT6}
 * {pin_num: '74', peripheral: FLEXCOMM8, signal: HS_SPI_SSEL0, pin_signal: PIO0_20/FC3_CTS_SDA_SSEL0/CTIMER1_MAT1/CT_INP15/SCT_GPI2/FC7_RXD_SDA_MOSI_DATA/HS_SPI_SSEL0/PLU_IN5/SECURE_GPIO0_20/FC4_TXD_SCL_MISO_WS}
 * {pin_num: '1', PIO1_4 is the Mux_Select for switching between GP/Gowin SPI Flash access
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

    // PIO1_4 is the Mux_Select
    IOCON->PIO[1][4] = IOCON_PIO_FUNC0 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    gpio_pin_config_t outputconf = { kGPIO_DigitalOutput, 0 };
    GPIO_PinInit(GPIO, 1, 4, &outputconf);
}

/*  GPIO_SPI_MUX output config for Flexcomm8
 *  PIO1_4 is the Mux_Select for switching between GP/Gowin Access
 *  param : 0 = Gowin Access  , 1 = GP Access
 */
void BOARD_SetSPIMux(uint8_t on) {
    GPIO_PinWrite(GPIO, 1, 4, on);
}

/* For future use - Schematic change  16Mhz MClk for Gowin on 96Mhz Clk
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
