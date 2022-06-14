/*********************** (C) COPYRIGHT BARCO  *********************************
 * File Name           : GOWIN_protocol.c
 * Author              : DAVTH
 * created             : 5/05/2021
 * Description         : GOWIN protocol decoder from a serial stream
 * History:
 * 05/05/2021 : copied from lcd_protocol introduced in gpmcu code (DAVTH)
 *******************************************************************************/

// DECODER:
// Decodes a serial stream encoded with the GOWIN protocol
// The principle is: we receive bytes on interrupt base
// We decode them in the background as soon as they arrive.
// We can only receive one message at the time, it's not a streaming protocol.
//
// The decoded message is stored in *GOWIN_DATA[].MsgBuf
// REMARKS:
//  ISR handled outside this file

//

#define _GOWIN_PROTOCOL_C_

#include "gowin_protocol.h"

#ifndef UNIT_TEST
#include "peripherals.h"
#include "fsl_common_arm.h"
#endif

#include <ctype.h>
#include <stdio.h>

#include "logger.h"

#include "data_map.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern volatile t_uart_data_raw UART_DATA[2]; // main.c

u8 FPGA_MSGBUF[FPGA_MSGBUF_SIZE]; // uart1 message buffer

// initialize GOWIN data structures
t_gowin_data GOWIN_DATA = { (u8 *)&FPGA_MSGBUF, (u8 *)&FPGA_MSGBUF,
                            FPGA_MSGBUF_SIZE,   0,                 0,0, 0, GOWIN_WAIT_FOR_START, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

t_gowin_msg GOWIN_MSG_QUEUE[GOWIN_MSG_QUEUE_SIZE] = {};

static const uint8_t GOWIN_START = GOWIN_START_BYTE;
static const uint8_t GOWIN_STOP = GOWIN_STOP_BYTE;

static u8 EdidIndex = 0;
static u8 DpcdIndex = 0;
static u8 MsgIndex = 0;

/*******************************************************************************
 * Enum/Define to string helper code
 ******************************************************************************/
const char * returnStateName(t_gowin_state val) {
    switch (val) {
        case GOWIN_WAIT_FOR_START:
            return "GOWIN_WAIT_FOR_START";
        case GOWIN_WAIT_FOR_STOP:
            return "GOWIN_WAIT_FOR_STOP";
        case GOWIN_WAIT_FOR_EDID:
            return "GOWIN_WAIT_FOR_EDID";
        case GOWIN_WAIT_FOR_DPCD:
            return "GOWIN_WAIT_FOR_DPCD";
        case GOWIN_READING_EDID:
            return "GOWIN_READING_EDID";
        case GOWIN_READING_DPCD:
            return "GOWIN_READING_DPCD";
        case GOWIN_WAIT_FOR_EDID_WRITE_ACK:
            return "GOWIN_WAIT_FOR_EDID_WRITE_ACK";
        case GOWIN_WAIT_FOR_ACK:
            return "GOWIN_WAIT_FOR_ACK";
        case GOWIN_READING_1BYTE:
            return "GOWIN_READING_1BYTE";
        case GOWIN_READING_2BYTE:
            return "GOWIN_READING_2BYTE";
        default:
            return "";
            break;
    }
}

const char * returnCodeName(int val) {
    switch (val) {
        case GOWIN_RETURN_NONE:
            return "GOWIN_RETURN_NONE";
        case GOWIN_RETURN_NEW_MSG:
            return "GOWIN_RETURN_NEW_MSG";
        case GOWIN_RETURN_ERROR:
            return "GOWIN_RETURN_ERROR";
        case GOWIN_RETURN_TIMEOUT:
            return "GOWIN_RETURN_TIMEOUT";
        case GOWIN_RETURN_MAX_MSG:
            return "GOWIN_RETURN_MAX_MSG";
        case GOWIN_RETURN_WAIT_EDID_ACK:
            return "GOWIN_RETURN_WAIT_EDID_ACK";
        case GOWIN_RETURN_TX_QUEUE:
            return "GOWIN_RETURN_TX_QUEUE";
        default:
            return "GOWIN_RETURN_undefined";
    }
}

const char * returnCmdName(t_gowin_command val) {
    switch (val) {
        case NO_GOWIN_CMD:
            return "NO_GOWIN_CMD";
        case ACK:
            return "ACK";
        case NACK:
            return "NACK";
        case SET_FPGA_OFF:
            return "SET_FPGA_OFF";
        case SET_STANDBY_OFF:
            return "SET_STANDBY_OFF";
        case SET_FPGA_ON:
            return "SET_FPGA_ON";
        case SET_STANDBY_ON:
            return "SET_STANDBY_ON";
        case WRITE_EDID:
            return "WRITE_EDID";
        case READ_EDID:
            return "READ_EDID";
        case WRITE_DPCD:
            return "WRITE_DPCD";
        case READ_DPCD:
            return "READ_DPCD";
        case WAKE_UP:
            return "WAKE_UP";
        case PANEL_VOLTAGELO:
            return "PANEL_VOLTAGELO";
        case PANEL_VOLTAGEHI:
            return "PANEL_VOLTAGEHI";
        case BACKLIGHT_OFF:
            return "BACKLIGHT_OFF";
        case BACKLIGHT_ON:
            return "BACKLIGHT_ON";
        case PANEL_PWR_OFF:
            return "PANEL_PWR_OFF";
        case PANEL_PWR_ON:
            return "PANEL_PWR_ON";
        case READ_ETH_STATUS:
            return "READ_ETH_STATUS";
        case READ_BL_CURRENT:
            return "READ_BL_CURRENT";

        default:
            return "";
    }
}

/*******************************************************************************
 * Code
 ******************************************************************************/

void AddRx2Buffer(const u8 c) {
    // LOG_DEBUG("GOWIN AddRx2Buffer add %c",c);
    *GOWIN_DATA.MsgBufWr = c; // store the received char in buf
    GOWIN_DATA.offset++;
    GOWIN_DATA.previous_char = c;
    if (GOWIN_DATA.MsgBufWr < (GOWIN_DATA.MsgBuf + GOWIN_DATA.MsgBufSize - 1)) {
        GOWIN_DATA.MsgBufWr++;
    } else {
        // generate error
        LOG_DEBUG("GOWIN BadMsgCount, set State = GOWIN_WAIT_FOR_START");
        GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_START);
        if (GOWIN_DATA.BadMsgCount < 0xFFFF) {
            GOWIN_DATA.BadMsgCount++; // increment bad msg counter
        }
    }
}

