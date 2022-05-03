#ifndef _COMM_PROTOCOL_MOCK_H_
#define _COMM_PROTOCOL_MOCK_H_
/*********************** (C) COPYRIGHT BARCO  *********************************
* File Name           : unit_test_comm_protocol_mock.h  - native
* Author              : DAVTH
* created             : 14/06/2021
* Description         : protocol mock

* History:
* 14/6/2021 - initial
*******************************************************************************/
#include "comm/protocol.h"
#include "comm/comm.h"

/* redefinitons/wrapping */

void __real_PROTO_TX_SendMsg (t_uart_channel uart_channel, u8 *data, u16 length);
void __wrap_PROTO_TX_SendMsg (t_uart_channel uart_channel, u8 *data, u16 length);

void __real_sharedram_log_read (u8 *RxBuf, u8 Offset, u8 NrOfBytes);
void __wrap_sharedram_log_read (u8 *RxBuf, u8 Offset, u8 NrOfBytes);

void __real_spi_queue_msg_param(u8 *data, u16 size);
void __wrap_spi_queue_msg_param(u8 *data, u16 size);

#endif