/**
 * @file comm_parser.h
 * @brief  COMPP Parser
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-27
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "comm_protocol.h"

#define COMMP_OPCODE_BYTE 1

struct comm_ctxt_t; // !< Forward declaration of comm_ctxt_t struct

/**
 * @brief Create a new ack packet
 *
 * @param transfer_ctxt The transfer_ctxt containing the global transfer state
 * @param buffer Buffer which will contain the ack packet
 */
void comm_protocol_create_ack(struct comm_ctxt_t * transfer_ctxt, uint8_t * buffer);


/**
 * @brief Create a new error packet
 *
 * @param errorcode The error code
 * @param errstr The errorstr incapsulated in the packet
 * @param buffer The buffer which will contain the new error packet
 */
void comm_protocol_create_error(uint16_t errorcode, char * errstr, uint8_t * buffer);

/**
 * @brief  Parse a given COMMP packet
 *
 * @param transfer_ctxt A context containing the global state of the current transfers in progress
 * @param buffer buffer containing the data
 * @param len Length of the data buffer
 *
 * @returns Ehhh,...
 */
int comm_protocol_parse_packet(struct comm_ctxt_t * transfer_ctxt, uint8_t * buffer, size_t len);

/**
 * @brief  Translate the comm_proto_cmd_t-enum to string
 *
 * @param comm_proto_cmd_t cmd the enum
 *
 * @returns the name of the enum
 */
const char * comm_protocol_cmd_to_string(comm_proto_cmd_t cmd);
