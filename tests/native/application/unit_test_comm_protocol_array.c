/*********************** (C) COPYRIGHT BARCO  *********************************
 * File Name           : unit_test_comm_protocol_array.c  - native
 * Author              : DAVTH
 * created             : 31/05/2021
 * Description         : array protocol test
 *                       for cmd processing we also use comm.c
 * History:
 * 25/5/2021 - initial
 *******************************************************************************/
#include "unit_test_comm_protocol_helper.h"
#include "unit_test_comm_protocol_mock.h"

const u8 NACKSizeErrorReply[] = \
{ COMM_START_BYTE, ADR, COMM_CMD_NACK, COMM_NACK_MSG_LENGTH, 0x11, COMM_STOP_BYTE };
// ------------------------------------------------------------------------------
// Array Read Test register at address 0x00 , version info
void array_read_test(void ** states) {
    const u8 readGitBoot[] = { CMD_READ_ARRAY, 0x00, 0x00, 20 };
    const u8 readGitApp[] = { CMD_READ_ARRAY, 0x01, 0x00, 10 };
    const u8 expectedReply[] = \
    { CMD_READ_ARRAY, 0x78, 0x78, 0x2E, 0x79, 0x79, 0x2E, 0x7A, 0x7A, 0x00 };

    initialize_global_data_map();

    strncpy((char *)Main.VersionInfo.gitapplication, "xx.yy.zz", 8);
    Main.VersionInfo.gitapplication[8] = '\0';
    u8 size;
    t_comm_protocol_return_value ret;
    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Array Read Test COMM_Protocol should pass");
    // put messages in the Rx-buffer
    size = feed_RingBuffer(readGitApp, sizeof(readGitApp), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
    assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
    assert_int_equal(COMM_DATA[UART1].MsgLength, size);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf + 2, expectedReply, sizeof(expectedReply));
    LOG_DEBUG("Application [%s]", Main.VersionInfo.gitapplication);
    LOG_DEBUG("Boot [%s]", Main.VersionInfo.gitbootloader);
}

// ------------------------------------------------------------------------------
// Array Read Test register at address 0x00 , version info
void array_read_beyond_test(void ** states) {
    const u8 GitSize = sizeof(Main.VersionInfo.gitbootloader);
    const u8 readGitBoot[] = { CMD_READ_ARRAY, CMD_ID_GITBOOTVER, 0x00, GitSize + 10 };
    // reply is CMD without data = NACK
    const u8 expectedReply[] = { COMM_START_BYTE, ADR, CMD_READ_ARRAY, 0x28, COMM_STOP_BYTE };

    initialize_global_data_map();

    strncpy((char *)Main.VersionInfo.gitapplication, "xx.yy.zz", 8);
    Main.VersionInfo.gitapplication[8] = '\0';
    u8 size;
    t_comm_protocol_return_value ret;
    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Array Read Test COMM_Protocol should pass");
    // put message in the Rx-buffer
    size = feed_RingBuffer(readGitBoot, sizeof(readGitBoot), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
    assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
    assert_int_equal(COMM_DATA[UART1].MsgLength, size);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, expectedReply, sizeof(expectedReply));
    LOG_DEBUG("Application [%s]", Main.VersionInfo.gitapplication);
    LOG_DEBUG("Boot [%s]", Main.VersionInfo.gitbootloader);
}

// ------------------------------------------------------------------------------
// Array Read bootlog
void array_read_log_test(void ** states) {
    const u8 SIZE = 0xff; // maximum value
    const u8 OFFSET = 0x10;
    const u8 readLog[] = { CMD_READ_ARRAY, CMD_ID_BOOTLOG, OFFSET, SIZE };
    // there will be 2 escape characters inside unitTest_SendBuf, 0x80 and 0xfe
    u8 expectedReply[300];

    for (int i = 0, escaped = 0; (i + escaped) < sizeof(expectedReply); i++) {
        u8 c = i + OFFSET;
        if ((c == COMM_ESCAPE) || (c == COMM_START_BYTE) || (c == COMM_STOP_BYTE)) {
            expectedReply[i + escaped] = COMM_ESCAPE;
            escaped++;
            expectedReply[i + escaped] = c - COMM_ESCAPE;
            // LOG_DEBUG("Escape replaced with 0x80 0x%x @ %x", expectedReply[i+escaped],i);
        } else {
            expectedReply[i + escaped] = c;
        }
    }

    u8 size;
    t_comm_protocol_return_value ret;
    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Array Read BOOTLOG COMM_Protocol should pass");

    size = feed_RingBuffer(readLog, sizeof(readLog), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler(); // sharedram_log_read - Mock NOK
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf + 3, expectedReply, SIZE + 2);
}

