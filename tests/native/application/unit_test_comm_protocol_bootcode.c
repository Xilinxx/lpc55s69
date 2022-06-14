/*********************** (C) COPYRIGHT BARCO  *********************************
 * File Name           : unit_test_comm_protocol_bootcode.c  - native
 * Author              : DAVTH
 * created             : 07/08/2021
 * Description         : comm protocol test bootcode validation
 *                       for cmd processing we also use comm.c
 * History:
 * 07/08/2021 - initial
 *******************************************************************************/
#include "unit_test_comm_protocol_helper.h"

// ------------------------------------------------------------------------------
// Bootcode byte Read Test register at address 0x30
void bootcode_read_test(void ** states) {
    const u8 readBoot[] = { CMD_READ_BYTE, 0x30 };
    const u8 expectedReply[] = { CMD_READ_BYTE, 0xaa };

    u8 size;
    t_comm_protocol_return_value ret;

    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Bootcode Read Test COMM_Protocol should pass");
    // put messages in the Rx-buffer
    size = feed_RingBuffer(readBoot, sizeof(readBoot), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
    assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
    assert_int_equal(COMM_DATA[UART1].MsgLength, size);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf + 2, expectedReply, sizeof(expectedReply));
}

// ------------------------------------------------------------------------------
// Bootcode byte Write Test register at address 0x30
void bootcode_write_test(void ** states) {
    const u8 writeBoot[] = { CMD_WRITE_BYTE, 0x30, 0x01 };
    const u8 expectedReply[] = { CMD_WRITE_BYTE, 0x1 }; // ACK

    u8 size;
    t_comm_protocol_return_value ret;

    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Bootcode Write Test COMM_Protocol should pass");
    // put messages in the Rx-buffer
    size = feed_RingBuffer(writeBoot, sizeof(writeBoot), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
    assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
    assert_int_equal(COMM_DATA[UART1].MsgLength, size);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf + 2, expectedReply, sizeof(expectedReply));
}


// ------------------------------------------------------------------------------
// Bootcode byte Write Test register at address 0x30 with val > max
void bootcode_write_faulty_value_test(void ** states) {
    const u8 writeBoot[] = { CMD_WRITE_BYTE, 0x30, 0x05 };
    const u8 expectedReply[] = { CMD_WRITE_BYTE, 0x0 }; // NACK

    u8 size;
    t_comm_protocol_return_value ret;

    COMM_DATA[UART1].MsgCount = 0;
    LOG_INFO("Bootcode Write Enum out of range COMM_Protocol should pass");
    // put messages in the Rx-buffer
    size = feed_RingBuffer(writeBoot, sizeof(writeBoot), false);
    int cnt = COMM_DATA[UART1].GoodMsgCount;
    ret = COMM_Protocol(UART1);
    assert_return_code(ret, NO_ERROR);

    assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
    assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
    assert_int_equal(COMM_DATA[UART1].MsgLength, size);

    comm_handler();
    COMM_DATA[UART1].MsgCount = 0;
    assert_memory_equal(unitTest_SendBuf + 2, expectedReply, sizeof(expectedReply));
}


// ------------------------------------------------------------------------------
int main(void) {
    const struct CMUnitTest bootcode_tests[] = {
        cmocka_unit_test(bootcode_read_test),
        cmocka_unit_test(bootcode_write_test),
        cmocka_unit_test(bootcode_write_faulty_value_test),
    };

    return cmocka_run_group_tests(bootcode_tests, NULL, NULL);
}