// ------------------------------------------------------------------------------
void StartByte() {
    GOWIN_DATA.RxBusy = true;
    // LOG_DEBUG("GOWIN_START_BYTE");
    // in case of start char, unconditionally init the decoder

    // check if we were already receiving a msg:
    if (GOWIN_DATA.State != GOWIN_WAIT_FOR_START)
        if (GOWIN_DATA.BadMsgCount < 0xFFFF)
            GOWIN_DATA.BadMsgCount++; // increment bad msg counter

    GOWIN_DATA.MsgLength = 0;
    GOWIN_DATA.MsgBufWr = (u8 *)GOWIN_DATA.MsgBuf; // reset write pointer
    GOWIN_DATA.offset = 0;
    GOWIN_DATA.previous_char = 0;
    GOWIN_DATA.TimeoutCount = 0;
    GOWIN_DATA.MsgBufSize = FPGA_MSGBUF_SIZE;
}

// ------------------------------------------------------------------------------
void StopByte() {
    // complete message has been received, incr msg counter, store message length
    GOWIN_DATA.MsgCount++;
    // store msg length
    GOWIN_DATA.MsgLength = (u16)(GOWIN_DATA.MsgBufWr - GOWIN_DATA.MsgBuf - 1);
    LOG_DEBUG("StopByte() GOWIN_DATA.MsgLength = %d", GOWIN_DATA.MsgLength);
}

