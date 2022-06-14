/**
 * @file comm_protocol.c
 * @brief  COMMP Stack implementation
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-10-13
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "comm_parser.h"
#include "comm_protocol.h"
#include "logger.h"

extern struct comm_driver_t uart_comm;
extern struct comm_driver_t serial_comm;
extern struct comm_driver_t tcp_comm;
extern struct comm_driver_t unit_comm;

static struct comm_driver_t * cdrivers[] = {
#ifdef COMM_DRIVER_UART
    &uart_comm,
#endif
#ifdef COMM_DRIVER_SERIAL
    &serial_comm,
#endif
#ifdef COMM_DRIVER_TCP
    &tcp_comm,
#endif
#ifdef COMM_DRIVER_UNIT
    &unit_comm,
#endif
    NULL,
};

static struct comm_ctxt_t run_transfer_ctxt;

/* Get rid of static in case of unit test */
#ifdef UNIT_TEST
#define static
#endif


/* Helpers interfaces/commp_cmd_helpers.c implementation */
int _comm_send_generic_cmd(struct comm_driver_t * cdriver, comm_proto_cmd_t cmd);
int _comm_send_crc(struct comm_driver_t * cdriver, uint32_t crc);

uint32_t _comm_get_crc(uint8_t * data);

/* Helpers from commp_opcode_helpers.c */
void _reset_transfer_ctxt(struct comm_ctxt_t * ctxt);
void _clear_transfer_ctxt(struct comm_ctxt_t * ctxt);
int _comm_send_wrq(struct comm_driver_t * cdriver, struct comm_ctxt_t * ctxt);
int _comm_send_rrq(struct comm_driver_t * cdriver, struct comm_ctxt_t * ctxt);
int _comm_send_ack(struct comm_driver_t * cdriver, struct comm_ctxt_t * ctxt);
int _comm_send_err(struct comm_driver_t * cdriver, uint16_t errorcode, char * errorstr);

int comm_protocol_retrieve_binary(struct comm_driver_t * cdriver, struct storage_driver_t * sdriver, struct comm_ctxt_t * ctxt);

