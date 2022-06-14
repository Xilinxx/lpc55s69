/**
 * @file tmds181_regmap.h
 * @brief  TI TMDS181
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-09-21
 */

#ifndef _GP_DRV_TMDS181_REGMAP_
#define _GP_DRV_TMDS181_REGMAP_

/* Device id (should spell TMDS181) */
#define TMDS181_DEVICE_ID0    0x00
#define TMDS181_DEVICE_ID1    0x01
#define TMDS181_DEVICE_ID2    0x02
#define TMDS181_DEVICE_ID3    0x03
#define TMDS181_DEVICE_ID4    0x04
#define TMDS181_DEVICE_ID5    0x05
#define TMDS181_DEVICE_ID6    0x06
#define TMDS181_DEVICE_ID7    0x07

/* Revision ID */
#define TMDS181_DEVICE_REV_ID 0x08

/* Misc control registers */
#define TMDS181_MISC_CTRL0    0x09
#define TMDS181_MISC_CTRL1    0x0A
#define TMDS181_MISC_CTRL2    0x0B
#define TMDS181_MISC_CTRL3    0x0C

/* Equalizer control register */
#define TMDS181_EQ_CTRL0      0x0D

/* RX Pattern Verifier Registers */
#define TMDS181_RX_PAT_VCSR0  0x0E
#define TMDS181_RX_PAT_VCSR1  0x0F
#define TMDS181_RX_PAT_VCSR2  0x10
#define TMDS181_RX_PAT_VCSR3  0x11
#define TMDS181_RX_PAT_VCSR4  0x12
#define TMDS181_RX_PAT_VCSR5  0x13
#define TMDS181_RX_PAT_VCSR6  0x14
#define TMDS181_RX_PAT_VCSR7  0x15
#define TMDS181_RX_PAT_VCSR8  0x16
#define TMDS181_RX_PAT_VCSR9  0x17
#define TMDS181_RX_PAT_VCSR10 0x18
#define TMDS181_RX_PAT_VCSR11 0x19
#define TMDS181_RX_PAT_VCSR12 0x1A
#define TMDS181_RX_PAT_VCSR13 0x1B
#define TMDS181_RX_PAT_VCSR14 0x1C
#define TMDS181_RX_PAT_VCSR15 0x1D
#define TMDS181_RX_PAT_VCSR16 0x1E
#define TMDS181_RX_PAT_VCSR17 0x1F
#define TMDS181_RX_PAT_VCSR18 0x20   // Power / Standby status

#endif /* _GP_DRV_TMDS181_REGMAP_ */