// ------------------------------------------------------------------------------
// int8_t process_rx_characters(const u8 c)
// local helper function
// Called from State_Machine, while waiting for START_BYTE
// ------------------------------------------------------------------------------
int8_t process_rx_character(const u8 c) {
    switch (c) {
        case GOWIN_START_BYTE:
            StartByte();
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_STOP);
            break;
        // --------------------------------------------------------------------------
        case GOWIN_ACK:
            GOWIN_DATA.GoodMsgCount++;
            LOG_DEBUG("State[%s] GOWIN_ACK received", returnStateName(GOWIN_DATA.State));
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_START);
            break;
        // --------------------------------------------------------------------------
        case GOWIN_NACK:
            LOG_DEBUG("State[%s] GOWIN_NACK received", returnStateName(GOWIN_DATA.State));
            if (GOWIN_DATA.BadMsgCount < 0xFFFF)
                GOWIN_DATA.BadMsgCount++; // increment bad msg counter
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_START);
            return GOWIN_RETURN_ERROR;
            break;
        // --------------------------------------------------------------------------
        default:
            // unknown characters do not get processed
            break;
    }
    return GOWIN_RETURN_NONE;
}

// ------------------------------------------------------------------------------
// int8_t process_rx_data_character(const u8 c)
// local helper function
// Called from State_Machine, while data is coming in.
// Commands start with '$'/startbyte, we process the startbyte as data
// ------------------------------------------------------------------------------
void process_rx_data_character(const u8 c, const u16 size) {
    // LOG_DEBUG("==%d==", GOWIN_DATA.offset);
    AddRx2Buffer(c);
    if (GOWIN_DATA.offset >= size) {
        // incoming-data has no STOPBYTE! simulate it here.
        StopByte();

        LOG_DEBUG("State[%s] done (timeout count = %d/%d) , rx size[%d]",
                  returnStateName(GOWIN_DATA.State),
                  GOWIN_DATA.TimeoutCount,
                  GOWIN_REPLY_TIMEOUT,
                  GOWIN_DATA.MsgLength);

        // LOG_DEBUG("..Last byte [%x], First byte [%x]", *(GOWIN_DATA.MsgBufWr-1), *(GOWIN_DATA.MsgBufWr-1-GOWIN_DATA.MsgLength));
        // LOG_DEBUG("..Last ADR 0x%X, First ADR 0x%X", &GOWIN_DATA.MsgBufWr-1, &GOWIN_DATA.MsgBufWr-1-GOWIN_DATA.MsgLength);
        switch (GOWIN_DATA.State) {
            case GOWIN_READING_EDID:
                store_edid_data(EdidIndex, GOWIN_DATA.MsgBufWr - GOWIN_DATA.MsgLength - 1);
                GOWIN_Print_edid_in_log(EdidIndex);
                break;
            case GOWIN_READING_DPCD:
                store_dpcd_data(DpcdIndex, GOWIN_DATA.MsgBufWr - GOWIN_DATA.MsgLength - 1);
                GOWIN_Print_edid_in_log(DpcdIndex + 3u);
                break;
            case GOWIN_READING_1BYTE:
                LOG_DEBUG("GOWIN_READING_1BYTE = 0x[%02X]", *(GOWIN_DATA.MsgBufWr - 1));
                Main.DeviceState.EthernetStatusBits = *(GOWIN_DATA.MsgBufWr - 1);
                break;
            case GOWIN_READING_2BYTE:
                LOG_DEBUG("GOWIN_READING_2BYTE = 0x[%02X %02X]", *(GOWIN_DATA.MsgBufWr - 2), *(GOWIN_DATA.MsgBufWr - 1));
                Main.DeviceState.PanelCurrent = (u16)(*(GOWIN_DATA.MsgBufWr - 2) + (*(GOWIN_DATA.MsgBufWr - 1) << 8));
                break;
            default:
                LOG_WARN("unhandled state during rx_data process [%s]", returnStateName(GOWIN_DATA.State));
                break;
        }
        GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_START);
        GOWIN_DATA.RxBusy = false;
    }
}