static int _run_driver_init() {
    for (int i = 0; cdrivers[i] != NULL; i++) {
        if (cdrivers[i]->enabled && cdrivers[i]->ops) {
            if (cdrivers[i]->ops->init) {
                int error = cdrivers[i]->ops->init(
                    (void *)cdrivers[i]);
                if (error < 0) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

int comm_protocol_init() {
    int err = _run_driver_init();

    if (err < 0) {
        LOG_ERROR("Failed to run init");
        return -1;
    }
    LOG_OK("Comm Protocol initialized");

    return 0;
}

bool comm_protocol_is_packet_type(uint8_t * data, comm_proto_opcode_t type) {
    if (data[COMMP_OPCODE_BYTE] == type) {
        return true;
    }
    return false;
}

uint16_t comm_protocol_get_command_type(uint8_t * data) {
    uint16_t cmdcode = (uint16_t)((data[COMMP_CMD_CMDCODE_MSB] << 8) |
                                  data[COMMP_CMD_CMDCODE_LSB]);

    return cmdcode;
}

bool comm_protocol_is_command_type(uint8_t * data, comm_proto_cmd_t type) {
    uint16_t cmdcode = comm_protocol_get_command_type(data);

    if (cmdcode == type) {
        return true;
    }
    return false;
}

int comm_protocol_write_data(struct comm_driver_t * drv, uint8_t * data,
                             size_t len) {
    int written = -1;

    if (drv->enabled && drv->ops) {
        if (drv->ops->write) {
            written = drv->ops->write((void *)drv, data, len);
            if (written < 0) {
                return written;
            }
        }
    }
    return written;
}

int comm_protocol_read_data(struct comm_driver_t * drv, uint8_t * data,
                            size_t * len) {
    int read = -1;

    if (drv->enabled && drv->ops) {
        if (drv->ops->read) {
            read = drv->ops->read((void *)drv, data, len);
            if (read < 0) {
                return read;
            }
        }
    }
    return read;
}

struct comm_driver_t * comm_protocol_find_driver(char * name) {
    if (name == NULL) {
        return NULL;
    }

    for (int i = 0; cdrivers[i] != NULL; i++) {
        if (!strncmp(name, cdrivers[i]->name, MAX_COMM_NAME)) {
            return cdrivers[i];
        }
    }
    return NULL;
}

struct comm_ctxt_t _get_run_transfer_ctxt() {
    return run_transfer_ctxt;
}


int comm_protocol_run(struct bootloader_ctxt_t * bctxt,
                      struct comm_driver_t * cdriver,
                      struct storage_driver_t * sdriver,
                      struct storage_driver_t * spidriver,
                      struct spi_ctxt_t * spi_ctxt) {
    int err = 0;
    bool end_stack = false;
    uint8_t recv_buffer[COMMP_PACKET_SIZE];

    if (!cdriver) {
        LOG_ERROR("Invalid cdriver");
        return COMMP_CMD_ERROR;
    }

    LOG_INFO("Running COMMP Stack");

    _clear_transfer_ctxt(&run_transfer_ctxt);
    run_transfer_ctxt.parent = cdriver;

    if (sdriver) {
        cdriver->sdriver = sdriver;
    }

    comm_proto_cmd_t retval = 0;
    while (!end_stack) {
        bool needs_ack = false;
        bool reset_ctxt = false;

        size_t readlen = COMMP_PACKET_SIZE;
        memset(recv_buffer, 0, COMMP_PACKET_SIZE);

        err = comm_protocol_read_data(cdriver, recv_buffer, &readlen);
        if (err < 0) {
            LOG_ERROR("Read failed...");
            return COMMP_CMD_ERROR;
        } else {
            // LOG_INFO("Read succeeded... %d bytes",readlen);
            if (readlen == 0) {
                LOG_ERROR("..received 0 bytes? retry..");
                err = comm_protocol_read_data(cdriver, recv_buffer, &readlen);
                if (readlen == 0)
                    return COMMP_CMD_ERROR;
            }
        }

        err = comm_protocol_parse_packet(&run_transfer_ctxt, recv_buffer, readlen);
        if (err < 0) {
            LOG_ERROR("Failed to parse packet");
            _reset_transfer_ctxt(&run_transfer_ctxt);
            return COMMP_CMD_ERROR;
        }
        // LOG_INFO("Parsed... %d bytes - return code is %d", readlen, err);

        if (comm_protocol_is_packet_type(recv_buffer, COMMP_RRQ)) {
            // arriving struct is comm_proto_rrq_wrq_t : zero byte + opcode + device string
            run_transfer_ctxt.rom_readsize = (size_t)(recv_buffer[COMMP_RRQ_LENGTH_OFFSET] << 24) +
                (recv_buffer[COMMP_RRQ_LENGTH_OFFSET + 1] << 16) +
                (recv_buffer[COMMP_RRQ_LENGTH_OFFSET + 2] << 8) +
                (recv_buffer[COMMP_RRQ_LENGTH_OFFSET + 3]);
            char device[strlen(COMMP_ROMNAME_ROM0)];             // Keep ROMNAME-length identical!
            memcpy(device, recv_buffer + COMMP_RRQ_ROMNAME_OFFSET, strlen(COMMP_ROMNAME_ROM0));
            if (!strcmp((char *)device, COMMP_ROMNAME_SPIFLASH1)) {
                run_transfer_ctxt.part_nr = COMMP_ROMID_SPIFLASH1;
            } else {
                run_transfer_ctxt.part_nr = COMMP_ROMID_SPIFLASH0;                 // spi0  -> use spidriver
            }
            LOG_INFO("ReadRequest device[%s] size[%d]", device, run_transfer_ctxt.rom_readsize);
            comm_protocol_retrieve_binary(cdriver, spidriver, &run_transfer_ctxt);
            run_transfer_ctxt.transfer_in_progress = false;
            storage_flush_storage(spidriver);
        } else if (comm_protocol_is_packet_type(recv_buffer, COMMP_WRQ)) {
            LOG_INFO("WriteRequest for device [%d]", run_transfer_ctxt.part_nr);
            needs_ack = true;
            if ((run_transfer_ctxt.part_nr == COMMP_ROMID_SPIFLASH0) ||
                (run_transfer_ctxt.part_nr == COMMP_ROMID_SPIFLASH1)) {
                storage_flush_storage(spidriver);
                cdriver->sdriver = spidriver;
            } else {           // COMMP_ROMID_ROM0/1
                storage_flush_storage(sdriver);
                cdriver->sdriver = sdriver;
            }
        } else if (comm_protocol_is_packet_type(recv_buffer, COMMP_DATA)) {
            LOG_DEBUG("Data packet for device [%d]", run_transfer_ctxt.part_nr);
            needs_ack = true;
        } else if (comm_protocol_is_packet_type(recv_buffer, COMMP_CMD)) {
            switch (comm_protocol_get_command_type(recv_buffer)) {
                // CRC CMD is the 'END' after a binary transfer
                case COMMP_CMD_CRC:
                    if ((run_transfer_ctxt.part_nr == COMMP_ROMID_SPIFLASH0) ||
                        (run_transfer_ctxt.part_nr == COMMP_ROMID_SPIFLASH1)) {
                        gowin_partition_t partition = GOWIN_PARTITION_0;
                        if ((run_transfer_ctxt.part_nr == COMMP_ROMID_SPIFLASH0))
                            partition = GOWIN_PARTITION_0;
                        else
                            partition = GOWIN_PARTITION_1;

                        size_t filesize = (size_t)storage_flush_storage(spidriver);
                        uint32_t rcrc = _comm_get_crc(recv_buffer);
                        uint32_t crc = storage_crc_storage(spidriver, filesize);
                        if (rcrc == crc) {
                            LOG_OK("CRC received for spi%d is 0x%X == 0x%X calculated",
                                   partition, rcrc, crc);
                            // Put the partition context in place
                            if (spi_ctxt->gowin[partition].image_size == 0)
                                LOG_WARN("missing spi ctxt initialisation");
                            spi_ctxt->gowin[partition].bitfile_size = (uint32_t)filesize;
                            spi_ctxt->gowin[partition].bitfile_crc = crc;
                            spi_ctxt->gowin[partition].crc = storage_crc_storage(spidriver, spi_ctxt->gowin[partition].image_size);
                            spi_ctxt->part = partition;                     // will contain last flashed partition
                            _comm_send_ack(cdriver, &run_transfer_ctxt);
                            return COMMP_CMD_WRITE_SPI_CTXT;                     // triggers storage of context in main.c
                            end_stack = true;
                        } else
                            LOG_ERROR("CRC received for spi is 0x%X <> 0x%X calculated", rcrc, crc);
                        return COMMP_CMD_ERROR;
                    } else {
                        storage_flush_storage(sdriver);
                        uint32_t rcrc = _comm_get_crc(recv_buffer);
                        uint32_t crc = storage_crc_storage(sdriver, 0);
                        LOG_INFO("CRC for partition nr %d is 0x%X", bctxt->part, crc);
                        bctxt->apps[bctxt->part].crc = crc;
#ifndef UNIT_TEST
                        if (crc != rcrc) {
                            LOG_ERROR("CRC's don't match!");
                            return COMMP_CMD_ERROR;
                        }
#endif
                        _comm_send_ack(cdriver, &run_transfer_ctxt);
                        return COMMP_CMD_CRC;
                    }
                    needs_ack = true;
                    reset_ctxt = true;
                    end_stack = true;
                    break;
                case COMMP_CMD_RESET:
                case COMMP_CMD_END:
                case COMMP_CMD_SPI_END:
                case COMMP_CMD_BOOT:
                case COMMP_CMD_BOOTINFO:
                case COMMP_CMD_PWR_ON:
                case COMMP_CMD_PWR_OFF:
                case COMMP_CMD_RECONFIG_GOWIN:
                case COMMP_CMD_TRIGGER_WDOG:
                case COMMP_CMD_ERASE_SPI0:
                case COMMP_CMD_ERASE_SPI1:
                case COMMP_CMD_WRITE_SPI_CTXT:
                case COMMP_CMD_SET_SPI0:
                case COMMP_CMD_SET_SPI1:
                case COMMP_CMD_SET_ROM0:
                case COMMP_CMD_SET_ROM1:
                case COMMP_CMD_INFO_SPI:
                    end_stack = true;
                    __attribute__((fallthrough));
                /* fall through */
                case COMMP_CMD_SWAP:
                    reset_ctxt = true;
                    retval = comm_protocol_get_command_type(recv_buffer);
                    break;
                default:
                    end_stack = true;
                    retval = COMMP_CMD_ERROR;
                    break;
            }
        }

        if (needs_ack) {
            err = _comm_send_ack(cdriver, &run_transfer_ctxt);
            if (err < 0) {
                return COMMP_CMD_ERROR;
            }
        }

        if (reset_ctxt) {
            _reset_transfer_ctxt(&run_transfer_ctxt);
        }

    }
    return retval;
}


// comm_protocol_transfer_binary - flash_tool code
int comm_protocol_transfer_binary(struct comm_driver_t * cdriver,
                                  uint8_t * buffer, size_t len, uint8_t rom_nr, uint32_t crc) {
    struct comm_ctxt_t transfer_ctxt = {
        .transfer_in_progress = false,
        .crc_received         = false,
        .last_blocknr         = 0,
        .parent               = cdriver,
        .part_nr              = rom_nr    // -> COMMP_ROMID_ROM0/1, COMMP_ROMID_SPIFLASH0
    };

    uint16_t i = 0;
    uint16_t blocknr = 0;
    int packets = (int)len / COMMP_DATA_SIZE;
    uint8_t out_buffer[COMMP_PACKET_SIZE] = { 0 };
    uint8_t in_buffer[COMMP_PACKET_SIZE] = { 0 };
    int error = 0;
    size_t readsize = COMMP_PACKET_SIZE;

    error = _comm_send_wrq(cdriver, &transfer_ctxt);
    if (error < 0) {
        _reset_transfer_ctxt(&transfer_ctxt);
        return -1;
    }

    LOG_DEBUG("Waiting for WRQ ACK");

    error = comm_protocol_read_data(cdriver, in_buffer, &readsize);
    if (error < 0) {
        LOG_ERROR("Read failed");
        return -1;
    }
    if (comm_protocol_is_packet_type(in_buffer, COMMP_ACK)) {
        LOG_OK("Received for WRQ ACK (%d) bytes", error);
    } else {
        LOG_ERROR("Packet is not ACK");
        return -1;
    }
    LOG_INFO("Will transfer %d packets", packets);

    transfer_ctxt.transfer_in_progress = true;

    out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
    out_buffer[COMMP_OPCODE_BYTE] = COMMP_DATA;

    /* Transfer packets equal to COMMP_DATA_SIZE */
    for (i = 0, blocknr = 1; i < packets; i++, blocknr++) {
        if ((i % 50 == 0) || (i > (packets - 5))) {  // reduce output
            LOG_INFO("Transfering packet %d of %d", i, packets);
        }
        memcpy(&out_buffer[COMMP_DATA_OFFSET], &buffer[i * COMMP_DATA_SIZE],
               COMMP_DATA_SIZE);
        out_buffer[COMMP_DATA_PNUMBER_MSB] = (uint8_t)((blocknr & 0xff00) >> 8);
        out_buffer[COMMP_DATA_PNUMBER_LSB] = (uint8_t)(blocknr & 0x00ff);
        error = comm_protocol_write_data(cdriver, out_buffer, COMMP_PACKET_SIZE);
        if (error != COMMP_PACKET_SIZE) {
            LOG_ERROR("Send failed, did not send requested bytes in packet %d", i);
            return -1;
        }

        readsize = COMMP_PACKET_SIZE;
        transfer_ctxt.last_blocknr = blocknr;

        error = comm_protocol_read_data(cdriver, in_buffer, &readsize);
        if (error < 0) {
            LOG_ERROR("Transfered packet got no ACK reply");
            return -1;
        }
        /* This can be better.. */
        if (comm_protocol_is_packet_type(in_buffer, COMMP_ACK) ||
            comm_protocol_is_packet_type(in_buffer, COMMP_ERR)) {
            error = comm_protocol_parse_packet(&transfer_ctxt,
                                               in_buffer,
                                               readsize);
            if (error < 0) {
                LOG_ERROR("Failed to parse packet");
                _reset_transfer_ctxt(&transfer_ctxt);
                return -1;
            }
        } else {
            _comm_send_err(cdriver, COMMP_ERR_SEQ, "Invalid sequence of packets");

            LOG_ERROR("Received packet makes no sense %d %d", in_buffer[0], in_buffer[1]);

            return -1;
        }
    }

    size_t remaining = (len - (size_t)(packets * COMMP_DATA_SIZE));
    if (remaining != 0) {
        LOG_INFO("Sending last packet of %d bytes", remaining);
        memcpy(&out_buffer[COMMP_DATA_OFFSET], &buffer[i * COMMP_DATA_SIZE],
               remaining);
        out_buffer[COMMP_DATA_PNUMBER_MSB] = (uint8_t)((blocknr & 0xff00) >> 8);
        out_buffer[COMMP_DATA_PNUMBER_LSB] = (uint8_t)(blocknr & 0x00ff);
        error = comm_protocol_write_data(cdriver, out_buffer,
                                         remaining + COMMP_DATA_OFFSET);

        readsize = COMMP_PACKET_SIZE;
        transfer_ctxt.last_blocknr = blocknr;

        error = comm_protocol_read_data(cdriver, in_buffer, &readsize);
        if (error < 0) {
            LOG_ERROR("Read failed");
            _reset_transfer_ctxt(&transfer_ctxt);
            return -1;
        }
        if (comm_protocol_is_packet_type(in_buffer, COMMP_ACK) ||
            comm_protocol_is_packet_type(in_buffer, COMMP_ERR)) {
            error = comm_protocol_parse_packet(&transfer_ctxt,
                                               in_buffer,
                                               readsize);
            if (error < 0) {
                LOG_ERROR("Failed to parse packet");
                _reset_transfer_ctxt(&transfer_ctxt);
                return -1;
            }
        } else {
            LOG_ERROR("Received packet makes no sense");
            return -1;
        }
    }

    LOG_OK("Sending CRC! 0x%X", crc);
    error = _comm_send_crc(cdriver, crc);
    if (error < 0) {
        LOG_ERROR("Failed to send crc");
    }

    /* Dummy ack read for synchronisation */
    error = comm_protocol_read_data(cdriver, in_buffer, &readsize);
    if (error < 0) {
        LOG_ERROR("Read failed");
        _reset_transfer_ctxt(&transfer_ctxt);
        return -1;
    }

    if (rom_nr == COMMP_ROMID_SPIFLASH0)
        error = _comm_send_generic_cmd(cdriver, COMMP_CMD_SPI_END);
    else
        error = _comm_send_generic_cmd(cdriver, COMMP_CMD_END);

    if (error < 0) {
        LOG_ERROR("Failed to send crc");
    }

    _reset_transfer_ctxt(&transfer_ctxt);
    LOG_INFO("Transfer done");
    return 0;
}

/* comm_protocol_retrieve_binary - gpmcu code
 * Flash is read and sent back to the flash_tool
 */
int comm_protocol_retrieve_binary(struct comm_driver_t * cdriver,
                                  struct storage_driver_t * sdriver,
                                  struct comm_ctxt_t * ctxt) {
    struct comm_ctxt_t transfer_ctxt =
    {
        .part_nr              = ctxt->part_nr,
        .transfer_in_progress = false,
        .crc_received         = false,
        .last_blocknr         = 0,
        .parent               = cdriver,
        .rom_readsize         = ctxt->rom_readsize
    };

    _reset_transfer_ctxt(&transfer_ctxt);

    storage_flush_storage(sdriver);

    uint16_t i = 0;
    uint16_t blocknr = 0;
    int packets = (int)transfer_ctxt.rom_readsize / COMMP_DATA_SIZE;
    // Packet fits in 1 message is == COMMP_DATA_SIZE but we don't want 2 packets sent
    if (transfer_ctxt.rom_readsize == COMMP_DATA_SIZE) {
        packets = 0;
    }
    uint8_t out_buffer[COMMP_PACKET_SIZE] = { 0 };
    uint8_t in_buffer[COMMP_PACKET_SIZE] = { 0 };

    int error;

    size_t readsize;

    // LOG_DEBUG("Sending bin data");
    LOG_INFO("Will transfer %d packets (a total of %d bytes)", packets + 1, ctxt->rom_readsize);

    transfer_ctxt.transfer_in_progress = true;

    out_buffer[COMMP_OPCODE_ZERO_BYTE] = 0;
    out_buffer[COMMP_OPCODE_BYTE] = COMMP_DATA;

    readsize = COMMP_PACKET_SIZE;

    transfer_ctxt.transfer_in_progress = true;
    // Transfer packets equal to COMMP_DATA_SIZE
    for (i = 0, blocknr = 0; i < packets; i++, blocknr++) {
        if ((i % 50 == 0) || (i > (packets - 5)))  // reduce output
            LOG_INFO("Tx packet %d of %d", i, packets);

        out_buffer[COMMP_DATA_PNUMBER_MSB] = (uint8_t)((blocknr & 0xff00) >> 8);
        out_buffer[COMMP_DATA_PNUMBER_LSB] = (uint8_t)(blocknr & 0x00ff);

        /* Fetch the data from the selected rom */
        error = storage_read_data(sdriver, (uint8_t *)out_buffer + 4, COMMP_DATA_SIZE);
        if (error != COMMP_DATA_SIZE) {
            LOG_ERROR("Failed to retrieve rom data %d", error);
            return -1;
        }

        error = comm_protocol_write_data(cdriver, out_buffer, COMMP_PACKET_SIZE);
        if (error != COMMP_PACKET_SIZE) {
            LOG_ERROR("Send failed, packet size fault %d<>%d", i, COMMP_PACKET_SIZE);
            return -1;
        }

        readsize = COMMP_PACKET_SIZE;
        transfer_ctxt.last_blocknr = blocknr;

        error = comm_protocol_read_data(cdriver, in_buffer, &readsize);
        if (error < 0) {
            LOG_ERROR("Read failed");
            return -1;
        }
        if (comm_protocol_is_packet_type(in_buffer, COMMP_ACK) ||
            comm_protocol_is_packet_type(in_buffer, COMMP_ERR)) {
            error = comm_protocol_parse_packet(&transfer_ctxt,
                                               in_buffer,
                                               readsize);
            if (error < 0) {
                LOG_ERROR("Failed to parse packet");
                _reset_transfer_ctxt(&transfer_ctxt);
                return -1;
            }
        } else {
            _comm_send_err(cdriver, COMMP_ERR_SEQ, "Invalid sequence of packets");

            LOG_ERROR("Received packet makes no sense");
            LOG_ERROR("4byte [%x %x %x %x]", in_buffer[0], in_buffer[1], in_buffer[2], in_buffer[3]);

            return -1;
        }
    }

    // The final Block

    size_t remaining = transfer_ctxt.rom_readsize - (size_t)(packets * COMMP_DATA_SIZE);
    LOG_INFO("Tx last packet of 4 bytes header + %d data bytes", remaining);
    if (remaining != 0) {
        out_buffer[COMMP_DATA_PNUMBER_MSB] = (uint8_t)((blocknr & 0xff00) >> 8);
        out_buffer[COMMP_DATA_PNUMBER_LSB] = (uint8_t)(blocknr & 0x00ff);

        error = storage_read_data(sdriver, (uint8_t *)out_buffer + 4, remaining);
        if (error != (int)remaining) {
            LOG_ERROR("Failed to retrieve rom data %d", error);
            return -1;
        }

        error = comm_protocol_write_data(cdriver, out_buffer,
                                         remaining + COMMP_DATA_OFFSET);
        if (error < 0) {
            LOG_ERROR("DATA Write failed");
            _reset_transfer_ctxt(&transfer_ctxt);
            return -1;
        }

        transfer_ctxt.last_blocknr = blocknr;

        // check ACK
        readsize = COMMP_PACKET_SIZE;
        error = comm_protocol_read_data(cdriver, in_buffer, &readsize);
        if (error < 0) {
            LOG_ERROR("Read failed");
            _reset_transfer_ctxt(&transfer_ctxt);
            return -1;
        }
        if (comm_protocol_is_packet_type(in_buffer, COMMP_ACK) ||
            comm_protocol_is_packet_type(in_buffer, COMMP_ERR)) {
            error = comm_protocol_parse_packet(&transfer_ctxt,
                                               in_buffer,
                                               readsize);
            if (error < 0) {
                LOG_ERROR("Failed to parse packet");
                _reset_transfer_ctxt(&transfer_ctxt);
                return -1;
            }
        } else {
            LOG_ERROR("Received packet makes no sense!");
            LOG_ERROR("4byte [%x %x %x %x]", in_buffer[0], in_buffer[1], in_buffer[2], in_buffer[3]);
            return -1;
        }
    }


    if (transfer_ctxt.part_nr == COMMP_ROMID_SPIFLASH0 || transfer_ctxt.part_nr == COMMP_ROMID_SPIFLASH1)
        error = _comm_send_generic_cmd(cdriver, COMMP_CMD_SPI_END);
    else
        error = _comm_send_generic_cmd(cdriver, COMMP_CMD_END);

    if (error < 0) {
        LOG_ERROR("Failed to send crc");
    }

    _reset_transfer_ctxt(&transfer_ctxt);
    LOG_INFO("bin transfer done");
    return 0;
}


struct bootloader_ctxt_t comm_protocol_retrieve_bootinfo(struct comm_driver_t * cdriver) {
    _comm_send_generic_cmd(cdriver, COMMP_CMD_BOOTINFO);
    uint8_t out_buffer[COMMP_CMD_PACKET_SIZE + sizeof(struct bootloader_ctxt_t)];
    size_t len = (COMMP_PACKET_SIZE + sizeof(struct bootloader_ctxt_t));
    int err = comm_protocol_read_data(cdriver, out_buffer, &len);
    if (err < 0) {
        LOG_ERROR("Failed to read bootinfo");
    }

    struct bootloader_ctxt_t ctxt;
    memcpy(&ctxt, &out_buffer[COMMP_CMD_PACKET_SIZE], sizeof(struct bootloader_ctxt_t));

    return ctxt;
}

struct spi_ctxt_t comm_protocol_retrieve_spiinfo(struct comm_driver_t * cdriver) {
    _comm_send_generic_cmd(cdriver, COMMP_CMD_INFO_SPI);
    uint8_t out_buffer[COMMP_CMD_PACKET_SIZE + sizeof(struct spi_ctxt_t)];
    size_t len = (COMMP_PACKET_SIZE + sizeof(struct spi_ctxt_t));
    int err = comm_protocol_read_data(cdriver, out_buffer, &len);
    if (err < 0) {
        LOG_ERROR("Failed to read spiinfo");
    }

    struct spi_ctxt_t ctxt;
    memcpy(&ctxt, &out_buffer[COMMP_CMD_PACKET_SIZE], sizeof(struct spi_ctxt_t));

    return ctxt;
}

int comm_protocol_force_boot(struct comm_driver_t * cdriver) {
    int error = _comm_send_generic_cmd(cdriver, COMMP_CMD_BOOT);

    if (error < 0) {
        LOG_ERROR("Failed to send crc");
    }
    return error;
}

/* comm_protocol_read_binary - (flash_tool code)
 * Read a block of maximum COMMP_DATA_SIZE(512) bytes , the 4byte data-header is not returned
 * Parameters
 *  cdriver : comm_driver_t
 *  buffer : destination mem
 *  blocknr : the offset block inside the ROM
 *  len : is only relevant for the last chunk of data , otherwise = 512
 *  rom_nr : which rom approm0, approm1, spi0
 *  totalByteCount : needed for the RRQ request
 */
int comm_protocol_read_binary(struct comm_driver_t * cdriver,
                              uint8_t * buffer,
                              uint16_t blocknr,
                              size_t len,
                              uint8_t rom_nr,
                              size_t totalByteCount) {
    // read is per 4 header bytes + 512bytes data (struct comm_proto_data_t)
    if ( len > COMMP_DATA_SIZE ) {
        len = COMMP_DATA_SIZE;
        LOG_WARN("Read length %d > %d", len, COMMP_DATA_SIZE);
    }

    struct comm_ctxt_t transfer_ctxt = {
        .transfer_in_progress = false,
        .crc_received         = false,
        .last_blocknr         = blocknr,
        .parent               = cdriver,
        .rom_readsize         = totalByteCount,
        .part_nr              = rom_nr
    };

    size_t readsize = COMMP_PACKET_SIZE;
    uint8_t in_buffer[COMMP_ACK_BUFFER_SIZE] = { 0 };
    int error;

    if ( blocknr == 0 ) {
        // initiate read transfer
        error = _comm_send_rrq(cdriver, &transfer_ctxt);
        if (error < 0) {
            _reset_transfer_ctxt(&transfer_ctxt);
            return -1;
        }
    }

    // Read the header: ZERO - OPCODE - 2BYTE Blocknr
    // and clean not expected messages
    do {
        readsize = COMMP_ACK_BUFFER_SIZE;
        error = comm_protocol_read_data(cdriver, in_buffer, &readsize);
        if (error < 0) {
            return error;
        }
        // check if we need to flush data
        if (in_buffer[COMMP_OPCODE_ZERO_BYTE] != 0 || in_buffer[COMMP_OPCODE_BYTE] != COMMP_DATA) {
            LOG_DEBUG("Flush rxDATA -> rx[%x %x %x %x] %d bytes",
                      in_buffer[0], in_buffer[1], in_buffer[2], in_buffer[3], readsize);
        }
    } while (in_buffer[COMMP_OPCODE_ZERO_BYTE] != 0 || in_buffer[COMMP_OPCODE_BYTE] != COMMP_DATA);

    // return data will always start with 00 03(opcode COMMP_DATA) + 00 00(blocknr) = 4 bytes overhead
    if (in_buffer[COMMP_OPCODE_ZERO_BYTE] == 0 && in_buffer[COMMP_OPCODE_BYTE] == COMMP_DATA) {
        if ((blocknr % 50 == 0) || (blocknr > (totalByteCount / COMMP_DATA_SIZE - 5)))          // reduce output
            LOG_OK("RRQ->DATA answer -> rx[zero:%x opcode:%x blocknr:0x%x%x] %d",
                   in_buffer[0], in_buffer[1], in_buffer[2], in_buffer[3], readsize);
    }

    // Fetch the actual incoming DATA
    // LOG_DEBUG("readback - Waiting for %d DATA , blocknr %d", len, blocknr);
    size_t rx_size = 0;
    while (rx_size < len) {
        readsize = COMMP_PACKET_SIZE;
        error = comm_protocol_read_data(cdriver, buffer + rx_size, &readsize);
        if (error < 0) {
            LOG_ERROR("Failed to read binary data, error=%d", error);
            LOG_ERROR("Last rx data bytes was %d/%d", rx_size, len);
            return error;
        }
        rx_size += readsize;
        // LOG_DEBUG("Received Data block (read %d bytes) %d/%d", readsize, rx_size, transfer_ctxt.rom_readsize);
        if (readsize == 0)
            break;
    }
    // LOG_DEBUG("rx[%x %x %x %x ..] size %d", buffer[0], buffer[1], buffer[2], buffer[3], readsize);
    // LOG_OK("Received Data block %d/%d, last byte [%X]", rx_size,transfer_ctxt.rom_readsize,buffer[rx_size-1]);

    int err = _comm_send_ack(cdriver, &transfer_ctxt);
    if (err < 0) {
        LOG_ERROR("Failed to send ack on binary data");
        return err;
    }

    return (int)rx_size;
}

void comm_protocol_close() {
    for (int i = 0; cdrivers[i] != NULL; i++) {
        if (cdrivers[i]->enabled && cdrivers[i]->ops) {
            if (cdrivers[i]->ops->close) {
                cdrivers[i]->ops->close((void *)cdrivers[i]);
            }
        }
    }
    LOG_OK("Comm Protocol closed");
    return;
}
