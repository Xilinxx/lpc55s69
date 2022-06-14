/*********************** (C) COPYRIGHT BARCO  *********************************
 * File Name           : unit_test_gowin_protocol_byte.c  - native
 * Author              : DAVTH
 * created             : 15/11/2021
 * Description         : GOWIN protocol test for 1byte and 2byte reply
 * History:
 * 15/11/2021 - initial
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "logger.h"

#include "comm/gowin_protocol.h"
#include "data_map.h"

u8 MCU_RXBUF[0]; // uart1 ring buffer - not in use for gowin
u8 GOWIN_RXBUF[GOWIN_RXBUF_SIZE]; // UART2 ring buffer - raw data -> holds any incoming byte!

// initialize raw uart data structures , Note: MCU_RXBUF is never used.
volatile t_uart_data_raw UART_DATA[2] = { { (u8 *)&MCU_RXBUF,   MCU_RXBUF_SIZE,   (u8 *)&MCU_RXBUF,   (u8 *)&MCU_RXBUF   },
                                          { (u8 *)&GOWIN_RXBUF, GOWIN_RXBUF_SIZE, (u8 *)&GOWIN_RXBUF, (u8 *)&GOWIN_RXBUF } };


void feed_RingBuffer(char * data, u16 length) {
    // LOG_INFO("feed_RingBuffer - data length is %d with address %X[0]=0x%02x",length, data, *data);
    for (int i = 0; i < length; i++) {
        // copy from main IRQ routine
        *((u8 *)UART_DATA[UART2].RxBufWr) = *data;
        // LOG_RAW("%02X ",*data);
        UART_DATA[UART2].RxBufWr++;
        if (UART_DATA[UART2].RxBufWr >= (UART_DATA[UART2].RxBuf +
                                         UART_DATA[UART2].RxBufSize)) {
            LOG_DEBUG("feed_RingBuffer - reset write pointer at %d data[0x%02x]", i, *data);
            UART_DATA[UART2].RxBufWr = UART_DATA[UART2].RxBuf; // reset write pointer
        }
        data++;
    }
}

// ------------------------------------------------------------------------------
// Check buffer overrun
// expect to drop FPGA_MSGBUF_SIZE data bytes
void msg_overrun_should_pass_tests(void ** state) {
    int8_t ret;

    // initialize_global_data_map();
    // GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_INFO("Testing buffer overrun (should pass)");

    // create buffer overrun
    for (int i = 0; i < FPGA_MSGBUF_SIZE; i++) {
        feed_RingBuffer("@", 1);
    }
    feed_RingBuffer("A", 1); // will loose all previous chars
    ret = GOWIN_Protocol(UART2); // will process only 1 byte because of overrun
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_OK("overrun test passed (should pass)");
}

// ------------------------------------------------------------------------------
// Ethernet-status read-back from gowin
void msg_1byte_should_pass_tests(void ** state) {
    int8_t ret;

    // initialize_global_data_map();
    // GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_INFO("Testing 1byte reply message (should pass)");
    GOWIN_TX_SendMsg(READ_ETH_STATUS); // returns 1 byte
    feed_RingBuffer("$@", 2); // '@' = 0x40
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    assert_int_equal(1, GOWIN_DATA.MsgCount); // we received 1 byte ('$' excluded)
    assert_int_equal(1, GOWIN_DATA.MsgLength);

    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_OK("1byte reply test passed (should pass)");
}

// ------------------------------------------------------------------------------
// Ethernet-status read-back from gowin
void msg_2byte_should_pass_tests(void ** state) {
    int8_t ret;

    // initialize_global_data_map();
    // GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_INFO("Testing 2byte reply message (should pass)");
    GOWIN_TX_SendMsg(READ_BL_CURRENT); // returns 2 byte
    feed_RingBuffer("$@A", 3); // '@' = 0x40 , 'A' = 0x41
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    assert_int_equal(1, GOWIN_DATA.MsgCount); // we receive 2 byte ('$' excluded)
    assert_int_equal(2, GOWIN_DATA.MsgLength);

    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    assert_int_equal(0x40, Main.DeviceState.EthernetStatusBits);

    LOG_OK("2byte reply test passed (should pass)");
}

// ------------------------------------------------------------------------------
// Check 2byte overrun by sending 6 chars.
// Expected: Only 3 should get processed, rest dropped
void msg_2byte_overrun_should_pass_tests(void ** state) {
    int8_t ret;

    // initialize_global_data_map();
    // GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_INFO("Testing 2byte overrun message (should pass)");
    GOWIN_TX_SendMsg(READ_BL_CURRENT); // returns 2 byte
    feed_RingBuffer("$@ABCD", 6); // '@' = 0x40 , 'A' = 0x41
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    ret = GOWIN_Protocol(UART2); // will process "$@A"
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    assert_int_equal(1, GOWIN_DATA.MsgCount); // we receive 2 byte ('$' excluded)
    assert_int_equal(2, GOWIN_DATA.MsgLength);

    ret = GOWIN_Protocol(UART2); // nothing gets processed until MsgCount = 0
    assert_int_equal(ret, GOWIN_RETURN_MAX_MSG);

    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2); // process the following "BCD"
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    assert_int_equal(0x40 + (0x41 << 8), Main.DeviceState.PanelCurrent);

    LOG_OK("2byte overrun reply test passed (should pass)");
}

// ------------------------------------------------------------------------------
int main(void) {
    const struct CMUnitTest gowin_protocol_tests[] = {
        cmocka_unit_test(msg_overrun_should_pass_tests),
        cmocka_unit_test(msg_1byte_should_pass_tests),
        cmocka_unit_test(msg_1byte_should_pass_tests),
        cmocka_unit_test(msg_2byte_should_pass_tests),
        cmocka_unit_test(msg_2byte_should_pass_tests),
        cmocka_unit_test(msg_2byte_overrun_should_pass_tests)
    };

    return cmocka_run_group_tests(gowin_protocol_tests, NULL, NULL);
}
