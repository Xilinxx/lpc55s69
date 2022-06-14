#ifndef __GOWIN_PROTOCOL_H__
#define __GOWIN_PROTOCOL_H__

#include "protocol.h"  // for uart channel definition
#include "stdint.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#undef EXTERN
#ifdef _GOWIN_PROTOCOL_C_
  #define EXTERN
#else
  #define EXTERN         extern
#endif

#define GOWIN_START_BYTE 0x24       // '$'
#define GOWIN_STOP_BYTE  0x23       // '#'

/* see wiki https://wiki.barco.com/display/p900/GreenPOWER+FPGA+%28Gowin%29+Design */
typedef enum {
    NO_GOWIN_CMD           = 0x00u,
    // single byte commands
    ACK                    = 0x06u,
    NACK                   = 0x15u,
    // combined with Start and Stop Byte
    // from GPMCU -> FPGA/GOWIN
    WRITE_EDID             = 0x77u, // w
    READ_EDID              = 0x72u, // r
    WRITE_DPCD             = 0x64u, // d
    READ_DPCD              = 0x65u, // e
    READ_ETH_STATUS        = 0x61u, // a
    READ_BL_CURRENT        = 0x62u, // b , Backlight current

    // Output signals
    SET_FPGA_ON            = 0x66u, // f
    SET_FPGA_OFF           = 0x46u, // F
    SET_STANDBY_ON         = 0x73u, // s
    SET_STANDBY_OFF        = 0x53, // S
    PANEL_VOLTAGELO        = 0x56u, // V , 10V panel
    PANEL_VOLTAGEHI        = 0x76u, // v , 12V panel
    BACKLIGHT_OFF          = 0x4cu, // L
    BACKLIGHT_ON           = 0x6cu, // l
    PANEL_PWR_OFF          = 0x50u, // P
    PANEL_PWR_ON           = 0x70u, // p
    HDMI_WRITE_PROTECT_OFF = 0x48u, // H
    HDMI_WRITE_PROTECT_ON  = 0x68u, // h

    // from FPGA/GOWIN -> GPMCU
    WAKE_UP                = 0x25u // '%'
} t_gowin_command;

// state machine codes
typedef enum {
    GOWIN_WAIT_FOR_START = 0,
    GOWIN_WAIT_FOR_STOP,
    GOWIN_WAIT_FOR_EDID,
    GOWIN_WAIT_FOR_DPCD,
    GOWIN_READING_EDID,
    GOWIN_READING_DPCD,
    GOWIN_WAIT_FOR_EDID_WRITE_ACK,
    GOWIN_WAIT_FOR_ACK,
    GOWIN_READING_1BYTE,
    GOWIN_READING_2BYTE
} t_gowin_state;

// return codes
#define GOWIN_RETURN_NONE          0
#define GOWIN_RETURN_NEW_MSG       1
#define GOWIN_RETURN_ERROR         -1
#define GOWIN_RETURN_TIMEOUT       -2
#define GOWIN_RETURN_MAX_MSG       2
#define GOWIN_RETURN_WAIT_EDID_ACK 3
#define GOWIN_RETURN_TX_QUEUE      4


#define GOWIN_MAX_NO_MSG           1
#define GOWIN_MSG_QUEUE_SIZE       10
#define GOWIN_REPLY_TIMEOUT        8000

#define GOWIN_EDID_BYTE_LENGTH     256

#define GOWIN_ACK                  0x06u // command executed successfully
#define GOWIN_NACK                 0x15u // command not executed (NACK)

// GOWIN message buffer
#define FPGA_MSGBUF_SIZE           300 // size of uart1 message buffer that holds the messages

/*******************************************************************************
 * Variables
 ******************************************************************************/

typedef struct {
    t_gowin_command cmd;
    u8 param;
} t_gowin_msg;

EXTERN t_gowin_msg GOWIN_MSG_QUEUE[GOWIN_MSG_QUEUE_SIZE];  // Max 5 pending messages

typedef struct {
    u8 * MsgBuf;        // pointer to buffer where the decoded message will be stored
    u8 * MsgBufWr;      // write pointer
    u16 MsgBufSize;    // length of message buffer
    u8 MsgCount;       // number of messages, we only support one message, so this is in fact a flag.
    u16 MsgLength;     // length of the received message
    u16 BadMsgCount;   // keep track of bad messages (debug only)
    u32 GoodMsgCount;  // keep track of good messages (debug only)
    t_gowin_state State; // protocol handler machine state
    u8 checksum;       // internal - checksum
    u16 offset;        // internal - offset
    u8 previous_char;  // internal - previous character
    u8 WaitForReply;   // flag to indicate we are waiting for a reply
    u8 ReplyCount;     // keep track of the number of replies
    u8 RetryCounter;   // keep track of the number of retries
    u16 TimeoutCount;  // reply timeout counter
    u8 RxBusy;         // busy flag
    u8 verifyReadBack; // verify flag, meant for edid/dpcd-write verification
} t_gowin_data;

EXTERN t_gowin_data GOWIN_DATA;


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

const char * returnCmdName(t_gowin_command val);
const char * returnCodeName(int val);
const char * returnStateName(t_gowin_state val);

/*
 * @brief GOWIN_Protocol processor
 *
 * @param uart_channel ( UART2 -> Gowin FPGA)
 */
EXTERN int8_t GOWIN_Protocol(t_uart_channel uart_channel);
EXTERN int8_t GOWIN_TX_SendMsg(t_gowin_command cmd);

t_gowin_state GOWIN_Protocol_state_get(void);
void GOWIN_Protocol_state_set(t_gowin_state state);

u8   GOWIN_EdidIndex_get();
void GOWIN_Protocol_edid_readback(u8 index);
void GOWIN_Protocol_edid_write(u8 index);

u8   GOWIN_DpcdIndex_get();
void GOWIN_Protocol_dpcd_readback(u8 index);
void GOWIN_Protocol_dpcd_write(u8 index);

void GOWIN_Edid_Initialize(u8 verify);

unsigned int GOWIN_Compare_EdidDpcd(u8 * block1, u8 * block2, const char * logtxt);
void GOWIN_Print_edid_in_log(unsigned index);

u8   GOWIN_QueueIndex_get();
void GOWIN_Queue_Init();
u8   GOWIN_Queue_size();
void GOWIN_Queue_Tx_Msg(t_gowin_command cmd);
void GOWIN_Queue_Tx_Msg_Index(t_gowin_command cmd, u8 storage_index);
void GOWIN_Process_Queue();

#endif  // __GOWIN_PROTOCOL_H__
