/**
 * @file    bootloader_usb_helpers.c
 * @brief   Bootloader usb helper functions
 * @author  davth
 * @version v0.1
 * @date    2022-6-10
 */

#include "bootloader_usb_helpers.h"

#include <stdint.h>
#include <string.h>

#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "peripherals.h"
#include "logger.h"

// Zeus
#include "si5341.h"
#include "usb5807c.h"

// Gaia
#include "usb7206c.h"

#ifdef BOARD_GAIA
static int upstream_cable_usb0 = 0;
static int upstream_cable_usb1 = 0;
#endif

#define FSL_FEATURE_I2C_TIMEOUT_RECOVERY 1U
#define I2C_RETRY_TIMES                  3U /* Define to zero means keep waiting until the flag is assert/deassert. */

// Zeus

int _initialize_usb_zeus(void) {
    BOARD_Set_USB_Reset(0);
    SDK_DelayAtLeastUs(100000, 96000000U);

    i2c_master_handle_t i2cmh;
    I2C_MasterTransferCreateHandle(BOARD_I2C_USB_PERIPHERAL, &i2cmh, NULL, NULL);

    usb5807c_init_ctxt(BOARD_I2C_USB_PERIPHERAL, &i2cmh);

    LOG_INFO("USB5807C Setting ResetN");
    BOARD_Set_USB_Reset(0);
    SDK_DelayAtLeastUs(100000, 96000000U);
    BOARD_Set_USB_Reset(1);
    SDK_DelayAtLeastUs(500000, 96000000U);

    if (usb5807c_enable_runtime_with_smbus() < 0) {
        return -1;
    }
    SDK_DelayAtLeastUs(100000, 96000000U);
    if (usb5807c_set_upstream_port(USB5807C_FLEXPORT1) < 0) {
        return -1;
    }
    int id = usb5807c_retrieve_id();
    if (id != -1) {
        LOG_INFO("USB ID: %d", id);
        LOG_INFO("USB VID: %x", usb5807c_retrieve_usb_vid());

        int rev = usb5807c_retrieve_revision();
        LOG_INFO("USB Revision: 0x%x", rev);
    } else {
        LOG_ERROR("USB ID: %d", id);
        LOG_ERROR("USB VID: %x", usb5807c_retrieve_usb_vid());
        return -1;
    }

    // Note that si5341 can not be configured when usb5807 is not configured
    // Clock generator
    si5341_init_ctxt(FLEXCOMM5_PERIPHERAL, &i2cmh);
    return si5341_dump_initial_regmap();
}


// Gaia

int _initialize_usb0_gaia(bool initialize_i2c) {
#ifdef BOARD_GAIA
    i2c_master_handle_t i2cmh;

    BOARD_Set_USB_Reset(1); // reset
    SDK_DelayAtLeastUs(50000, 96000000U);

    if (initialize_i2c)
        BOARD_AppInitI2CMASTERPeripherals();

    usb7206c_init_ctxt(BOARD_I2C_USB_PERIPHERAL, &i2cmh, USB7206C_I2C_ADDR);

    LOG_INFO("USB7206C Release USBHUB0 ResetN");
    BOARD_Set_USB_Reset(0); // release reset
    SDK_DelayAtLeastUs(100000, 96000000U);

    uint8_t timeout = 6;
    uint32_t revision = usb7206c_retrieve_revision();
    while (revision == (uint32_t)-1) {
        LOG_WARN("USB0 - waiting for hub to answer");
        revision = usb7206c_retrieve_revision();
        SDK_DelayAtLeastUs(500000, 96000000U);
        if (timeout-- == 0) {
            LOG_ERROR("USB0 - timeout");
            return -1;
        }
    }

    LOG_INFO("USB0 Revision: 0x%.8X , ID: 0x%.4X, VID: 0x%.4X",
             revision,
             usb7206c_retrieve_id(),
             usb7206c_retrieve_usb_vid());

    usb7206c_set_i2s_audio_disabled(); // prevent AUDIO detection

    /*
     * uint8_t vid[2] = { 0x34, 0x12 }; // LSB first!
     * usb7206c_set_usb_vid(vid);
     * LOG_INFO("Vendor id readback is %X",usb7206c_retrieve_usb_vid());
     */

    if (usb7206c_enable_runtime_with_smbus() < 0) {
        return -1;
    }

    upstream_cable_usb0 = BOARD_read_UsbUp0_nVbus();
    LOG_INFO("(UsbUp0 has %s)", upstream_cable_usb0 ? "NO CABLE" : "CABLE");
#endif

    return 0;
}

