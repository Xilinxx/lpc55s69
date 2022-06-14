/**
 * @file commp_cmd_helpers.c
 * @brief  COMMP Stack command helpers
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version
 * @date 2020-11-05
 */

#include "comm_protocol.h"
#include "comm_parser.h"
#include "logger.h"

uint32_t _comm_get_crc(uint8_t * data) {
    uint32_t crc32 = (uint32_t)((data[COMMP_CMD_CRC_BYTE0] << 24) |
                                (data[COMMP_CMD_CRC_BYTE1] << 16) |
                                (data[COMMP_CMD_CRC_BYTE2] << 8) |
                                data[COMMP_CMD_CRC_BYTE3]);

    return crc32;
}

/* _comm_send_generic_cmd
 *       Created for passing the comm_proto_cmd_t-typedef
 */
int _comm_send_generic_cmd(struct comm_driver_t * cdriver, comm_proto_cmd_t cmd) {
    uint8_t out_buffer[COMMP_CMD_PACKET_SIZE];

    out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
    out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
    out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
    out_buffer[COMMP_CMD_CMDCODE_LSB] = cmd;

    int error =
        comm_protocol_write_data(cdriver, out_buffer, COMMP_CMD_PACKET_SIZE);
    if (!error) {
        LOG_ERROR("Send of generic cmd [%d - %s] Failed", cmd,
                  comm_protocol_cmd_to_string(cmd));
    }
    return error;
}

int _comm_send_crc(struct comm_driver_t * cdriver, uint32_t crc) {
    uint8_t out_buffer[COMMP_CMD_CRC_PACKET_SIZE];

    out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
    out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
    out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
    out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_CRC;

    out_buffer[COMMP_CMD_CRC_BYTE0] = (uint8_t)((crc & 0xff000000) >> 24);
    out_buffer[COMMP_CMD_CRC_BYTE1] = (uint8_t)((crc & 0x00ff0000) >> 16);
    out_buffer[COMMP_CMD_CRC_BYTE2] = (uint8_t)((crc & 0x0000ff00) >> 8);
    out_buffer[COMMP_CMD_CRC_BYTE3] = (uint8_t)(crc & 0x000000ff);

    LOG_OK("%x %x %x %x", out_buffer[COMMP_CMD_CRC_BYTE0],
           out_buffer[COMMP_CMD_CRC_BYTE1],
           out_buffer[COMMP_CMD_CRC_BYTE2],
           out_buffer[COMMP_CMD_CRC_BYTE3]);

    int error = comm_protocol_write_data(cdriver, out_buffer,
                                         COMMP_CMD_CRC_PACKET_SIZE);
    if (!error) {
        LOG_ERROR("Send of CRC Failed");
    }
    return error;
}

int _comm_send_end(struct comm_driver_t * cdriver, uint8_t romnr) {
    uint8_t out_buffer[COMMP_CMD_END_PACKET_SIZE];

    out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
    out_buffer[COMMP_OPCODE_BYTE] = COMMP_CMD;
    out_buffer[COMMP_CMD_CMDCODE_MSB] = 0;
    if (romnr == COMMP_ROMID_SPIFLASH0)
        out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_SPI_END;
    else
        out_buffer[COMMP_CMD_CMDCODE_LSB] = COMMP_CMD_END;

    int error = comm_protocol_write_data(cdriver, out_buffer,
                                         COMMP_CMD_END_PACKET_SIZE);
    if (!error) {
        LOG_ERROR("Send of END Failed");
    }
    return error;
}