// ------------------------------------------------------------------------------
// int8_t GOWIN_State_Machine(const u8 c)
// Called upon every received char
// ------------------------------------------------------------------------------
int8_t GOWIN_State_Machine(const u8 c) {
    // LOG_DEBUG("State_machine char %02X,state = %s", c,returnStateName(GOWIN_DATA.State));
    switch (GOWIN_DATA.State) {
        case GOWIN_WAIT_FOR_EDID:
        case GOWIN_WAIT_FOR_DPCD:
            // EDID reply start with $<256byte> = 257byte
            if (c == GOWIN_START_BYTE) {
                StartByte();
                if ((GOWIN_DATA.State == GOWIN_WAIT_FOR_EDID)) {
                    LOG_DEBUG("State[%s] got STARTBYTE", returnStateName(GOWIN_DATA.State));
                    GOWIN_DATA.State = GOWIN_READING_EDID;
                }
                if (GOWIN_DATA.State == GOWIN_WAIT_FOR_DPCD) {
                    LOG_DEBUG("State[%s] got STARTBYTE", returnStateName(GOWIN_DATA.State));
                    GOWIN_DATA.State = GOWIN_READING_DPCD;
                }
            } else {
                LOG_DEBUG("State[%s] missing STARTBYTE got [0x%02x,%c]",
                          returnStateName(GOWIN_DATA.State), c, c);
            }
            break;
        // --------------------------------------------------------------------------
        case GOWIN_WAIT_FOR_EDID_WRITE_ACK:
            if (c == GOWIN_ACK) {
                GOWIN_DATA.GoodMsgCount++;
                LOG_DEBUG("State[%s] GOWIN_ACK received", returnStateName(GOWIN_DATA.State));
                GOWIN_DATA.State = GOWIN_WAIT_FOR_START;
                GOWIN_DATA.RxBusy = false;
            } else {
                LOG_DEBUG("State[%s] - dropping byte [0x%02x,%c]",
                          returnStateName(GOWIN_DATA.State), c, c);
            }
            break;
        // --------------------------------------------------------------------------
        case GOWIN_READING_EDID:
        case GOWIN_READING_DPCD:
            process_rx_data_character(c, GOWIN_EDID_BYTE_LENGTH);
            if (!GOWIN_DATA.RxBusy)
                return GOWIN_RETURN_NEW_MSG;
            break;
        case GOWIN_READING_1BYTE:
            process_rx_data_character(c, 2); // '$' 'b0'
            LOG_DEBUG("State[%s] GOWIN_READING_1BYTE  / RxBusy=%d",
                      returnStateName(GOWIN_DATA.State),
                      GOWIN_DATA.RxBusy);
            if (!GOWIN_DATA.RxBusy)
                return GOWIN_RETURN_NEW_MSG;
            break;
        case GOWIN_READING_2BYTE:
            process_rx_data_character(c, 3); // '$' 'b0' 'b1'
            LOG_DEBUG("State[%s] GOWIN_READING_2BYTE / RxBusy=%d",
                      returnStateName(GOWIN_DATA.State),
                      GOWIN_DATA.RxBusy);
            if (!GOWIN_DATA.RxBusy)
                return GOWIN_RETURN_NEW_MSG;
            break;
        // --------------------------------------------------------------------------
        case GOWIN_WAIT_FOR_ACK: // when expecting an ACK upon tx
        case GOWIN_WAIT_FOR_START: // Default state - wait for command
            if (process_rx_character(c) == GOWIN_RETURN_ERROR)
                return GOWIN_RETURN_ERROR;
            break;
        // --------------------------------------------------------------------------
        default: // GOWIN_WAIT_FOR_STOP
            switch (c) {
                case GOWIN_START_BYTE: // unexpected start after start..
                    GOWIN_DATA.State = GOWIN_WAIT_FOR_STOP;
                    break;
                // ------------------------------------------------------------------------
                case GOWIN_STOP_BYTE:
                    LOG_DEBUG("State[%s] GOWIN_STOP_BYTE", returnStateName(GOWIN_DATA.State));
                    StopByte();
                    GOWIN_DATA.State = GOWIN_WAIT_FOR_START;
                    GOWIN_DATA.RxBusy = false;
                    GOWIN_DATA.WaitForReply = false;
                    return GOWIN_RETURN_NEW_MSG;
                    break;
                // ------------------------------------------------------------------------
                default:
                    AddRx2Buffer(c);
                    break;
            }
            break;
    }

    return GOWIN_RETURN_NONE;
}

