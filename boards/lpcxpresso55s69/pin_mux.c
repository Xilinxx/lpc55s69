/**
 * @file pin_mux.c
 * @brief  LPCXpresso55S69 Pin Mux configuration
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */
#include "fsl_common.h"
#include "pin_mux.h"

void BOARD_BootInitBootPins(void) {
    BOARD_InitDebugUART();
}

void BOARD_AppInitBootPins(void) {
    BOARD_InitDebugUART();
    BOARD_InitI2CMASTER();
}

void BOARD_InitDebugUART(void) {
    CLOCK_EnableClock(kCLOCK_Iocon);

    // RXD
    IOCON->PIO[1][24] = IOCON_PIO_FUNC1 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    // TXD
    IOCON->PIO[0][27] = IOCON_PIO_FUNC1 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    // RXD
    IOCON->PIO[0][29] = IOCON_PIO_FUNC1 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    // TXD
    IOCON->PIO[0][30] = IOCON_PIO_FUNC1 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;
}


void BOARD_InitI2CMASTER(void) {
    /* Enables the clock for the I/O controller.: Enable Clock. */
    CLOCK_EnableClock(kCLOCK_Iocon);

    /* I2C SDA PIO0_13 Alt-mode 1 */
    IOCON->PIO[0][13] = IOCON_PIO_FUNC1 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;

    /* I2C SCL PIO0_14 Alt-mode 1 */
    IOCON->PIO[0][14] = IOCON_PIO_FUNC1 |
        IOCON_PIO_MODE_INACT |
        IOCON_PIO_SLEW_STANDARD |
        IOCON_PIO_INV_DI |
        IOCON_PIO_DIGITAL_EN |
        IOCON_PIO_OPENDRAIN_DI;
}