// ------------------------------------------------------------------------------
// Array Read bootlog beyond maxpage(0..31)
void array_read_beyond_maxpage_log_test(void ** states) {
    const u8 SIZE = 0xff; // maximum value
    const u8 OFFSET = 32;
    const u8 readLog[] = { CMD_READ_ARRAY, CMD_ID_BOOTLOG, OFFSET, SIZE };
    const u8 expectedReply[] = { COMM_START_BYTE, ADR, 0x00, COMM_NACK_OUTOFBOUNDARY };

    u8 size;
    t_comm_protocol_return_value ret;

    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Array Read beyond maxpage BOOTLOG COMM_Protocol should pass");

    size = feed_RingBuffer(readLog, sizeof(readLog), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler(); // sharedram_log_read - Mock NOK
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, expectedReply, sizeof(expectedReply));
}

// ------------------------------------------------------------------------------
// Array Read bootlog - size mismatch (2 bytes of size) = 6bytes instead of 5
void array_wrong_read_log_test(void ** states) {
    const u8 SIZE = 0xff; // maximum value
    const u8 OFFSET = 0x10;
    const u8 readLog[] = { CMD_READ_ARRAY, CMD_ID_BOOTLOG, OFFSET, SIZE, SIZE };

    u8 size;
    t_comm_protocol_return_value ret;

    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Array wrong Read BOOTLOG COMM_Protocol should pass");

    size = feed_RingBuffer(readLog, sizeof(readLog), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, NACKSizeErrorReply, sizeof(NACKSizeErrorReply));
}

// ------------------------------------------------------------------------------
// Array Read Edid1 - including escape chars
void array_read_edid1_test(void ** states) {
    const u8 LENGTH = 0xff; // maximum value
    const u8 tx[] = { CMD_READ_ARRAY, CMD_ID_EDID1, 0, LENGTH };

    u8 size;
    t_comm_protocol_return_value ret;

    LOG_INFO("Array Read edid1 COMM_Protocol should pass");
    initialize_global_data_map(); // edid1 init with 1..255
    // check intialisation in data_map
    assert_int_equal(Main.Edid.Edid1[1], 1);
    assert_int_equal(Main.Edid.Edid1[0x80], 0x80);
    assert_int_equal(Main.Edid.Edid1[255], 0xff);

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    // check the edid data in the tx-buf
    assert_int_equal(*(unitTest_SendBuf + 3), Main.Edid.Edid1[0]);
    assert_int_equal(*(unitTest_SendBuf + 4), Main.Edid.Edid1[1]);
    assert_int_equal(*(unitTest_SendBuf + 5), Main.Edid.Edid1[2]);
}


// ------------------------------------------------------------------------------
// Array Write Edid - excluding escaped chars
void array_write_readback_edid_test(void ** states) {
    const u8 LENGTH = 0x81;
    u8 tx[LENGTH + 4];
    const u8 OFFSET = 0x0;

    tx[0] = CMD_WRITE_ARRAY;
    tx[1] = CMD_ID_EDID2;
    tx[2] = OFFSET;
    tx[3] = LENGTH;
    for (int i = 0; i < LENGTH; i++) {
        u8 c = i;
        if ((c == COMM_ESCAPE) || (c == COMM_START_BYTE) || (c == COMM_STOP_BYTE)) {
            tx[4 + i] = 0xAA;
        } else {
            tx[4 + i] = c;
        }
    }

    u8 size;
    t_comm_protocol_return_value ret;
    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Array Write and Readback edid COMM_Protocol should pass");
    initialize_global_data_map(); // edid1 init was with 0..255
    assert_int_equal(Main.Edid.Edid1[0], 0);
    assert_int_equal(Main.Edid.Edid1[1], 1);
    assert_int_equal(Main.Edid.Edid1[0x81], 0x81);

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_int_equal(COMM_DATA[UART1].MsgLength, 5 + LENGTH);
    assert_memory_equal(Main.Edid.Edid2, tx + 4, LENGTH);
    const u8 rx[] = { 0xFE, ADR, CMD_WRITE_ARRAY, 0x01 };
    assert_memory_equal(unitTest_SendBuf, rx, sizeof(rx));

    // readback part
    const u8 readback[] = { CMD_READ_ARRAY, CMD_ID_EDID2, 0, 0xff };
    size = feed_RingBuffer(readback, sizeof(readback), false);
    cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    // check the edid data in the tx-buf
    assert_memory_equal(unitTest_SendBuf + 3, tx + 4, LENGTH);
}

