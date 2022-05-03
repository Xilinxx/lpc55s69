/*********************** (C) COPYRIGHT BARCO  *********************************
* File Name           : unit_test_comm_protocol_helper.c  - native
* Author              : DAVTH
* created             : 08/06/2021
* Description         : protocol test helper

* History:
* 25/5/2021 - initial
*******************************************************************************/
#include "unit_test_comm_protocol_helper.h"

// initialize raw uart data structures , Note: GOWIN_RXBUF is never used.
volatile t_uart_data_raw UART_DATA[NUMBER_OF_PROTOCOL_UARTS] = {
  { (u8*)&MCU_RXBUF, MCU_RXBUF_SIZE, (u8*)&MCU_RXBUF, (u8*)&MCU_RXBUF },
  { (u8*)&GOWIN_RXBUF, GOWIN_RXBUF_SIZE, (u8*)&GOWIN_RXBUF, (u8*)&GOWIN_RXBUF }
};

// feed_RingBuffer - helper function for creating a valid input buffer
u8 feed_RingBuffer(const char * data, u16 length, bool print)
{
  *((u8*)UART_DATA[UART1].RxBufWr++) = (u8)COMM_START_BYTE;
  *((u8*)UART_DATA[UART1].RxBufWr++) = (u8)ADR;
  //LOG_INFO("feed_RingBuffer - data length is %d",length);
  u8 crc = ADR;
  for(int i=0; i<length; i++)
  {
    // copy from main IRQ routine
    if (print)
    {
      LOG_INFO("feed_RingBuffer - with %d %02X",i, (u8)*data);
    }
    // ESCAPE TRANSLATION
    if ((*data == COMM_ESCAPE) || (*data == (char)COMM_START_BYTE) ||
        (*data == (char)COMM_STOP_BYTE))
    {
      *((u8*)UART_DATA[UART1].RxBufWr++) = COMM_ESCAPE;
      crc += COMM_ESCAPE;
      *((u8*)UART_DATA[UART1].RxBufWr++) = *data - COMM_ESCAPE; //calculate new char
      crc += *data - COMM_ESCAPE;
    }
    else
    {
      crc += *data;
      *((u8*)UART_DATA[UART1].RxBufWr++) = *data;
    }
    data++;
    if ( UART_DATA[UART1].RxBufWr >=
        (UART_DATA[UART1].RxBuf + UART_DATA[UART1].RxBufSize)
       )
    {
      LOG_INFO("feed_RingBuffer - reset write pointer at %d",i);
      UART_DATA[UART1].RxBufWr = UART_DATA[UART1].RxBuf; // reset write pointer
    }
  }
  *((u8*)UART_DATA[UART1].RxBufWr++) = crc;
  *((u8*)UART_DATA[UART1].RxBufWr++) = (u8)COMM_STOP_BYTE;

  return (length + 1); //add address , crc does not count
}

void modify_RingBuffer(u8 backward,u8 newval)
{
  LOG_DEBUG("Modify ringbuffer value %x with %x ",
            *((u8*)UART_DATA[UART1].RxBufWr-backward), newval);
  *((u8*)UART_DATA[UART1].RxBufWr-backward) = newval;
}


