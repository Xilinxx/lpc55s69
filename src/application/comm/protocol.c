/*********************** (C) COPYRIGHT BARCO 2021 ******************************
* File Name           : protocol.c
* Author              : Barco
* created             : 25/05/2021
* Description         : Communication protocol decoder from a serial stream
* History:
* 25/05/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/

// DECODER:
// Decodes a serial stream encoded with the protocol
// The principle is: we receive bytes on interrupt base
// We decode them in the background as soon as they arrive
// (we don't wait until the complete msg has arrived).
// We can only receive one message at the time, because it's not a streaming
// protocol. This keeps everything simple....
// The decoded message is stored in *COMM_DATA[].MsgBuf
// REMARKS:
// ISR handled outside this file

// ENCODER:
// Done real time, but not on interrupt base
// (The encoded TxMsg is not stored in RAM)
//

#define _COMM_PROTOCOL_C_

#include "protocol.h"

#ifndef UNIT_TEST
  #include <board.h>
  #include "peripherals.h"
#endif

#include "logger.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
// main.c global vars
EXTERN u8 MCU_MSGBUF[MCU_RXBUF_SIZE]; // uart1 message buffer
EXTERN u8 GOWIN_MSGBUF[GOWIN_RXBUF_SIZE]; // uart2 message buffer
extern volatile t_uart_data_raw UART_DATA[NUMBER_OF_PROTOCOL_UARTS]; // main.c

// initialize communication data structures
t_comm_data COMM_DATA[NUMBER_OF_PROTOCOL_UARTS] = {
    { (u8 *)&MCU_MSGBUF,   (u8 *)&MCU_MSGBUF,   MSGBUF_SIZE,   0,   0,   0,   0,
      COMM_WAIT_FOR_START, 0, 0, 0, 0, 0, 0, 0, 0 },
    { (u8 *)&GOWIN_MSGBUF, (u8 *)&GOWIN_MSGBUF, MSGBUF_SIZE,   0,   0,   0,   0,
      COMM_WAIT_FOR_START, 0, 0, 0, 0, 0, 0, 0, 0 }
};

const u8 COMM_START = COMM_START_BYTE;
const u8 COMM_STOP = COMM_STOP_BYTE;
const u8 COMM_ESC = COMM_ESCAPE;

/*******************************************************************************
 * Code
 ******************************************************************************/
