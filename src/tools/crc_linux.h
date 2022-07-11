/**
 * @file crc.h
 * @brief CRC function
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version
 * @date 2020-08-26
 */

#ifndef _TOOLS_CRC_H_
#define _TOOLS_CRC_H_

#include <stdint.h>

/**
 * @brief  Calculate the CRC of a given file
 *
 * @param file File path + filename
 *
 * @returns   32bit CRC
 */
uint32_t calculate_bin_crc(const char * file);

/**
 * @brief  Calculate the 32bit CRC of a given buffer
 *
 * @param crc Seed for the CRC
 * @param buf Buffer that will be CRC'd
 * @param len Length of the CRC
 *
 * @returns 0 or the CRC
 *
 * @note: Code from: https://rosettacode.org/wiki/CRC-32#C
 */
uint32_t _crc32(uint32_t crc,
                const char * buf,
                size_t len);

#endif /* _TOOLS_CRC_H_ */