int _initialize_usb1_gaia(bool initialize_i2c) {
#ifdef BOARD_GAIA
    i2c_master_handle_t i2cmh;

    BOARD_Set_USB1_Reset(1); // reset
    SDK_DelayAtLeastUs(50000, 96000000U);

    if (initialize_i2c)
        BOARD_AppInitI2CMASTERPeripherals();

    usb7206c_init_ctxt(BOARD_I2C_USB_PERIPHERAL, &i2cmh, USB7206C_I2C_ADDR);

    LOG_INFO("USB7206C Release USBHUB1 ResetN");
    BOARD_Set_USB1_Reset(0); // release reset
    SDK_DelayAtLeastUs(100000, 96000000U);

    uint32_t revision = usb7206c_retrieve_revision();
    uint8_t timeout = 6;
    while (revision == (uint32_t)-1) {
        LOG_WARN("USB1 - waiting for hub to answer");
        revision = usb7206c_retrieve_revision();
        SDK_DelayAtLeastUs(500000, 96000000U);
        if (timeout-- == 0) {
            LOG_ERROR("USB1 - timeout");
            return -1;
        }
    }

    LOG_INFO("USB1 Revision: 0x%.8X , ID: 0x%.4X, VID: 0x%.4X",
             revision,
             usb7206c_retrieve_id(),
             usb7206c_retrieve_usb_vid());

    usb7206c_set_i2s_audio_disabled();

    uint8_t flex[1] = { 0x01 };
    usb7206c_set_generic_register(USB7206C_USB2_SYS_CONFIG, flex, sizeof(flex));
    usb7206c_set_generic_register(USB7206C_USB3_SYS_CONFIG, flex, sizeof(flex));

    LOG_DEBUG("USB1 FLEXCONNECT set to upstream MainCPU");

    // Example for reading BCD register. We expect 0x1002
    uint8_t bcd[2] = { 0, 0 };
    usb7206c_get_generic_register(USB7206C_USB2_VER_BCD, bcd, sizeof(bcd));
    int BCD = ((bcd[0] << 8) + bcd[1]);
    if (BCD != 0x1002) {
        LOG_WARN("USB1 USB7206C_USB2_VER_BCD(0x%.8X) [0x%.4X] <> 0x1002",
                 USB7206C_USB2_VER_BCD, BCD);
    }

    /* // For Debug purpose - Example Setting the VID
     * uint8_t vid[2] = { 0x56, 0x78 }; // LSB first!
     * usb7206c_set_usb_vid(vid);
     * LOG_INFO("USB1 Vendor id readback is %X",usb7206c_retrieve_usb_vid());
     * BOARD_Set_USB1_Reset(1); // reset
     * SDK_DelayAtLeastUs(100000, 96000000U);
     * LOG_INFO("USB0 Vendor id readback is %X",usb7206c_retrieve_usb_vid());
     */

    uint8_t adr[1];
    adr[0] = usb7206c_get_primary_i2c_address_reg();
    LOG_DEBUG("USB1 PRIMARY ADDRESS is %X", adr[0]);
    adr[0] = USB7206C_USBHUB1_NEW_I2C_ADDR;
    usb7206c_set_primary_i2c_address_reg(adr);

    // As from here hub0 i2c address is 0x2e
    usb7206c_init_ctxt(BOARD_I2C_USB_PERIPHERAL, &i2cmh, USB7206C_USBHUB1_NEW_I2C_ADDR);

    adr[0] = usb7206c_get_primary_i2c_address_reg();
    LOG_INFO("New USB1 PRIMARY address is %X, Secondary address is disabled.", adr[0]);
    adr[0] = USB7206C_I2C_DISABLE_ADDR;
    usb7206c_set_secondary_i2c_address_reg(adr);

    // Attach the SMBus = Runmode
    if (usb7206c_enable_runtime_with_smbus() < 0) {
        return -1;
    }

    upstream_cable_usb1 = BOARD_read_UsbUp1_nVbus();
    LOG_INFO("(UsbUp1 has %s)", upstream_cable_usb1 ? "NO CABLE" : "CABLE");
#endif
    return 0;
}


void _detect_cable_change_gaia() {
#ifdef BOARD_GAIA
    if (BOARD_read_UsbUp0_nVbus() != upstream_cable_usb0) {
        upstream_cable_usb0 = BOARD_read_UsbUp0_nVbus();
        LOG_INFO("(UsbUp0 has %s)", upstream_cable_usb0 ? "NO CABLE" : "CABLE");
        if (upstream_cable_usb0 == 1) {
            uint8_t flex[1] = { 0x01 };
            usb7206c_set_generic_register(USB7206C_USB2_SYS_CONFIG, flex, sizeof(flex));
            usb7206c_set_generic_register(USB7206C_USB3_SYS_CONFIG, flex, sizeof(flex));
        }
        BOARD_toggle_USB0_VbusUp(200);
    }

    if (BOARD_read_UsbUp1_nVbus() != upstream_cable_usb1) {

        upstream_cable_usb1 = BOARD_read_UsbUp1_nVbus();
        LOG_INFO("(UsbUp1 has %s)", upstream_cable_usb1 ? "NO CABLE" : "CABLE");
        if (upstream_cable_usb1 == 1) {
            uint8_t flex[1] = { 0x00 };
            usb7206c_set_generic_register(USB7206C_USB2_SYS_CONFIG, flex, sizeof(flex));
            usb7206c_set_generic_register(USB7206C_USB3_SYS_CONFIG, flex, sizeof(flex));
        }
        BOARD_toggle_USB0_VbusUp(200);
    }
#endif
}