// ------------------------------------------------------------------------------
// s8 COMM_Protocol(t_uart_channel uart_channel)
//  This function checks the raw uart buffer for valid messages.
//
//  Adds following chars to the msg:
//  SOP (start of packet) = 0xFE
//  EOP (end of packet    = 0xFF
//  ESCAPE CHAR           = 0x80
//  CHECKSUM = sum of previous bytes (without SOP and EOF!)
//
//  Format (at least 5 bytes!):
//  SOF ADR BYTE0 (BYTE1... BYTEN) CHECKSUM EOP
//
//  IMPORTANT:
//  The ADR is the first byte of the *data stream
// ------------------------------------------------------------------------------
t_comm_protocol_return_value COMM_Protocol(t_uart_channel uart_channel) {
    u8 c;

    while (UART_DATA[uart_channel].RxBufWr != UART_DATA[uart_channel].RxBufRd) {
        // check if we can still receive another msg
        if (COMM_DATA[uart_channel].MsgCount >= COMM_MAX_NO_MSG) {
            return NO_ERROR;
        }

        c = *UART_DATA[uart_channel].RxBufRd; // read char from circular UART buffer
        UART_DATA[uart_channel].RxBufRd++; // increment read pointer
        // LOG_DEBUG("COMM_Protocol process 0x%02X",c);

        if (UART_DATA[uart_channel].RxBufRd >= UART_DATA[uart_channel].RxBuf
            + UART_DATA[uart_channel].RxBufSize) {
            // reset read pointer
            UART_DATA[uart_channel].RxBufRd = UART_DATA[uart_channel].RxBuf;
        }

        // check if we have already received a start char, if not return
        if ((COMM_DATA[uart_channel].State == COMM_WAIT_FOR_START)
            && (c != COMM_START_BYTE)) {
            LOG_DEBUG("COMM_DISCARD_BYTE 0x%02X", c);
            // skip this char, and keep on skipping until start is found or buffer is empty
        } else {
            switch (c) {
                // ----------------------------------------------------------------------
                case (COMM_START_BYTE):
                    // in case of start char, unconditionally init the decoder!!

                    // check if we were already receiving a msg:
                    if (COMM_DATA[uart_channel].State != COMM_WAIT_FOR_START) {
                        if (COMM_DATA[uart_channel].BadMsgCount < 0xFFFF) {
                            COMM_DATA[uart_channel].BadMsgCount++; // increment bad msg counter
                        }
                    }

                    COMM_DATA[uart_channel].State = COMM_WAIT_FOR_STOP;
                    COMM_DATA[uart_channel].MsgLength = 0;
                    // reset write pointer
                    COMM_DATA[uart_channel].MsgBufWr = (u8 *)COMM_DATA[uart_channel].MsgBuf;
                    COMM_DATA[uart_channel].offset = 0;
                    COMM_DATA[uart_channel].checksum = 0;
                    COMM_DATA[uart_channel].previous_char = 0;
                    // LOG_DEBUG("COMM_START_BYTE");
                    break;
                // ----------------------------------------------------------------------
                case COMM_STOP_BYTE:
                    // LOG_DEBUG("COMM_STOP_BYTE");
                    // complete message has been received, incr msg counter, store message length
                    COMM_DATA[uart_channel].State = COMM_WAIT_FOR_START;

                    if (COMM_DATA[uart_channel].checksum == *(COMM_DATA[uart_channel].MsgBufWr -
                                                              1)) {
                        // any address is a valid one... (other than own addresses will be discarded!)
                        // at least address, cmdbyte and checksum
                        if ((COMM_DATA[uart_channel].MsgBufWr - COMM_DATA[uart_channel].MsgBuf) >=
                            3) {
                            COMM_DATA[uart_channel].MsgCount++; // set new msg flag
                            // store msg length (don't count checksum)
                            COMM_DATA[uart_channel].MsgLength =
                                (u16)(COMM_DATA[uart_channel].MsgBufWr -
                                      COMM_DATA[uart_channel].MsgBuf -
                                      1);
                            COMM_DATA[uart_channel].GoodMsgCount++;
                            // LOG_DEBUG("COMM_RETURN_NEW_MSG");
                            return NEW_MSG;
                        } else {
                            LOG_DEBUG("COMM_RETURN_ERROR");
                            if (COMM_DATA[uart_channel].BadMsgCount < 0xFFFF) {
                                COMM_DATA[uart_channel].BadMsgCount++;
                            }
                            return ERROR;
                        }
                    } else {
                        LOG_DEBUG("COMM_RETURN_ERROR CRC, expected %X is %X",
                                  COMM_DATA[uart_channel].checksum,
                                  *(COMM_DATA[uart_channel].MsgBufWr - 1));
                        if (COMM_DATA[uart_channel].BadMsgCount < 0xFFFF) {
                            COMM_DATA[uart_channel].BadMsgCount++; // increment bad msg counter
                        }
                        return CRC_ERROR;
                    }
                    break;
                // ----------------------------------------------------------------------
                case COMM_ESCAPE:
                    // we don't store the escape char
                    COMM_DATA[uart_channel].offset = COMM_ESCAPE;
                    break;
                // ----------------------------------------------------------------------
                default:
          #pragma GCC diagnostic push
          #pragma GCC diagnostic ignored "-Wconversion"
                    c += COMM_DATA[uart_channel].offset; // add the offset, so if previous char was an eccape char, this will correct it
                    COMM_DATA[uart_channel].offset = 0; // reset the offset because previous was ESCAPED
                    *COMM_DATA[uart_channel].MsgBufWr = c; // store the received char in buf
                    COMM_DATA[uart_channel].checksum += COMM_DATA[uart_channel].previous_char; // calculate checksum (this includes the ESC_CHAR)
                    COMM_DATA[uart_channel].previous_char = c; // we use previous char
          #pragma GCC diagnostic pop
                    if (COMM_DATA[uart_channel].MsgBufWr < (COMM_DATA[uart_channel].MsgBuf
                                                            + COMM_DATA[uart_channel].MsgBufSize -
                                                            1)) {
                        COMM_DATA[uart_channel].MsgBufWr++;
                    } else {
                        // generate error
                        COMM_DATA[uart_channel].State = COMM_WAIT_FOR_START;
                        if (COMM_DATA[uart_channel].BadMsgCount < 0xFFFF) {
                            COMM_DATA[uart_channel].BadMsgCount++; // increment bad msg counter
                        }
                    }
                    break;
            }
        }
    }

    return NO_ERROR;
}

// ------------------------------------------------------------------------------
// void PROTO_TX_SendMsg(t_uart_channel uart_channel, u8 *data, u16 length)
//  encodes AND sends a serial stream according to the protocol
//  Adds following chars to the msg:
//  SOF (start of packet) = 0xFE
//  EOP (end of packet    = 0xFF
//  ESCAPE CHAR           = 0x80
//  CHECKSUM = sum of previous bytes (without SOF and EOF!)
//
//  Format (at least 5 bytes!):
//  SOF RA BYTE0 (BYTE1... BYTEN) CHECKSUM EOP
//
//  IMPORTANT:
//  We assume that RA is the first byte of the *data stream
// ------------------------------------------------------------------------------
void PROTO_TX_SendMsg(t_uart_channel uart_channel,
                      u8 * data,
                      u16 length) {
#ifndef UNIT_TEST  // mock is being used for UNIT_TEST
    u8 c, checksum;
    u16 j;

    USART_Type * base;

    // select UART base address
    switch (uart_channel) {
        case UART1:
            base = USART0;
            break;

        case UART2:
            base = USART1;
            break;

        default:      return;
    }

    checksum = 0;
    USART_WriteBlocking(base, (u8 *)&COMM_START, 1);
    for (j = 0; j <= length; j++) {
        if (j < length) {
            c = data[j];
            checksum = (u8)(checksum + c);
        } else
            c = checksum;

        if ((c == COMM_ESCAPE) || (c == COMM_START_BYTE) || (c == COMM_STOP_BYTE)) {
            USART_WriteBlocking(base, (u8 *)&COMM_ESC, 1);
            c = (u8)(c - COMM_ESCAPE); // calculate new char
        }
        USART_WriteBlocking(base, (u8 *)&c, 1);
    }
    USART_WriteBlocking(base, (u8 *)&COMM_STOP, 1);

    while (!(kUSART_TxFifoEmptyFlag & USART_GetStatusFlags(base))) {
    } // wait till transmission complete

#else
    LOG_ERROR("PROTO_TX_SendMsg should be mocked");
#endif


}
