/*********************** (C) COPYRIGHT BARCO  *********************************
 * File Name           : unit_test_gowin_protocol.c  - native
 * Author              : DAVTH
 * created             : 10/05/2021
 * Description         : GOWIN protocol test
 * History:
 * 10/5/2021 - initial
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


// random edid data
static uint8_t edid_data[256] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40,
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
    0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80,
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90,
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0,
    0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0,
    0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0,
    0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
    0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0,
    0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xF3
};

void randomize_edid_data() {
    for (int i = 0; i < sizeof(edid_data); i++) {
        edid_data[i] = rand() % 0xff;
    }
    LOG_DEBUG("randomized edid data");
}

void ordered_edid_data() {
    for (int i = 0; i < sizeof(edid_data); i++) {
        edid_data[i] = i;
    }
}

void feed_RingBuffer(char * data, u16 length) {
    // LOG_INFO("feed_RingBuffer - data length is %d with address %X[0]=0x%x",length, data, *data);
    for (int i = 0; i < length; i++) {
        // copy from main IRQ routine
        *((u8 *)UART_DATA[UART2].RxBufWr) = *data;
        UART_DATA[UART2].RxBufWr++;
        if (UART_DATA[UART2].RxBufWr >= (UART_DATA[UART2].RxBuf + UART_DATA[UART2].RxBufSize)) {
            LOG_DEBUG("feed_RingBuffer - reset write pointer at %d data[0x%02x]", i, *data);
            UART_DATA[UART2].RxBufWr = UART_DATA[UART2].RxBuf; // reset write pointer
        }
        data++;
    }
}

// ------------------------------------------------------------------------------
void empty_buffers_should_pass_tests(void ** states) {
    LOG_INFO("Empty buffer should pass");
    int8_t ret;
    for (unsigned i = 0; i < 10; i++) {
        ret = GOWIN_Protocol(UART2);
        assert_return_code(ret, GOWIN_RETURN_NONE);
    }
}

// ------------------------------------------------------------------------------
// * we receive a wake-up request from the gowin
// * we receive a 2nd message without processing the 1st.
void msg_should_pass_tests(void ** state) {
    int8_t ret;

    GOWIN_DATA.MsgCount = 0;
    LOG_INFO("Testing gowin message (should pass)");

    feed_RingBuffer("$%#", 3); // Wake up request from FPGA to GPMCU
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);


    feed_RingBuffer("$r#", 3);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_MAX_MSG);

    GOWIN_DATA.MsgCount = 0; // reset msg count, allow next

    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    GOWIN_DATA.MsgCount = 0; // reset msg count, allow next

    // Partial message
    feed_RingBuffer("#r$", 3); // '#p' should get discarded, $=start
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    feed_RingBuffer("p#", 2); // finish the message
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);

    LOG_OK("Gowin message tests passed (should pass)");
}

// ------------------------------------------------------------------------------
// process ack/nack from gowin after sending a command
void msg_acknack_should_pass_tests(void ** state) {
    int8_t ret;

    initialize_global_data_map();
    GOWIN_DATA.MsgCount = 0;
    LOG_INFO("Testing ACK/NACK gowin/fpga message (should pass)");

    t_gowin_command cmd;
    cmd = ACK;
    *((u8 *)UART_DATA[UART2].RxBufWr) = cmd; // ACK
    UART_DATA[UART2].RxBufWr++;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    cmd = NACK;
    *((u8 *)UART_DATA[UART2].RxBufWr) = cmd; // NACK
    UART_DATA[UART2].RxBufWr++;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_ERROR);

    LOG_OK("ACK/NACK test passed (should pass)");
}

// ------------------------------------------------------------------------------
// edid read-back from gowin
void msg_edid_should_pass_tests(void ** state) {
    int8_t ret;

    initialize_global_data_map();
    GOWIN_DATA.MsgCount = 0;
    assert_int_equal(0, GOWIN_DATA.MsgCount);
    LOG_INFO("Testing edid gowin/fpga message (should pass)");
    GOWIN_TX_SendMsg(READ_EDID);
    feed_RingBuffer("$", 1);
    feed_RingBuffer(edid_data, sizeof(edid_data));
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);

    // check if next message can't be processed
    feed_RingBuffer("$r#", 3);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_MAX_MSG);
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);


    LOG_OK("edid test passed (should pass)");
}

// ------------------------------------------------------------------------------
// intentionally fail to send the required 256 bytes of edid data, check timeout
// verify the GOWIN_DATA.RxBusy flag
void msg_edid_timeout_should_pass_tests(void ** state) {
    int8_t ret;

    GOWIN_DATA.MsgCount = 0;
    GOWIN_Queue_Init();
    LOG_INFO("Testing edid timeout gowin/fpga message (should pass)");
    GOWIN_TX_SendMsg(READ_EDID);
    feed_RingBuffer("$", 1);
    feed_RingBuffer(edid_data, 5);
    assert_int_equal(GOWIN_DATA.RxBusy, false);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(GOWIN_DATA.RxBusy, true);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    feed_RingBuffer(edid_data, 5);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    int cnt = GOWIN_DATA.BadMsgCount; // check bad message counter

    for (int i = 0; i < GOWIN_REPLY_TIMEOUT + 1; i++) { // trigger timeout
        ret = GOWIN_Protocol(UART2);
        if (ret == GOWIN_RETURN_TIMEOUT) {
            // LOG_INFO("timeout iterations needed %d",i);
            assert_in_range(i, 2, GOWIN_REPLY_TIMEOUT + 1);
            break;
        }
        assert_int_equal(GOWIN_DATA.RxBusy, true);
    }
    assert_int_not_equal(GOWIN_DATA.BadMsgCount, cnt);
    assert_int_equal(ret, GOWIN_RETURN_TIMEOUT);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(GOWIN_DATA.RxBusy, false);

    // check if next message can be processed
    feed_RingBuffer("$r#", 3);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);

    LOG_OK("edid timeout test passed (should pass)");
}

// ------------------------------------------------------------------------------
// intentionally fail to send STOP byte
void msg_stopbyte_timeout_should_pass_tests(void ** state) {
    int8_t ret;

    initialize_global_data_map();
    GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    LOG_INFO("Testing STOP byte timeout gowin/fpga message (should pass)");
    feed_RingBuffer("$", 1);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_STOP);

    int cnt = GOWIN_DATA.BadMsgCount; // check bad message counter

    for (int i = 0; i < GOWIN_REPLY_TIMEOUT + 1; i++) { // trigger timeout
        ret = GOWIN_Protocol(UART2);
        if (ret == GOWIN_RETURN_TIMEOUT) {
            LOG_INFO("timeout iterations needed %d", i);
            assert_in_range(i, 2, GOWIN_REPLY_TIMEOUT + 1);
            break;
        }
    }
    assert_int_not_equal(GOWIN_DATA.BadMsgCount, cnt);
    assert_int_equal(ret, GOWIN_RETURN_TIMEOUT);

    // check if next message can be processed
    feed_RingBuffer("$r#", 3);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);

    LOG_OK("stopbyte timeout test passed (should pass)");
}

// ------------------------------------------------------------------------------
// edid read-back from gowin, verify data by filling up EDID0 and EDID2
// check if the data_map with EDID data gets filled in
void msg_edid_verifydata_should_pass_tests(void ** state) {
    int8_t ret;
    uint8_t _edid_data[256]; // use local copy, had issues with changing data

    ordered_edid_data();
    memcpy(_edid_data, edid_data, sizeof(edid_data) );
    assert_memory_equal(_edid_data, edid_data, sizeof(edid_data));

    initialize_global_data_map();
    GOWIN_Queue_Init();
    assert_int_equal(GOWIN_Queue_size(), 0);
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_INFO("Testing edid gowin/fpga edid data (should pass)");

    // edid1 is filled with ordered data for unit-tests
    assert_memory_equal(Main.Edid.Edid1, _edid_data, sizeof(_edid_data));
    assert_memory_not_equal(Main.Edid.Edid2, _edid_data, sizeof(_edid_data));
    assert_memory_not_equal(Main.Edid.Edid3, _edid_data, sizeof(_edid_data));

    const int READBACK_INDEX = 1;
    assert_int_equal(GOWIN_Queue_size(), 0);
    GOWIN_Protocol_edid_readback(READBACK_INDEX); // Fill up data_map - edid index 0
    assert_int_equal(GOWIN_Queue_size(), 1);
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2); // Process the queued command
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    feed_RingBuffer("$", 1);
    feed_RingBuffer(_edid_data, sizeof(_edid_data));
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2); // Process the reply from Gowin edid_read
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    assert_int_equal(READBACK_INDEX, GOWIN_EdidIndex_get());
    ret = GOWIN_Protocol(UART2); // Process the reply from Gowin edid_read
    assert_int_equal(GOWIN_Queue_size(), 0);

    // Note: EDID1 is already filled in by default
    assert_memory_equal(Main.Edid.Edid1, _edid_data, sizeof(_edid_data));
    assert_memory_equal(Main.Edid.Edid2, _edid_data, sizeof(_edid_data));
    assert_memory_not_equal(Main.Edid.Edid3, _edid_data, sizeof(_edid_data));

    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    randomize_edid_data();
    memcpy(_edid_data, edid_data, sizeof(edid_data) );
    GOWIN_Protocol_edid_readback(READBACK_INDEX + 1); // Fill up data_map - edid index 2(last of 3)
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    feed_RingBuffer("$", 1);
    feed_RingBuffer(_edid_data, sizeof(_edid_data));
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    assert_int_equal(READBACK_INDEX + 1, GOWIN_EdidIndex_get());
    assert_memory_not_equal(Main.Edid.Edid1, _edid_data, sizeof(_edid_data));
    assert_memory_not_equal(Main.Edid.Edid2, _edid_data, sizeof(_edid_data));
    assert_memory_equal(Main.Edid.Edid3, _edid_data, sizeof(_edid_data));
    assert_int_equal(GOWIN_Queue_size(), 0);
    // round up
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_OK("edid-data test passed (should pass)");
}

// ------------------------------------------------------------------------------
// dpcd read-back from gowin, verify data by filling up EDID2 and EDID3
// check if the data_map with EDID data gets filled in
void msg_dcpd_verifydata_should_pass_tests(void ** state) {
    int8_t ret;
    uint8_t _edid_data[256]; // use local copy, had issues with changing data

    initialize_global_data_map();
    GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    LOG_INFO("Testing dpcd gowin/fpga edid data (should pass)");
    randomize_edid_data();
    memcpy(_edid_data, edid_data, sizeof(edid_data) );
    GOWIN_Protocol_dpcd_readback(2); // Fill up dcpd data @ index 2 = Dpcd3
    ret = GOWIN_Protocol(UART2);
    feed_RingBuffer("$", 1);
    feed_RingBuffer(_edid_data, sizeof(_edid_data));
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    assert_int_equal(2, GOWIN_DpcdIndex_get());

    assert_memory_not_equal(Main.Dpcd.Dpcd1, _edid_data, sizeof(edid_data));
    assert_memory_not_equal(Main.Dpcd.Dpcd2, _edid_data, sizeof(edid_data));
    assert_memory_equal(Main.Dpcd.Dpcd3, _edid_data, sizeof(edid_data));

    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    randomize_edid_data();
    memcpy(_edid_data, edid_data, sizeof(edid_data) );
    GOWIN_Protocol_dpcd_readback(1); // Fill up data_map - edid index 3
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(1, GOWIN_DpcdIndex_get()),
    feed_RingBuffer("$", 1);
    feed_RingBuffer(edid_data, sizeof(edid_data));
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);
    assert_memory_not_equal(Main.Dpcd.Dpcd1, _edid_data, sizeof(edid_data));
    assert_memory_equal(Main.Dpcd.Dpcd2, _edid_data, sizeof(edid_data));
    assert_memory_not_equal(Main.Dpcd.Dpcd3, _edid_data, sizeof(edid_data));

    // round up
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_OK("dpcd-data test passed (should pass)");
}

// ------------------------------------------------------------------------------
// edid write to gowin, verify ACK processing
// meanwhile verify drop of unexpected characters during waiting for ACK
void msg_edid_write_should_pass_tests(void ** state) {
    int8_t ret;

    initialize_global_data_map();
    GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_INFO("Testing write edid to gowin/fpga (should pass)");
    const int EDID_ID = 0;
    GOWIN_Protocol_edid_write(EDID_ID);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    char str[6];
    strcpy(str, "dummy");
    // LOG_DEBUG("%X[0]=%c", str, str[0]);
    // feed some dummy chars to trigger the protocol return code
    feed_RingBuffer(str, 5);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_WAIT_EDID_ACK);
    assert_int_equal(EDID_ID, GOWIN_EdidIndex_get()); // we just wrote to index 1

    // now feed the expected ACK
    str[0] = (char)GOWIN_ACK;
    // LOG_DEBUG("%X[0]=%x", str, str[0]);
    feed_RingBuffer(str, 1);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    assert_int_equal(GOWIN_Queue_size(), 0);

    LOG_OK("edid-write test passed (should pass)");
}

// ------------------------------------------------------------------------------
// dpcd write to gowin, verify ACK processing
// meanwhile verify drop of unexpected characters during waiting for ACK
void msg_dpcd_write_should_pass_tests(void ** state) {
    int8_t ret;

    initialize_global_data_map();
    GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);

    LOG_INFO("Testing write dpcd to gowin/fpga (should pass)");
    const int DPCD_ID = 0;
    GOWIN_Protocol_dpcd_write(DPCD_ID);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    assert_int_equal(GOWIN_Queue_size(), 1);

    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_WAIT_EDID_ACK);
    assert_int_equal(DPCD_ID, GOWIN_DpcdIndex_get());

    // now feed the expected ACK
    char str[1];
    str[0] = (char)GOWIN_ACK;
    // LOG_DEBUG("%X[0]=%x", str, str[0]);
    feed_RingBuffer(str, 1);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    assert_int_equal(GOWIN_Queue_size(), 0);

    LOG_OK("dpcd-write test passed (should pass)");
}

// ------------------------------------------------------------------------------
// edid write to gowin + verify with readback
void msg_edid_write_verify_should_pass_tests(void ** state) {
    int8_t ret;

    initialize_global_data_map();
    GOWIN_Queue_Init();
    GOWIN_DATA.MsgCount = 0;
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    LOG_INFO("Testing write/verify edid to gowin/fpga (should pass)");

    const int EDID_ID = 0;
    GOWIN_Protocol_edid_write(EDID_ID);
    ret = GOWIN_Protocol(UART2); // transmit
    assert_int_equal(GOWIN_Queue_size(), 1);
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_WAIT_EDID_ACK);
    assert_int_equal(GOWIN_DATA.MsgCount, 0);

    // Feed ACK for edid_write
    char str[1];
    str[0] = (char)GOWIN_ACK;
    // LOG_DEBUG("%X[0]=%x", str, str[0]);
    feed_RingBuffer(str, 1);

    ret = GOWIN_Protocol(UART2);
    LOG_INFO("ret=%d state=%d", ret, GOWIN_DATA.State);
    assert_int_equal(ret, GOWIN_RETURN_NONE);
    assert_int_equal(GOWIN_Queue_size(), 0);

    GOWIN_Protocol_edid_readback(2);
    ret = GOWIN_Protocol(UART2);
    feed_RingBuffer("$", 1);
    feed_RingBuffer(Main.Edid.Edid1, sizeof(edid_data));
    ret = GOWIN_Protocol(UART2);
    assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);

    assert_memory_equal(Main.Edid.Edid1, Main.Edid.Edid3, sizeof(Main.Edid.Edid1));
    ret = GOWIN_Compare_EdidDpcd(Main.Edid.Edid1, Main.Edid.Edid3, "Edid1==Edid3");
    assert_int_equal(ret, 0);

    LOG_OK("edid-write/verify test passed (should pass)");
}

// ------------------------------------------------------------------------------
int main(void) {
    const struct CMUnitTest gowin_protocol_tests[] = {
        cmocka_unit_test(empty_buffers_should_pass_tests),
        cmocka_unit_test(msg_should_pass_tests),
        cmocka_unit_test(msg_acknack_should_pass_tests),
        cmocka_unit_test(msg_edid_should_pass_tests),
        cmocka_unit_test(msg_edid_timeout_should_pass_tests),
        cmocka_unit_test(msg_stopbyte_timeout_should_pass_tests),
        cmocka_unit_test(msg_edid_verifydata_should_pass_tests),
        cmocka_unit_test(msg_dcpd_verifydata_should_pass_tests),
        cmocka_unit_test(msg_edid_write_should_pass_tests),
        cmocka_unit_test(msg_dpcd_write_should_pass_tests),
        cmocka_unit_test(msg_edid_write_verify_should_pass_tests),
    };

    return cmocka_run_group_tests(gowin_protocol_tests, NULL, NULL);
}