// ------------------------------------------------------------------------------
// u8 GOWIN_Protocol(t_uart_channel uart_channel)
//  This function checks the raw uart buffer for 'GOWIN' messages.
//  This functions transmits pending commands from the Queue
//
//  Adds following chars to the msg:
//  SOF (start of packet) = 0x
//  EOP (end of packet    = 0x
//
//  Format (always 3 bytes!) , except NACK ACK
//  STX <cmd_id> ETX
//
// There is actually only coming in
// * cmd WAKE_UP '%'
// * ACK/NACK
// * $<256byte> EDID data
// ------------------------------------------------------------------------------
int8_t GOWIN_Protocol(t_uart_channel uart_channel) {
    u8 c;
    int8_t ret = GOWIN_RETURN_NONE;

    const uint32_t _kUSART_RxFifoFullFlag = 128; // from f_usart.h

    if (_kUSART_RxFifoFullFlag & UART_DATA[UART1].flags) {
        // we do not print from the IRQ-routine but here
        LOG_WARN("Flexcomm Gowin kUSART_RxFifoFullFlag [%X]", UART_DATA[UART1].flags);
    }

    // LOG_DEBUG("GOWIN Wr(%x)Rd(%x)",UART_DATA[uart_channel].RxBufWr,UART_DATA[uart_channel].RxBufRd);
    while (UART_DATA[uart_channel].RxBufWr != UART_DATA[uart_channel].RxBufRd) {
        // check if we can still receive another msg
        if (GOWIN_DATA.MsgCount >= GOWIN_MAX_NO_MSG) {
            LOG_WARN("GOWIN_DATA.MsgCount == 1");
            return GOWIN_RETURN_MAX_MSG;
        }

        c = *UART_DATA[uart_channel].RxBufRd; // read char from circular UART buffer
        if ((GOWIN_DATA.State != GOWIN_READING_EDID) &
            (GOWIN_DATA.State != GOWIN_READING_DPCD)) {
            if (isprint(c))
                LOG_RAW("Gowin RX char is 0x%02X %c\tMsgCount[%d] State[%s]", c, c, GOWIN_DATA.MsgCount, returnStateName(GOWIN_DATA.State));
            else
                LOG_RAW("Gowin RX char is 0x%02X\tMsgCount[%d] State[%s]", c, GOWIN_DATA.MsgCount, returnStateName(GOWIN_DATA.State));
        }
        UART_DATA[uart_channel].RxBufRd++; // increment read pointer

        if (UART_DATA[uart_channel].RxBufRd >=
            (UART_DATA[uart_channel].RxBuf + UART_DATA[uart_channel].RxBufSize)) {
            // reset read pointer
            UART_DATA[uart_channel].RxBufRd = UART_DATA[uart_channel].RxBuf;
            LOG_DEBUG("buffer rotation, Reset Read pointer");
        }
        ret = GOWIN_State_Machine(c);
        if (ret == GOWIN_RETURN_NEW_MSG)
            break;
        if (ret < 0)
            break;
    }

    // LOG_DEBUG("GOWIN_Protocol queue index=%d\t state=%s", MsgIndex, returnStateName(GOWIN_DATA.State));

    // timeout processing (based on nr of iterations of this loop, to do: use timer)
    if (GOWIN_DATA.State != GOWIN_WAIT_FOR_START) {
        if (GOWIN_DATA.TimeoutCount++ > GOWIN_REPLY_TIMEOUT) {
            LOG_DEBUG("GOWIN reply Timeout reached on state %s", returnStateName(GOWIN_DATA.State));
            if (GOWIN_DATA.BadMsgCount < 0xFFFF)
                GOWIN_DATA.BadMsgCount++; // increment bad msg counter
            GOWIN_DATA.TimeoutCount = 0;
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_START);
            GOWIN_DATA.RxBusy = false;
            GOWIN_DATA.WaitForReply = false;
            return GOWIN_RETURN_TIMEOUT;
        }

        if (GOWIN_DATA.State == GOWIN_WAIT_FOR_EDID_WRITE_ACK)
            return GOWIN_RETURN_WAIT_EDID_ACK;
    } else { // GOWIN_WAIT_FOR_START
        if (GOWIN_DATA.WaitForReply == true) {
            GOWIN_DATA.WaitForReply = false;
            if (GOWIN_MSG_QUEUE[MsgIndex].cmd != NO_GOWIN_CMD)
                LOG_DEBUG("Finished queue-msg index %d with cmd %s", MsgIndex,
                          returnCmdName(GOWIN_MSG_QUEUE[MsgIndex].cmd));
            GOWIN_MSG_QUEUE[MsgIndex].cmd = NO_GOWIN_CMD;
        } else {
            if (GOWIN_Queue_size())
                GOWIN_Process_Queue();
        }
    }

    // LOG_DEBUG("GOWIN_Protocol ret=%s, state=%s", returnCodeName(ret), returnStateName(GOWIN_DATA.State));
    return ret;
}

