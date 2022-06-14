/*********************** (C) COPYRIGHT BARCO  *********************************
 * File Name           : unit_test_comm_protocol_mock.c  - native
 * Author              : DAVTH
 * created             : 14/06/2021
 * Description         : protocol mock - redefinition/wrapping
 *
 * History:
 * 14/6/2021 - initial
 *******************************************************************************/
#include <string.h>
#include <stdio.h>

#include "unit_test_comm_protocol_mock.h"

#include "logger.h"

#include "data_map.h"
#include "comm/comm.h"


u8 unitTest_SendBuf[MSGBUF_SIZE];

// ------------------------------------------------------------------------------
void __wrap_PROTO_TX_SendMsg(t_uart_channel uart_channel, u8 * data, u16 length) {
    // we log in ASCII here! each character takes 4 bytes!
    u8 c, checksum;
    u16 j;
    u8 logbuffer[80];
    u8 * logCur = logbuffer;
    u8 * logEnd = logbuffer + sizeof(logbuffer);

    u8 * cur = unitTest_SendBuf;
    u8 * end = unitTest_SendBuf + sizeof(unitTest_SendBuf);

    memset(&unitTest_SendBuf[0], 0, sizeof(unitTest_SendBuf));

    logCur += snprintf(logbuffer, logEnd - logCur, "{ %02X, ", COMM_START_BYTE);
    *cur++ = COMM_START_BYTE;
    checksum = 0;
    for (j = 0; j <= length; j++) {
        if ((logEnd - logCur) < 10) { // time to abort, full buffer
            // logCur +=snprintf(logCur, logEnd-logCur,"%s", " FULL ");
            // break;
            LOG_DEBUG("PROTO_TX_SendMsg %s", logbuffer);
            logCur = logbuffer;
            logEnd = logbuffer + sizeof(logbuffer);
        }
        if (j < length) {
            c = data[j];
            checksum += c;
        } else {
            c = checksum;
        }

        if ((c == COMM_ESCAPE) || (c == COMM_START_BYTE) || (c == COMM_STOP_BYTE)) {
            LOG_DEBUG("PROTO_TX_SendMsg ESCAPE");
            logCur += snprintf(logCur, logEnd - logCur, "%02X, ", COMM_ESCAPE);
            *cur++ = COMM_ESCAPE;
            c -= COMM_ESCAPE; // calculate new char
        }
        logCur += snprintf(logCur, logEnd - logCur, "%02X, ", c);
        *cur++ = c;
    }
    logCur += snprintf(logCur, logEnd - logCur, "%02X }", COMM_STOP_BYTE);
    *cur++ = COMM_STOP_BYTE;
    LOG_DEBUG("PROTO_TX_SendMsg %s", logbuffer);
}

// ------------------------------------------------------------------------------
void __wrap_spi_queue_msg_param(u8 * data, u16 size) {
    LOG_DEBUG("mocked: spi");
}

// ------------------------------------------------------------------------------
// wrapped sharedram_log_read is called from comm/comm.c
void __wrap_sharedram_log_read(u8 * RxBuf, u8 Offset, u8 NrOfBytes) {
    LOG_DEBUG("mocked: sharedram_log_read Offset(%d) NrOfBytes(%d)", Offset, NrOfBytes);
    const u8 MAXPAGE = 32;

    u8 mem[512];

    if (Offset >= MAXPAGE) { // nack on exceed boundary
        reply_invalid(COMM_NACK_OUTOFBOUNDARY);
        return;
    }

    for (int i = 0; i < sizeof(mem); i++) {
        mem[i] = i;
    }
    memcpy(RxBuf + PROTOCOL_RX_OFFSET_ADR, (u8 *)mem + Offset, NrOfBytes);
    PROTO_TX_SendMsg(UART1, RxBuf, (u16)(NrOfBytes + PROTOCOL_RX_OFFSET_ADR));
}


app_partition_t __wrap_switch_boot_partition() {
    LOG_DEBUG("mocked: switch_boot_partition");
    return APP_PARTITION_0;
}

// ------------------------------------------------------------------------------
// not in use
void __wrap_wdog_refresh() {
    LOG_DEBUG("mocked: wdog_refresh");
}
