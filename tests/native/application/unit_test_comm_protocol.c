/*********************** (C) COPYRIGHT BARCO  *********************************
* File Name           : unit_test_comm_protocol.c  - native
* Author              : DAVTH
* created             : 25/05/2021
* Description         : protocol test
*                       for cmd processing we also use comm.c
* History:
* 25/5/2021 - initial
*******************************************************************************/
#include "unit_test_comm_protocol_helper.h"


const u8 ByteWrite[]     = { CMD_WRITE_BYTE , BYTE_TEST_REG, 0xAA };
const u8 ByteWriteReply[]= { COMM_START_BYTE, ADR, CMD_WRITE_BYTE, 0x01, 0x22, COMM_STOP_BYTE };

const u8 ByteRead[]      = { CMD_READ_BYTE , BYTE_TEST_REG };
const u8 ByteReadReply[] = { COMM_START_BYTE, ADR, CMD_READ_BYTE , 0xAA, 0xCA, COMM_STOP_BYTE };

//------------------------------------------------------------------------------
void empty_buffers_test(void **states)
{
  LOG_INFO("Empty COMM_Protocol buffer should pass");
  t_comm_protocol_return_value ret = COMM_Protocol(UART1);

  assert_return_code(ret, NO_ERROR);
}

//------------------------------------------------------------------------------
void crc_error_test(void **states)
{
  LOG_INFO("Running COMM_Protocol BAD CRC Test should fail");
  u8 tmpCmd[sizeof(ByteWriteReply)];

  feed_RingBuffer(ByteWrite, sizeof(ByteWrite), false);
  // manipulate the buffer, change data value
  modify_RingBuffer(3,0xAB);
  t_comm_protocol_return_value ret = COMM_Protocol(UART1);

  assert_int_equal(ret, CRC_ERROR);
}

//------------------------------------------------------------------------------
// Byte R/W Test register at address 0x03
// We write value 0xAA in the testRegister and Read it back
void byte_rw_test(void **states)
{
  u8 size;
  t_comm_protocol_return_value ret;
  COMM_DATA[UART1].MsgCount = 0;
  LOG_INFO("Byte R/W TestReg COMM_Protocol should pass");
  // put 2 debug messages in the Rx-buffer
  size = feed_RingBuffer(ByteWrite, sizeof(ByteWrite), false);
  feed_RingBuffer(ByteRead, sizeof(ByteRead), false);
  int cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  assert_return_code(ret, NO_ERROR);

  assert_int_not_equal(cnt , COMM_DATA[UART1].GoodMsgCount);
  assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
  assert_int_equal(COMM_DATA[UART1].MsgLength, size);

  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  assert_memory_equal( unitTest_SendBuf, ByteWriteReply, sizeof(ByteWriteReply));

  assert_int_equal(COMM_DATA[UART1].MsgCount, 0);
  ret = COMM_Protocol(UART1);
  assert_return_code(ret, NO_ERROR);
  assert_int_not_equal(cnt+1 , COMM_DATA[UART1].GoodMsgCount);
  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  assert_memory_equal( unitTest_SendBuf, ByteReadReply, sizeof(ByteReadReply));
}

//------------------------------------------------------------------------------
// Byte R/W Test register at Read Only Register
// We write value 0xAA in the testRegister and Read it back
const u8 ByteWriteVer[]   = { CMD_WRITE_BYTE, 0x00, 0x00 };
const u8 ByteWriteVerReply[]= { COMM_START_BYTE, ADR, CMD_WRITE_BYTE, 0x00, 0x21, COMM_STOP_BYTE };
const u8 ByteReadVer[]    = { CMD_READ_BYTE , 0x00 };
const u8 ByteReadVerReply[]= { COMM_START_BYTE, ADR, CMD_READ_BYTE , 0x00, 0x20, COMM_STOP_BYTE };
void byte_w_readonly_test(void **states)
{
  t_comm_protocol_return_value ret;
  COMM_DATA[UART1].MsgCount = 0;
  LOG_INFO("Byte R/W readonly COMM_Protocol should pass");
  // put 2 debug messages in the Rx-buffer
  u8 size = feed_RingBuffer(ByteWriteVer, sizeof(ByteWriteVer), false);
  feed_RingBuffer(ByteReadVer, sizeof(ByteReadVer), false);
  int cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  assert_return_code(ret, NO_ERROR);

  assert_int_not_equal(cnt , COMM_DATA[UART1].GoodMsgCount);
  assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
  assert_int_equal(COMM_DATA[UART1].MsgLength, size);

  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  assert_memory_equal( unitTest_SendBuf, ByteWriteVerReply, sizeof(ByteWriteVerReply));

  assert_int_equal(COMM_DATA[UART1].MsgCount, 0);
  ret = COMM_Protocol(UART1);
  assert_return_code(ret, NO_ERROR);
  assert_int_not_equal(cnt+1 , COMM_DATA[UART1].GoodMsgCount);
  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  assert_memory_equal( unitTest_SendBuf, ByteReadVerReply, sizeof(ByteReadVerReply));
}