// ------------------------------------------------------------------------------
u8 GOWIN_QueueIndex_get() {
    return MsgIndex;
}

// ------------------------------------------------------------------------------
void GOWIN_Queue_Init() {
    for (int i = 0u; i < GOWIN_MSG_QUEUE_SIZE; i++) {
        GOWIN_MSG_QUEUE[i].cmd = NO_GOWIN_CMD;
        GOWIN_MSG_QUEUE[i].param = 0;
    }
    MsgIndex = 0;
}

// ------------------------------------------------------------------------------
u8 GOWIN_Queue_size() {
    u8 pending_msg = 0;

    for (int i = 0u; i < GOWIN_MSG_QUEUE_SIZE; i++) {
        if (GOWIN_MSG_QUEUE[i].cmd != NO_GOWIN_CMD)
            pending_msg++;
    }
    if (pending_msg != 0)
        LOG_DEBUG("Queue size is %d", pending_msg);

    if (pending_msg == GOWIN_MSG_QUEUE_SIZE)
        LOG_WARN("Gowin message-Queue is FULL!");

    return pending_msg;
}

// ------------------------------------------------------------------------------
// void GOWIN_Queue_Tx_Msg(t_gowin_command cmd)
//  queue up messages for TX
void GOWIN_Queue_Tx_Msg(t_gowin_command cmd) {
    u8 newId = MsgIndex;

    for (int i = 0u; i < GOWIN_MSG_QUEUE_SIZE; i++) {
        if (GOWIN_MSG_QUEUE[i].cmd == NO_GOWIN_CMD) {
            newId = (u8)i;
            break;
        }
    }

    GOWIN_MSG_QUEUE[newId].cmd = cmd;

    LOG_DEBUG("GOWIN_Queue_Tx_Msg %s on index %d", returnCmdName(cmd), newId);
}

// ------------------------------------------------------------------------------
// void GOWIN_Queue_Tx_Msg_Index(t_gowin_command cmd,u8 storage_index)
//  queue up messages for TX
void GOWIN_Queue_Tx_Msg_Index(t_gowin_command cmd, u8 storage_index) {
    // we could transmit immediatly, but queue anyhow

    u8 newId = MsgIndex;

    for (int i = 0u; i < GOWIN_MSG_QUEUE_SIZE; i++) {
        if (GOWIN_MSG_QUEUE[i].cmd == NO_GOWIN_CMD) {
            newId = (u8)i;
            break;
        }
    }
    GOWIN_MSG_QUEUE[newId].cmd = cmd;
    LOG_DEBUG("GOWIN_Queue_Tx_Msg_Index %s[%d] on index %d", returnCmdName(cmd), storage_index, newId);

    if ((cmd == WRITE_EDID) || (cmd == READ_EDID))
        GOWIN_MSG_QUEUE[newId].param = storage_index;

    if ((cmd == WRITE_DPCD) || (cmd == READ_DPCD))
        GOWIN_MSG_QUEUE[newId].param = storage_index;
}

// ------------------------------------------------------------------------------
// void GOWIN_Process_Queue()
//  Processes the messages ready for transmit
void GOWIN_Process_Queue() {
    // set index to next command
    for (int i = 0u; i < GOWIN_MSG_QUEUE_SIZE; i++) {
        if (GOWIN_MSG_QUEUE[i].cmd != NO_GOWIN_CMD) {
            MsgIndex = (u8)i;
            break;
        }
    }

    if (GOWIN_MSG_QUEUE[MsgIndex].cmd == NO_GOWIN_CMD)
        return;

    LOG_DEBUG("GOWIN_Process_Queue %d cmd [%s][%d]", MsgIndex,
              returnCmdName(GOWIN_MSG_QUEUE[MsgIndex].cmd),
              GOWIN_MSG_QUEUE[MsgIndex].param);

    switch (GOWIN_MSG_QUEUE[MsgIndex].cmd) {
        case WRITE_EDID:
        case READ_EDID:
            EdidIndex = GOWIN_MSG_QUEUE[MsgIndex].param;
            break;
        case WRITE_DPCD:
        case READ_DPCD:
            DpcdIndex = GOWIN_MSG_QUEUE[MsgIndex].param;
            break;
        default:
            break;
    }

    GOWIN_TX_SendMsg(GOWIN_MSG_QUEUE[MsgIndex].cmd);
}

