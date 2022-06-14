/*********************** (C) COPYRIGHT BARCO  *********************************
 * File Name           : unit_test_comm_protocol_helper.h  - native
 * Author              : DAVTH
 * created             : 08/06/2021
 * Description         : protocol test helper
 *
 * History:
 * 8/6/2021 - initial
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#define UNIT_TESTING 1

#include "logger.h"

// application includes
#include "comm/protocol.h"
#include "comm/comm.h"
#include "data_map.h"


u8 MCU_RXBUF[MCU_RXBUF_SIZE]; // uart1 ring buffer - raw data -> holds any incoming byte!
u8 GOWIN_RXBUF[GOWIN_RXBUF_SIZE]; // uart2 ring buffer - raw data -> holds any incoming byte!

// fixed address 0x10
#define ADR           0x10

#define BYTE_TEST_REG 0x03

u8 feed_RingBuffer(const char * data, u16 length, bool print);
void modify_RingBuffer(u8 backward, u8 newval);

