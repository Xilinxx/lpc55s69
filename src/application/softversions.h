#ifndef _SOFTVERSIONS_H_
#define _SOFTVERSIONS_H_
/* softversions.h
**
** Here the recent software version is defined
** This file is used to keep track of the software changes
**
** (C) Barco
**
******************************************************************************/

// history
// ******************************************************************************
#define BOOTLOADER_VERSION "0.2.0"

// history BOOT
//
// V0.0.1   22/04/2021   bram vlerick: - initial version
// V0.2.0   15/06/2022   davth: - memory map reconfigured
//                              - spi flash update


#define SOFTVERSIONBOOTMAJOR 0
#define SOFTVERSIONBOOTMINOR 2
#define SOFTVERSIONBOOTBUILD 0

// history APP
//
// V0.0.1   25/05/2021   davth: - initial version
// V0.1.4   08/02/2021   davth: - synchronize with git Tag (previous was v0.1.3)
// V0.1.6   13/06/2022   davth: - init usb hub for Gaia included
// V0.2.0   15/06/2022   davth: - synchronize tag with bootloader

#define SOFTVERSIONRUNMAJOR 0
#define SOFTVERSIONRUNMINOR 2
#define SOFTVERSIONRUNBUILD 0

#endif /*_SOFTVERSIONS_H_*/