//------------------------------------------------------------------------------
// write a StopByte as value in the test register
void escape_sequence_msg_test(void **states)
{
const u8 ByteWriteEsc[] = { CMD_WRITE_BYTE , BYTE_TEST_REG, COMM_STOP_BYTE };
const u8 ByteWriteEscReply[] = { COMM_START_BYTE, ADR, CMD_WRITE_BYTE , 1, 0x22, COMM_STOP_BYTE };

  u8 size;
  t_comm_protocol_return_value ret;
  COMM_DATA[UART1].MsgCount = 0;
  LOG_INFO("Message with ESC char - should pass");
  size = feed_RingBuffer(ByteWriteEsc, sizeof(ByteWriteEsc), false);
  int cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  // The escaped character was translated to original
  assert_int_equal(ret, NEW_MSG);

  assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
  assert_int_equal(COMM_DATA[UART1].MsgCount, 1);
  // check total length = no START, no STOP, no ESC, no CRC
  assert_int_equal(COMM_DATA[UART1].MsgLength, size);
  //the 7th byte should hold the translated STOP_BYTE
  assert_int_equal(*(COMM_DATA[UART1].MsgBuf+3), COMM_STOP_BYTE);

  comm_handler();
  // The STOP_BYTE character-> escaped character + (c-0x80)
  COMM_DATA[UART1].MsgCount = 0;

  assert_memory_equal( unitTest_SendBuf, ByteWriteEscReply, sizeof(ByteWriteEscReply));
}

//------------------------------------------------------------------------------
// byteWrite too many data bytes , we expect a NACK with error-code
// this message always expects a size of 4 byte, sending 6 will return an error
void byte_write_data_size_nok_msg_test(void **states)
{
const u8 ByteWriteSizeNOK[] = { CMD_WRITE_BYTE , BYTE_TEST_REG, 1, 2, 3 };
const u8 ByteWriteSizeNOKReply[]  = { COMM_CMD_NACK, COMM_NACK_MSG_LENGTH };

  u8 size;
  t_comm_protocol_return_value ret;
  COMM_DATA[UART1].MsgCount = 0;
  LOG_INFO("Message with to many data bytes - should pass");
  size = feed_RingBuffer(ByteWriteSizeNOK, sizeof(ByteWriteSizeNOK), false);
  int cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  assert_int_equal(ret, NEW_MSG);

  assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
  assert_int_equal(COMM_DATA[UART1].MsgCount, 1);

  // The actual check of the written data-size will be checked here.
  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;

  //We only check the data part for COMM_NACK_MSG_LENGTH
  assert_memory_equal( unitTest_SendBuf+2, ByteWriteSizeNOKReply, sizeof(ByteWriteSizeNOKReply));
}

//------------------------------------------------------------------------------
// integerWrite/Read test register - 0xAA
void int_rw_testregister_msg_test(void **states)
{
const u8 write4ByteTestReg[] = { CMD_WRITE_4BYTE, 0xAA, 1, 2, 3, 4  };
const u8 read4ByteTestReg[]  = { CMD_READ_4BYTE,  0xAA };
const u8 read4ByteTestRegReply[]  = { CMD_READ_4BYTE, 1, 2, 3, 4 };

  //LOG_INFO("1 byte test register is %0.2X", Main.Debug.byteTestRegister);
  //LOG_INFO("4 byte test register is %0.8X", Main.Debug.intTestRegister);
  initialize_global_data_map();
  u8 size;
  t_comm_protocol_return_value ret;
  COMM_DATA[UART1].MsgCount = 0;
  u8 state = Main.Debug.byteTestRegister;
  LOG_INFO("Read/write 4 byte testregister - should pass");
  size = feed_RingBuffer(write4ByteTestReg, sizeof(write4ByteTestReg), false);
  int cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  assert_int_equal(ret, NEW_MSG);
  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  //check if the 1st byte of the struct is not overwritten
  assert_int_equal(Main.Debug.byteTestRegister, state);
  LOG_INFO("4 byte test register is %0.8X", Main.Debug.intTestRegister);
  assert_int_equal(Main.Debug.intTestRegister, 0x01020304);

  size = feed_RingBuffer(read4ByteTestReg, sizeof(read4ByteTestReg), false);
  ret = COMM_Protocol(UART1);
  assert_int_equal(ret, NEW_MSG);
  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  assert_memory_equal( unitTest_SendBuf+2, read4ByteTestRegReply, sizeof(read4ByteTestRegReply));
}

