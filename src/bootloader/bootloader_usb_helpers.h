#ifndef _BOOTLOADER_USB_HELPER_H_
#define _BOOTLOADER_USB_HELPER_H_
/**
 * @file bootloader_usb_helpers.h
 * @brief  Bootloader usb helper functions
 * @version v0.1
 * @date 2022-06-10
 */

#define USB7206C_USBHUB1_NEW_I2C_ADDR USB7206C_I2C_ADDR + 1   // !< I2C Address for USB7206C hub

/**
 * @brief  _initialize_usb_zeus
 *          usb initialisation for zeus
 *
 * @returns -1 if failure
 */
int _initialize_usb_zeus(void);

/**
 * @brief  _initialize_usb0_gaia
 *          usb initialisation for gaia USBHUB0
 *          This hub needs to be iniitalized last. Orginal address remains!
 *
 * @returns -1 if failure
 */
int _initialize_usb0_gaia(void);

/**
 * @brief  _initialize_usb1_gaia
 *          usb initialisation for gaia USBHUB1
 *          For address switching, this hub needs the be initialized first!
 *
 * @returns -1 if failure
 */
int _initialize_usb1_gaia(void);

/**
 * @brief  _detect_cable_change_gaia
 *         This function is meant for application code IO switching.
 *         Usb initialisation is forced by toggling the UsbHub0_nVBusUp line
 *         Flexconnect is switched if upstream connection changes
 *         (detection takes place with cable-detect IO lines)
 * @returns -1 if failure
 */
void _detect_cable_change_gaia(void);

#endif