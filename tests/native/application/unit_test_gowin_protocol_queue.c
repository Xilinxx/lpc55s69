/*********************** (C) COPYRIGHT BARCO  *********************************
* File Name           : unit_test_gowin_protocol_queue.c  - native
* Author              : DAVTH
* created             : 10/11/2021
* Description         : GOWIN protocol queue test
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
volatile t_uart_data_raw UART_DATA[2] = { { (u8*)&MCU_RXBUF, MCU_RXBUF_SIZE, (u8*)&MCU_RXBUF, (u8*)&MCU_RXBUF },
                                          { (u8*)&GOWIN_RXBUF, GOWIN_RXBUF_SIZE, (u8*)&GOWIN_RXBUF, (u8*)&GOWIN_RXBUF }
};


void feed_RingBuffer(char * data, u16 length)
{
  //LOG_INFO("feed_RingBuffer - data length is %d with address %X[0]=0x%x",length, data, *data);
  for(int i=0; i<length; i++)
  {
    //copy from main IRQ routine
    *((u8*)UART_DATA[UART2].RxBufWr) = *data;
    UART_DATA[UART2].RxBufWr++;
    if (UART_DATA[UART2].RxBufWr >= (UART_DATA[UART2].RxBuf + UART_DATA[UART2].RxBufSize))
    {
      LOG_DEBUG("feed_RingBuffer - reset write pointer at %d data[0x%02x]",i,*data);
      UART_DATA[UART2].RxBufWr = UART_DATA[UART2].RxBuf; // reset write pointer
    }
    data++;
  }
}

//------------------------------------------------------------------------------
// Check the message queue initialisation
void msg_queue_init_test(void **state)
{
  int8_t ret;
  initialize_global_data_map();
  GOWIN_Queue_Init();
  GOWIN_DATA.MsgCount = 0;
  LOG_INFO("Testing message init");

  assert_int_equal(GOWIN_QueueIndex_get(), 0);
  assert_int_equal(GOWIN_Queue_size(), 0);

  GOWIN_Process_Queue();

  LOG_OK("message queue init test passed");
}

//------------------------------------------------------------------------------
// Check the message queue function, wait/test for ACK before sending another msg
void msg_queue_should_pass_test(void **state)
{
  int8_t ret;
  initialize_global_data_map();
  GOWIN_Queue_Init();
  GOWIN_DATA.MsgCount = 0;
  LOG_INFO("Testing message queue");
  char _tx[1] = {ACK}; // ACK reply
  int cnt = GOWIN_DATA.GoodMsgCount; //check message counter

  GOWIN_Queue_Tx_Msg(BACKLIGHT_ON);
  GOWIN_Queue_Tx_Msg(BACKLIGHT_OFF);
  assert_int_equal(GOWIN_Queue_size(), 2);
  ret = GOWIN_Protocol(UART2);
  assert_int_equal(ret, GOWIN_RETURN_NONE);
  assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_ACK);

  ret = GOWIN_Protocol(UART2); // we are still waiting for ACK
  assert_int_equal(ret, GOWIN_RETURN_NONE);
  assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_ACK);

  feed_RingBuffer(_tx, 1);
  ret = GOWIN_Protocol(UART2); // process the ACK
  assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_START);
  assert_int_equal(GOWIN_Queue_size(), 1);

  // process the 2nd element of the queue
  ret = GOWIN_Protocol(UART2);
  assert_int_equal(ret, GOWIN_RETURN_NONE);
  assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_ACK);
  feed_RingBuffer(_tx, 1);

  ret = GOWIN_Protocol(UART2); // process the ACK

  assert_int_not_equal(GOWIN_DATA.GoodMsgCount,cnt);
  assert_int_equal(ret, GOWIN_WAIT_FOR_START);
  assert_int_equal(GOWIN_Queue_size(), 0);

  // check if next message can be processed
  feed_RingBuffer("$r#",3);
  ret = GOWIN_Protocol(UART2);
  assert_int_equal(ret, GOWIN_RETURN_NEW_MSG);

  LOG_OK("message queue test passed");
}

//------------------------------------------------------------------------------
// Check the message queue function, wait/test for ACK before sending another msg
void msg_queue_overflow_should_pass_test(void **state)
{
  int8_t ret;
  initialize_global_data_map();
  GOWIN_Queue_Init();
  GOWIN_DATA.MsgCount = 0;
  LOG_INFO("Testing message queue overflow");
  char _tx[1] = {ACK}; // ACK reply
  int cnt = GOWIN_DATA.GoodMsgCount; //check message counter

  assert_int_equal(GOWIN_QueueIndex_get(), 0);
  assert_int_equal(GOWIN_Queue_size(), 0);

  GOWIN_Queue_Tx_Msg(BACKLIGHT_ON); // gets written on index0
  GOWIN_Queue_Tx_Msg(BACKLIGHT_OFF); // gets written on index1...
  GOWIN_Queue_Tx_Msg(PANEL_VOLTAGELO);
  GOWIN_Queue_Tx_Msg(PANEL_VOLTAGEHI);
  GOWIN_Queue_Tx_Msg(BACKLIGHT_OFF);
  GOWIN_Queue_Tx_Msg(BACKLIGHT_ON);
  GOWIN_Queue_Tx_Msg(PANEL_PWR_OFF);
  GOWIN_Queue_Tx_Msg(PANEL_PWR_ON);
  assert_int_equal(GOWIN_Queue_size(), 8);
  assert_int_equal(GOWIN_QueueIndex_get(), 0);
  GOWIN_Queue_Tx_Msg(BACKLIGHT_OFF);
  GOWIN_Queue_Tx_Msg(PANEL_VOLTAGELO); // gets written on index9
  GOWIN_Queue_Tx_Msg(PANEL_VOLTAGEHI); // overwrites index0
  GOWIN_Queue_Tx_Msg(PANEL_PWR_OFF); // overwrites index0
  assert_int_equal(GOWIN_Queue_size(), GOWIN_MSG_QUEUE_SIZE);
  ret = GOWIN_Protocol(UART2);
  assert_int_equal(ret, GOWIN_RETURN_NONE);
  assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_ACK);


  for (int i; i <= GOWIN_MSG_QUEUE_SIZE+1; i++)
  {
    feed_RingBuffer(_tx, 1);
    ret = GOWIN_Protocol(UART2); // process the ACK
    ret = GOWIN_Protocol(UART2); // new cmd in queue
    if(GOWIN_Queue_size()) {
      assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_ACK);
      assert_int_equal(GOWIN_Queue_size(), GOWIN_MSG_QUEUE_SIZE-1-i);
      assert_int_equal(GOWIN_QueueIndex_get(), i+1);
      }
    else
      assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_START);
  }
  assert_int_equal(GOWIN_Queue_size(), 0);

  assert_int_not_equal(GOWIN_DATA.GoodMsgCount,cnt);

  //Queue is empty let's see if we can add a new element
  GOWIN_Queue_Tx_Msg(BACKLIGHT_ON);
  ret = GOWIN_Protocol(UART2);
  assert_int_equal(GOWIN_QueueIndex_get(), 0);
  assert_int_equal(GOWIN_Queue_size(), 1);
  feed_RingBuffer(_tx, 1);
  ret = GOWIN_Protocol(UART2);
  assert_int_equal(GOWIN_Queue_size(), 0);

  LOG_OK("message queue overflow test passed");
}

//------------------------------------------------------------------------------
// Check the message queue NACK response
void msg_queue_nack_test(void **state)
{
  int8_t ret;
  initialize_global_data_map();
  GOWIN_Queue_Init();
  GOWIN_DATA.MsgCount = 0;
  char _tx[1] = {NACK}; // ACK reply
  LOG_INFO("Testing message queue nack");

  int cntOK = GOWIN_DATA.GoodMsgCount; //check message counter
  int cntNOK = GOWIN_DATA.BadMsgCount; //check message counter

  GOWIN_Queue_Tx_Msg(SET_FPGA_ON);
  ret = GOWIN_Protocol(UART2);

  assert_int_equal(ret, GOWIN_RETURN_NONE);
  assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_ACK);

  feed_RingBuffer(_tx, 1);
  ret = GOWIN_Protocol(UART2);

  assert_int_equal(ret, GOWIN_RETURN_ERROR);
  assert_int_equal(GOWIN_DATA.State, GOWIN_WAIT_FOR_START);
  assert_int_equal(cntNOK+1, GOWIN_DATA.BadMsgCount);
  assert_int_equal(cntOK, GOWIN_DATA.GoodMsgCount);

  LOG_OK("message queue nack test passed");
}

//------------------------------------------------------------------------------
int main(void)
{
  const struct CMUnitTest gowin_protocol_queue_tests[] = {
    cmocka_unit_test(msg_queue_init_test),
    cmocka_unit_test(msg_queue_should_pass_test),
    cmocka_unit_test(msg_queue_overflow_should_pass_test),
    cmocka_unit_test(msg_queue_nack_test),
  };
  return cmocka_run_group_tests(gowin_protocol_queue_tests, NULL, NULL);
}
