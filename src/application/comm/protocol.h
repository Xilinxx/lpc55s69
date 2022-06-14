#ifndef __COMM_PROTOCOL_H__
#define __COMM_PROTOCOL_H__

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#undef EXTERN
#ifdef _COMM_PROTOCOL_C_
  #define EXTERN
#else
  #define EXTERN         extern
#endif

#define MCU_RXBUF_SIZE   550 // size of uart1 circular buffer
#define GOWIN_RXBUF_SIZE 300 // size of uart2 circular buffer

typedef struct {
    u8 * RxBuf;   // uart buffer base pointer
    u16 RxBufSize; // uart buffer size
    u8 * RxBufWr; // write pointer
    u8 * RxBufRd; // read pointer
    uint32_t flags; // uart flags
} t_uart_data_raw;

#ifdef BOARD_CPU2_FLEXCOMM_IRQ // gaia300d
  #define NUMBER_OF_PROTOCOL_UARTS 3     // Zynq Main CPU and Gowin FPGA & 2nd CPU
#else // zeus300s
  #define NUMBER_OF_PROTOCOL_UARTS 2     // Zynq Main CPU and Gowin FPGA
#endif

// uart channel definitions
typedef enum uart_channel {
    UART1, // Zynq Main CPU
    UART2, // Gowin FPGA
    UART3  // Zynq Secondary CPU
} t_uart_channel;

#define COMM_START_BYTE           0xFE
#define COMM_STOP_BYTE            0xFF
#define COMM_ESCAPE               0x80
/***  ESCAPE SEQUENCE  *********************************************************
*  Any command byte, data byte or checksum byte that equals 0x80, 0xFE or 0xFF
*  has to be converted:
*  transmission:
*  - instead of 0x80, send 0x80 followed by 0x00
*  - instead of 0xFE, send 0x80 followed by 0x7E
*  - instead of 0xFF, send 0x80 followed by 0x7F
*  reception:
*  - replace 0x80 followed by 0x00 with 0x80
*  - replace 0x80 followed by 0x7E with 0xFE
*  - replace 0x80 followed by 0x7F with 0xFF
*******************************************************************************/
/***
 ***   [ START-BYTE | CMD | DATA[0..] | CRC | STOP-BYTE ]
 ***/
// predefined offsets in the receiving buffer
#define PROTOCOL_RX_OFFSET_CMD    1 // Command byte
#define PROTOCOL_RX_OFFSET_ADR    2 // DATA[0]-byte
#define PROTOCOL_RX_OFFSET_OFFSET 3 // DATA[1]-byte , applicable for ARRAY read
#define PROTOCOL_RX_OFFSET_LENGTH 4 // DATA[2]-byte , applicable for ARRAY read

// GENERAL NACK is replied as Command byte = 0
// NACK 1st byte reply is fix
#define COMM_CMD_NACK             0
// NACK 2nd byte faultcode
#define COMM_NACK_UNKNOWN_CMD     0
#define COMM_NACK_MSG_LENGTH      1
#define COMM_NACK_CRC             2
#define COMM_NACK_OUTOFBOUNDARY   3
// nack command length
#define COMM_NACK_TX_LENGTH       3
// 4byte/integer reply length
#define COMM_4BYTE_TX_LENGTH      6
// Specific NACK reply for array write (as DATA byte)
#define COMM_REPLY_NACK           0
#define COMM_REPLY_ACK            1

#define COMM_WAIT_FOR_START       0
#define COMM_WAIT_FOR_STOP        1

typedef enum {
    NO_ERROR  = 0,
    NEW_MSG   = 1,
    ERROR     = -1,
    CRC_ERROR = -2
} t_comm_protocol_return_value;

// one message at a time limit
#define COMM_MAX_NO_MSG 1

// comm message buffers
#define MSGBUF_SIZE     MCU_RXBUF_SIZE // size of uartx message buffer that holds the decoded messages


/*******************************************************************************
 * Variables
 ******************************************************************************/
typedef struct {
    u8 * MsgBuf;       // pointer to buffer where the decoded message will be stored
    u8 * MsgBufWr;      // write pointer
    u16 MsgBufSize;    // length of the received message
    u8 MsgCount;       // number of messages, we only support one message, so this is in fact a flag.
    u16 MsgLength;     // length of the received message
    u16 BadMsgCount;   // keep track of bad messages (debug only)
    u32 GoodMsgCount;  // keep track of good messages (debug only)
    u8 State;          // protocol handler machine state
    u8 checksum;       // internal - checksum
    u8 offset;         // internal - offset
    u8 previous_char;  // internal - previous character
    u8 WaitForReply;   // flag to indicate we are waiting for a reply
    u8 ReplyCount;     // keep track of the number of replies
    u8 RetryCounter;   // keep track of the number of retries
    u16 TimeoutCount;  // reply timeout counter
    u8 RxBusy;         // busy flag
} t_comm_data;


extern t_comm_data COMM_DATA[NUMBER_OF_PROTOCOL_UARTS];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*
 * @brief COMM_Protocol processor
 *
 * @param uart_channel ( UART1 -> Main CPU)
 */
t_comm_protocol_return_value COMM_Protocol(t_uart_channel uart_channel);


void PROTO_TX_SendMsg(t_uart_channel uart_channel, u8 * data, u16 length);

#ifdef UNIT_TEST
EXTERN u8 unitTest_SendBuf[MCU_RXBUF_SIZE];
#endif

#endif  // __COMM_PROTOCOL_H__