// ------------------------------------------------------------------------------
// void GOWIN_TX_SendMsg(u8 *data, u16 length)
//  Sends a serial stream according to the GOWIN protocol
//
//  Format (3 bytes):
// ------------------------------------------------------------------------------
int8_t GOWIN_TX_SendMsg(t_gowin_command cmd) {
#ifndef UNIT_TEST
    USART_Type * base;
    base = USART1;

    if ((cmd == ACK) | (cmd == NACK)) { // single byte reply
        USART_WriteBlocking(base, (u8 *)&cmd, 1);
    } else { // 3 byte tx
        GOWIN_DATA.WaitForReply = true;
        USART_WriteBlocking(base, (u8 *)&GOWIN_START, 1);
        USART_WriteBlocking(base, (u8 *)&cmd, 1);
        USART_WriteBlocking(base, (u8 *)&GOWIN_STOP, 1);
        if (cmd == READ_EDID) {
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_EDID);
            LOG_DEBUG("State->GOWIN_WAIT_FOR_EDID[%d]", EdidIndex);
        } else if (cmd == READ_DPCD) {
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_DPCD);
            LOG_DEBUG("State->GOWIN_WAIT_FOR_DPCD[%d]", DpcdIndex);
        } else if (cmd == WRITE_EDID) { // 256byte of data
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_EDID_WRITE_ACK);
            LOG_DEBUG("GOWIN_WRITE_EDID index[%d]", EdidIndex);
            if (EdidIndex == 0)
                USART_WriteBlocking(base, (u8 *)&Main.Edid.Edid1, EDID_SIZE);
            else if (EdidIndex == 1)
                USART_WriteBlocking(base, (u8 *)&Main.Edid.Edid2, EDID_SIZE);
            else if (EdidIndex == 2)
                USART_WriteBlocking(base, (u8 *)&Main.Edid.Edid3, EDID_SIZE);
            else
                LOG_ERROR("GOWIN_WRITE_EDID wrong index");
        } else if (cmd == WRITE_DPCD) { // 256byte of data
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_EDID_WRITE_ACK);
            LOG_DEBUG("GOWIN_WRITE_DPCD index[%d]", DpcdIndex);
            if (DpcdIndex == 0)
                USART_WriteBlocking(base, (u8 *)&Main.Dpcd.Dpcd1, EDID_SIZE);
            else if (DpcdIndex == 1)
                USART_WriteBlocking(base, (u8 *)&Main.Dpcd.Dpcd2, EDID_SIZE);
            else if (DpcdIndex == 2)
                USART_WriteBlocking(base, (u8 *)&Main.Dpcd.Dpcd3, EDID_SIZE);
            else
                LOG_ERROR("GOWIN_WRITE_DPCD wrong index");
            // LOG_RAW("---WRITE DPCD DATA---");
            // GOWIN_Print_edid_in_log(DpcdIndex + 3u);
        } else if (cmd == READ_ETH_STATUS) {
            GOWIN_Protocol_state_set(GOWIN_READING_1BYTE);
            StartByte();
        } else if (cmd == READ_BL_CURRENT) {
            GOWIN_Protocol_state_set(GOWIN_READING_2BYTE);
            StartByte();
        } else { // All ACK readback commands
            LOG_DEBUG("GOWIN %s '%c' sent", returnCmdName(cmd), cmd);
            GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_ACK);

            switch (cmd) {
                case SET_STANDBY_ON:
                    Main.Diagnostics.InLowPowerMode = true;
                    break;
                case SET_STANDBY_OFF:
                    Main.Diagnostics.InLowPowerMode = false;
                    break;
                default:
                    break;
            }
        }
    }

    while (!(kUSART_TxFifoEmptyFlag & USART_GetStatusFlags(base))) {
    } // wait until transmission complete
#else
    LOG_ERROR("Mocking failure");
#endif
    return 0;
}

// ------------------------------------------------------------------------------
t_gowin_state GOWIN_Protocol_state_get(void) {
    return GOWIN_DATA.State;
}

