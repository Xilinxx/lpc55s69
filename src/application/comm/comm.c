#define _COMM_C_

/*********************** (C) COPYRIGHT BARCO 2021 ******************************
* File Name           : comm.c
* Author              : Barco
* created             : May 2021
* Description         : gpmcu Communication handler
* History:
* 25/05/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/
// Includes

#include <string.h>
#include "comm.h"
#include "softversions.h"
#include "protocol.h"
#include "gowin_protocol.h"

#ifndef UNIT_TEST
  #include "fsl_gpio.h"
  #include <board.h>
  #include "flash_helper.h"
#endif

#include "logger.h"

// run code includes
#include "run/comm_run.h"
#include "memory_map.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/


/*******************************************************************************
 * Code
 ******************************************************************************/

// invalid/unknown command -> return command error!
void reply_invalid(const u8 nack_type) { // NACK
    COMM_DATA[UART1].MsgBuf[PROTOCOL_RX_OFFSET_CMD] = COMM_CMD_NACK;
    COMM_DATA[UART1].MsgBuf[PROTOCOL_RX_OFFSET_ADR] = nack_type;
    PROTO_TX_SendMsg(UART1, COMM_DATA[UART1].MsgBuf, COMM_NACK_TX_LENGTH);
}

// ------------------------------------------------------------------------------
// sharedram_log_read  - 16Kb size = 16.384 bytes starting at 0x20040000
// the shared ram struct starts with ssram_data_t struct (memory_map.h)
// the logger takes 8Kb starting at `0x20042000` , end `0x20044000`
// the LOG gets read in chunks of 256byte.
// resulting in 32offset 0..31, so offset is multiplied by 256=0x100
// ------------------------------------------------------------------------------
void sharedram_log_read(u8 * RxBuf,
                        u8 Offset,
                        u8 NrOfBytes) {
#ifndef UNIT_TEST // gets mocked for UNIT_TESTS
    LOG_DEBUG("sharedram_log_read Offset(%d) NrOfBytes(%d)", Offset, NrOfBytes);

    const u8 MAXPAGE = 32;
    u8 * iSource;
    u32 Address = __ssram_log_start__;

    if (Offset < MAXPAGE) {
        iSource = (u8 *)Address + Offset * 0x100; // set address pointer
        // copy ram data into destination pointer
        memcpy(RxBuf + PROTOCOL_RX_OFFSET_ADR, iSource, NrOfBytes);
        PROTO_TX_SendMsg(UART1, RxBuf, (u16)(NrOfBytes + PROTOCOL_RX_OFFSET_ADR));
    } else { // maxpage boundary exceeded, reply NACK-command with error code
        reply_invalid(COMM_NACK_OUTOFBOUNDARY);
        LOG_DEBUG("sharedram_log_read page out of boundary >= %d", MAXPAGE);
    }
#else
    LOG_ERROR("sharedram_log_read should be mocked");
#endif
}

// ------------------------------------------------------------------------------
// SharedRamStruct_Write - struct at 0x20040000 in ram
// the shared ram struct starts with ssram_data_t struct (memory_map.h)
// Way of sending requests to the bootloader code.
// Possible parameters:
//   NONE = 0,
//   UPDATE = 1 ,
//   BOOT_PARTITION_0 = 2,
//   BOOT_PARTITION_1 = 3
//  when switching the Application Partition we keep the flash in sync
// ------------------------------------------------------------------------------
bool sharedram_bootloader_request_write(u8 request) {
    LOG_DEBUG("comm::sharedram_bootloader_request_write [%d]", request);
    if (request > BOOT_PARTITION_1)
        return false;
#ifndef UNIT_TEST
    SharedRamStruct_pointer()->application_request = request;
#endif
    // Keep flash memory in sync and check if change is needed
    if ((Main.PartitionInfo.part == APP_PARTITION_0) &&
        (request == BOOT_PARTITION_1)) {
        Main.PartitionInfo.part = switch_boot_partition();
    } else if ((Main.PartitionInfo.part == APP_PARTITION_1) &&
               (request == BOOT_PARTITION_0)) {
        Main.PartitionInfo.part = switch_boot_partition();
    }
    return true;
}

u8 sharedram_bootloader_request_read() {
    LOG_DEBUG("comm::sharedram_bootloader_request_read");
#ifndef UNIT_TEST
    return SharedRamStruct_pointer()->application_request;
#else
    return 0xaa;
#endif
}

