#ifndef _GPMCU_CRC_H_
#define _GPMCU_CRC_H_

#include "fsl_crc.h"
#include "peripherals.h"

static inline uint32_t crc_run_crc32(uint8_t * buffer, size_t size) {
    CRC_WriteData(CRC_ENGINE, buffer, size);
    uint32_t checksum32 = CRC_Get32bitResult(CRC_ENGINE);

    CRC_Reset(CRC_ENGINE);
    CRCEngine_init(CRC_ENGINE, 0xFFFFFFFFU);
    return checksum32;
}

#endif /* _GPMCU_CRC_H_ */
