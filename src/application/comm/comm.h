#ifndef _COMM_H_
#define _COMM_H_
/* comm.h
**
** Defines and function declarations for comm.c
**
** (C) Barco
**
******************************************************************************/
#include "protocol.h"
#include <stdbool.h>

#undef EXTERN
#ifdef _COMM_C_
  #define EXTERN
#else
  #define EXTERN extern
#endif

/*******************************************************************************
 * Definitions
 *******************************************************************************
 * https://wiki.barco.com/display/p900/Platform+300+-+GPmcu+to+Maincpu+communication
 *
 * 3 Basic command types : Byte , Integer , Array
 */
#define CMD_READ_BYTE          0x10
#define CMD_WRITE_BYTE         0x11
#define CMD_READ_4BYTE         0x14
#define CMD_WRITE_4BYTE        0x15
#define CMD_READ_ARRAY         0x18
#define CMD_WRITE_ARRAY        0x19

// Byte  subcommand identifiers - See wiki
// 4Byte subcommand identifiers - See wiki
// Array subcommand identifiers
#define CMD_ID_GITBOOTVER      0x00
#define CMD_ID_GITAPPVER       0x01
#define CMD_ID_EDID1           0x21
#define CMD_ID_EDID2           0x22
#define CMD_ID_EDID3           0x23
#define CMD_ID_DPCD1           0x24
#define CMD_ID_DPCD2           0x25
#define CMD_ID_DPCD3           0x26
#define CMD_ID_RESERVED        0x80 // don't use reserved characters
#define CMD_ID_BOOTLOG         0xA0


// error values communication
#define ERR_NO_ERROR  0
#define ERR_ERROR    -1

/*******************************************************************************
 * variables
 ******************************************************************************/
EXTERN struct ssram_data_t *SharedRamStruct_pointer();

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
extern void ResetISR(void);

void comm_handler (void);
void comm_handler_gowin(void);

bool sharedram_bootloader_request_write(u8 request);
u8   sharedram_bootloader_request_read();

void reply_invalid(const u8 nack_type);
void sharedram_log_read(u8 *RxBuf, u8 Offset, u8 NrOfBytes);

#endif //_COMM_H_