// ------------------------------------------------------------------------------
// void comm_handler (void)
// handle front-end communication
// Gowin Protocol actions also defined here until the speak COMM_protocol
// ------------------------------------------------------------------------------
void comm_handler(void) {
    const u8 ARRAY_READ_EXPECTED_RX_SIZE = 5;

#ifdef UNIT_TEST // board.h is not included
    const int DEVICE_ADDRESS = 0x10;
#endif

    if ((*COMM_DATA[UART1].MsgBuf) == DEVICE_ADDRESS) { // check address byte
        LOG_DEBUG("comm_handler - Address match");
        // primary command handler
        u8 Address = *(COMM_DATA[UART1].MsgBuf + PROTOCOL_RX_OFFSET_CMD);
        u8 Identifier = *(COMM_DATA[UART1].MsgBuf + PROTOCOL_RX_OFFSET_ADR);
        switch (Address) { // check command byte
            case CMD_READ_BYTE:
                switch (Identifier) {
                    case CMD_ID_EDID1 ... CMD_ID_EDID3: // Edid-readback from gowin
                        GOWIN_Protocol_edid_readback((u8)(Identifier - CMD_ID_EDID1));
                        break;
                    case CMD_ID_DPCD1 ... CMD_ID_DPCD3: // dpcd-readback from gowin
                        GOWIN_Protocol_dpcd_readback((u8)(Identifier - CMD_ID_DPCD1));
                        break;
                    default:
                        Run_CommHandler(COMM_DATA[UART1].MsgBuf, COMM_DATA[UART1].MsgLength);
                        break;
                }
                break;
            // --------------------------------------------------------------------------
            case CMD_WRITE_BYTE:
                switch (Identifier) {
                    case CMD_ID_EDID1 ... CMD_ID_EDID3: // Edid-readback from gowin
                        GOWIN_Protocol_edid_write((u8)(Identifier - CMD_ID_EDID1));
                        break;
                    case CMD_ID_DPCD1 ... CMD_ID_DPCD3: // dpcd-readback from gowin
                        GOWIN_Protocol_dpcd_write((u8)(Identifier - CMD_ID_DPCD1));
                        break;
                    default:
                        Run_CommHandler(COMM_DATA[UART1].MsgBuf, COMM_DATA[UART1].MsgLength);
                        break;
                }
                break;
            // --------------------------------------------------------------------------
            case CMD_READ_ARRAY:
                if (COMM_DATA[UART1].MsgLength == ARRAY_READ_EXPECTED_RX_SIZE) { // valid read command...
                    switch (Identifier) {
                        case CMD_ID_BOOTLOG:
                            sharedram_log_read(COMM_DATA[UART1].MsgBuf,
                                               (*(COMM_DATA[UART1].MsgBuf +
                                                  PROTOCOL_RX_OFFSET_OFFSET)),
                                               (*(COMM_DATA[UART1].MsgBuf +
                                                  PROTOCOL_RX_OFFSET_LENGTH))
                                               );
                            break;

                        default: // other identifiers -> execute RUN command handler
                            Run_CommHandler(COMM_DATA[UART1].MsgBuf, COMM_DATA[UART1].MsgLength);
                            break;
                    }
                } else {
                    LOG_DEBUG("comm_handler - CMD_READ_ARRAY - invalid reply (length=%d<>%d)"
                              , COMM_DATA[UART1].MsgLength, ARRAY_READ_EXPECTED_RX_SIZE);
                    reply_invalid(COMM_NACK_MSG_LENGTH);
                }
                break;
            // --------------------------------------------------------------------------
            // r/w byte, integer, array-commands handled below
            default:
                Run_CommHandler(COMM_DATA[UART1].MsgBuf, COMM_DATA[UART1].MsgLength);
                break;
        }
    } else {
        LOG_DEBUG("comm_handler - No Address match with %X", (*COMM_DATA[UART1].MsgBuf));
    }

}

// ------------------------------------------------------------------------------
// void comm_handler_gowin (void)
// used for passing cmd request from main towards the Gowin
// ------------------------------------------------------------------------------
void comm_handler_gowin(void) {
    if (Main.Diagnostics.PendingGowinCmd != NO_GOWIN_CMD) {
        GOWIN_TX_SendMsg(Main.Diagnostics.PendingGowinCmd);
        Main.Diagnostics.PendingGowinCmd = NO_GOWIN_CMD;
    } else {
        // If we get here we received an EDID/DPCD message
        LOG_DEBUG("comm_handler_gowin - msg received");
    }
}