//------------------------------------------------------------------------------
// integer Write faulty length test register - 0xAA
void int_w_faulty_testregister_msg_test(void **states)
{
  const u8 writeFaulty4ByteTestReg[] = { CMD_WRITE_4BYTE, 0xAA, 0x01 };
  const u8 writeFaulty4ByteTestReg1[] = { CMD_WRITE_4BYTE, 0xAA, 1,2,3,4,5,6 };
  const u8 expectedReply[] = { COMM_START_BYTE, ADR, COMM_CMD_NACK, COMM_NACK_MSG_LENGTH, 0x11, COMM_STOP_BYTE };

  u8 size;
  t_comm_protocol_return_value ret;
  COMM_DATA[UART1].MsgCount = 0;
  LOG_INFO("Write 4 byte testregister with data mismatch - should pass");
  size = feed_RingBuffer(writeFaulty4ByteTestReg, sizeof(writeFaulty4ByteTestReg), false);
  int cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  assert_int_equal(ret, NEW_MSG);
  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  assert_memory_equal( unitTest_SendBuf, expectedReply, sizeof(expectedReply));

  size = feed_RingBuffer(writeFaulty4ByteTestReg1, sizeof(writeFaulty4ByteTestReg1), false);
  cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  assert_int_equal(ret, NEW_MSG);
  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  assert_memory_equal( unitTest_SendBuf, expectedReply, sizeof(expectedReply));
}

//------------------------------------------------------------------------------
// integerRead of CRC value of bootloader at 0x08.
const u8 read4Byte[] = { CMD_READ_4BYTE, 0x08  };
const u8 read4ByteReply[]  = { CMD_READ_4BYTE, 1, 2, 3, 4 };
void int_read_msg_test(void **states)
{
  initialize_global_data_map();
  u8 size;
  t_comm_protocol_return_value ret;
  COMM_DATA[UART1].MsgCount = 0;
  LOG_INFO("Read message 4 byte - should pass");
  size = feed_RingBuffer(read4Byte, sizeof(read4Byte), false);
  int cnt = COMM_DATA[UART1].GoodMsgCount;
  ret = COMM_Protocol(UART1);
  assert_int_equal(ret, NEW_MSG);

  assert_int_not_equal(cnt, COMM_DATA[UART1].GoodMsgCount);
  assert_int_equal(COMM_DATA[UART1].MsgCount, 1);

  comm_handler();
  COMM_DATA[UART1].MsgCount = 0;
  //Init_data_map() tampers the crc to be 0x01020304 for unit testing
  LOG_INFO("Expect reply of %0.8X",Main.PartitionInfo.crc);
  //We only check the data part for COMM_NACK_MSG_LENGTH
  assert_memory_equal( unitTest_SendBuf+2, read4ByteReply, sizeof(read4ByteReply));
}

//------------------------------------------------------------------------------
int main(void)
{
  const struct CMUnitTest tests_protocol[] = {
    cmocka_unit_test(empty_buffers_test),
    cmocka_unit_test(crc_error_test),
    cmocka_unit_test(byte_rw_test),
    cmocka_unit_test(byte_w_readonly_test),
    cmocka_unit_test(escape_sequence_msg_test),
    cmocka_unit_test(byte_write_data_size_nok_msg_test),
    cmocka_unit_test(int_rw_testregister_msg_test),
    cmocka_unit_test(int_w_faulty_testregister_msg_test),
    cmocka_unit_test(int_read_msg_test)
  };

  return cmocka_run_group_tests(tests_protocol, NULL, NULL);
}