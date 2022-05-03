/**
 * @file crc.c
 * @brief  CRC
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version
 * @date 2020-08-26
 */

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "logger.h"
#include "crc_linux.h"

uint32_t _crc32(uint32_t crc, const char *buf, size_t len)
{
	static uint32_t table[256];
	static int have_table = 0;
	uint32_t rem;
	uint8_t octet;
	int i, j;
	const char *p, *q;

	/* This check is not thread safe; there is no mutex. */
	if (have_table == 0) {
		/* Calculate CRC table. */
		for (i = 0; i < 256; i++) {
			rem = i;  /* remainder from polynomial division */
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
		octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}

uint32_t calculate_bin_crc(const char *file)
{
	int fp = open(file, O_RDONLY);

	if (fp < 0) {
		LOG_ERROR("Failed to open file");
		return 0;
	}
	struct stat st;

	fstat(fp, &st);

	char *buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fp, 0);

	if (!buf) {
		LOG_ERROR("Failed to mmap file");
		return 0;
	}

	return _crc32(0, buf, st.st_size);
}
