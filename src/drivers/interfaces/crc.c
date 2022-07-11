/**
 * @file crc.c
 * @brief  CRC
 * @author davth
 * @version
 * @date 2022-05-11
 */

#include <stdint.h>
#include <unistd.h>

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

uint32_t crc32(uint32_t crc,
               const uint8_t * buf,
               size_t len) {
    static uint32_t table[256];
    static int have_table = 0;
    uint32_t rem;
    uint8_t octet;
    int i, j;
    const uint8_t * p, * q;

    /* This check is not thread safe; there is no mutex. */
    if (have_table == 0) {
        /* Calculate CRC table. */
        for (i = 0; i < 256; i++) {
            rem = (uint32_t)i;              /* remainder from polynomial division */
            for (j = 0; j < 8; j++) {
                if (rem & 1) {
                    rem >>= 1;
                    rem ^= 0xedb88320;
                } else {
                    rem >>= 1;
                }
            }
            table[i] = rem;
        }
        have_table = 1;
    }

    crc = ~crc;
    q = buf + len;
    for (p = buf; p < q; p++) {
        octet = *p;          /* Cast to unsigned octet. */
        crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
    }
    return ~crc;
}