// ------------------------------------------------------------------------------
// Write an unknown address/identifier 0xEE
void array_write_unknow_id_test(void ** states) {
    const u8 tx[] = { CMD_WRITE_ARRAY, 0xEE, 0, 1 };
    const u8 rx[] = { 0xFE, ADR, CMD_WRITE_ARRAY, 0x29, 0xFF };

    u8 size;
    t_comm_protocol_return_value ret;

    LOG_INFO("Array write unknown id COMM_Protocol should pass");

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, rx, sizeof(rx));
}

// ------------------------------------------------------------------------------
// Write a read only address/identifier 0xEE
void array_write_ro_id_test(void ** states) {
    const u8 tx[] = { CMD_WRITE_ARRAY, CMD_ID_GITBOOTVER, 0, 3, 'A', 'B', 'C' };
    const u8 rx[] = { 0xFE, ADR, CMD_WRITE_ARRAY, 0x29, 0xFF };

    u8 size;
    t_comm_protocol_return_value ret;

    LOG_INFO("Array write a read only id COMM_Protocol should pass");

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, rx, sizeof(rx));
}

// ------------------------------------------------------------------------------
// Array Write beyond boundary
void array_write_beyond_boundary_test(void ** states) {
    const u8 LENGTH = 0xff; // maximum value
    u8 tx[LENGTH + 4];
    const u8 OFFSET = 0x10;

    tx[0] = CMD_WRITE_ARRAY;
    tx[1] = CMD_ID_EDID2;
    tx[2] = OFFSET;
    tx[3] = LENGTH;
    for (int i = 0; i < LENGTH; i++) {
        u8 c = i + OFFSET;
        if ((c == COMM_ESCAPE) || (c == COMM_START_BYTE) || (c == COMM_STOP_BYTE)) {
            tx[4 + i] = 0xAA;
        } else {
            tx[4 + i] = c;
        }
    }

    u8 size;
    t_comm_protocol_return_value ret;
    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Array Write beyond boundary COMM_Protocol should pass");

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    // check if DATA[0]=0 , NACK
    const u8 rx[] = { 0xFE, ADR, CMD_WRITE_ARRAY, 0x0, 0x29, 0xff };
    assert_memory_equal(unitTest_SendBuf, rx, sizeof(rx));
}

// ------------------------------------------------------------------------------
// Write array without supplying the 3 DATA bytes
void array_write_missing_data_id_test(void ** states) {
    const u8 tx[] = { CMD_WRITE_ARRAY, CMD_ID_EDID2, 0 }; // no length

    u8 size;
    t_comm_protocol_return_value ret;

    LOG_INFO("Array write with missing DATA COMM_Protocol should pass");

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, NACKSizeErrorReply, sizeof(NACKSizeErrorReply));
}

// ------------------------------------------------------------------------------
// Read an unknown address/identifier 0xEE
void array_read_unknow_id_test(void ** states) {
    const u8 tx[] = { CMD_READ_ARRAY, 0xEE, 0, 10 };
    const u8 rx[] = { 0xFE, ADR, CMD_READ_ARRAY, 0x00, 0x28, 0xFF };

    u8 size;
    t_comm_protocol_return_value ret;

    LOG_INFO("Array read unknown id COMM_Protocol should pass");

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, rx, sizeof(rx));
}

// ------------------------------------------------------------------------------
// reading out of boundary replies the same cmd WITHOUT data
void array_read_outofboundary_test(void ** states) {
    const u8 tx[] = { CMD_READ_ARRAY, CMD_ID_GITBOOTVER, 0, 40 };
    const u8 rx[] = { 0xFE, ADR, CMD_READ_ARRAY, 0x28, 0xFF };

    u8 size;
    t_comm_protocol_return_value ret;

    LOG_INFO("Array read out of boundary COMM_Protocol should pass");

    size = feed_RingBuffer(tx, sizeof(tx), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf, rx, sizeof(rx));
}

// ------------------------------------------------------------------------------
int main(void) {
    const struct CMUnitTest tests_array[] = {
        cmocka_unit_test(array_read_test),
        cmocka_unit_test(array_read_beyond_test),
        cmocka_unit_test(array_read_beyond_maxpage_log_test),
        cmocka_unit_test(array_read_log_test),
        cmocka_unit_test(array_wrong_read_log_test),
        cmocka_unit_test(array_read_edid1_test),
        cmocka_unit_test(array_write_readback_edid_test),
        cmocka_unit_test(array_write_unknow_id_test),
        cmocka_unit_test(array_write_ro_id_test),
        cmocka_unit_test(array_write_beyond_boundary_test),
        cmocka_unit_test(array_write_missing_data_id_test),
        cmocka_unit_test(array_read_unknow_id_test),
        cmocka_unit_test(array_read_outofboundary_test),
    };

    return cmocka_run_group_tests(tests_array, NULL, NULL);;
}