// ------------------------------------------------------------------------------
// prefer setting this global variable via this setter
void GOWIN_Protocol_state_set(t_gowin_state state) {
    GOWIN_DATA.State = state;
}

// ------------------------------------------------------------------------------
u8 GOWIN_EdidIndex_get() {
    return EdidIndex;
}

// ------------------------------------------------------------------------------
// GOWIN_Protocol_edid_readback
// Meant for readback of written data as verification possbility
void GOWIN_Protocol_edid_readback(u8 index) {
    if (index > 2) {
        return;
    }
    GOWIN_Queue_Tx_Msg_Index(READ_EDID, index);
}

// ------------------------------------------------------------------------------
// GOWIN_Protocol_edid_write
// Takes place at startup and upon change
void GOWIN_Protocol_edid_write(u8 index) {
    if (index > 2) {
        return;
    }
    GOWIN_Queue_Tx_Msg_Index(WRITE_EDID, index);
}

// ------------------------------------------------------------------------------
u8 GOWIN_DpcdIndex_get() {
    return DpcdIndex;
}

// ------------------------------------------------------------------------------
// GOWIN_Protocol_dpcd_readback
// Meant for readback of written data as verification
void GOWIN_Protocol_dpcd_readback(u8 index) {
    if (index > 2) {
        return;
    }
    GOWIN_Queue_Tx_Msg_Index(READ_DPCD, index);
}

// ------------------------------------------------------------------------------
// GOWIN_Protocol_dpcd_write
void GOWIN_Protocol_dpcd_write(u8 index) {
    if (index > 2) {
        return;
    }
    GOWIN_Queue_Tx_Msg_Index(WRITE_DPCD, index);
}

// ------------------------------------------------------------------------------
// Send the initial EDID settings to Gowin
void GOWIN_Edid_Initialize(u8 verify) {
    // !< send initial edid/dpcd values to Gowin
    GOWIN_Protocol_edid_write(0); // transmit from index0
    GOWIN_Protocol_dpcd_write(0); // transmit from index0
    GOWIN_DATA.verifyReadBack = verify;
    if (verify == false)
        return;
    GOWIN_Protocol_edid_readback(2); // readback to index2
    GOWIN_Protocol_dpcd_readback(2); // readback to index2
}

// ------------------------------------------------------------------------------
// GOWIN_Compare_EdidDpcd
unsigned int GOWIN_Compare_EdidDpcd(u8 * block1, u8 * block2, const char * logtxt) {
    unsigned errorCnt = 0;

    for (unsigned i = 0u; i < EDID_SIZE; i++) {
        if (*(block1 + i) != *(block2 + i)) {
            LOG_ERROR("edid mismatch detected in readback at pos %d (%x<>%x)", i, *(block1 + i), *(block2 + i));
            errorCnt++;
            if (errorCnt > 3)
                break;
        }
    }
    if (errorCnt == 0)
        LOG_INFO("%s verified OK", logtxt);

    return errorCnt;
}

// ------------------------------------------------------------------------------
void GOWIN_Print_edid_in_log(unsigned index) {
    u8 * data;
    int nr_of_blocks = 2;

    switch (index) {
        case 0:
            data = Main.Edid.Edid1;
            break;
        case 1:
            data = Main.Edid.Edid2;
            break;
        case 2:
            data = Main.Edid.Edid3;
            break;
        case 3: // DPCD holds only 128 bytes (but we receive 256)
            data = Main.Dpcd.Dpcd1;
            nr_of_blocks = 1;
            break;
        case 4:
            data = Main.Dpcd.Dpcd2;
            nr_of_blocks = 1;
            break;
        case 5:
            data = Main.Dpcd.Dpcd3;
            nr_of_blocks = 1;
            break;
        default:
            LOG_WARN("...default state selected...");
            data = Main.Edid.Edid1;
            return;
    }

    (index <= 2) ? LOG_RAW("Edid %d:", index) : LOG_RAW("Dpcd %d:", index - 3);
    for (int j = 0; j != 8 * nr_of_blocks; j++) {
        if (j == 8)
            LOG_RAW("\n");
        char buf[128], * pos = buf;
        for (int i = 0; i != 16; i++) {
            pos += sprintf(pos, "%02x ", *data);
            data++;
        }
        LOG_RAW("\t%s", buf);
    }
}