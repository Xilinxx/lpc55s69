/*********************** (C) COPYRIGHT BARCO 2021 ******************************
* File Name           : comm_run.h
* Author              : Barco
* created             : 25/05/2021
* Description         : CommandHandler header
* History:
* 25/05/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/
#ifndef _COMM_RUN_H_
#define _COMM_RUN_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#undef EXTERN
#ifdef _COMM_RUN_C_
#define EXTERN
#else
#define EXTERN extern
#endif

#include "data_map.h"

/*******************************************************************************
 * variables
 ******************************************************************************/
typedef struct {
    u8 cmd;
    u16 byMsgLength;
    void (* Action)(u8 * Rxdata,
                    u16 FrontEndMsgLength);
} tCommand;

EXTERN const tCommand Command[];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
// internals/externals
EXTERN void Run_CommHandler(u8 * Rxdata,
                            u16 FrontEndMsgLength);

u8 byteReadbyId(u8 Identifier);
bool byteWritebyId(u8 Identifier,
                   u8 data);
u16 byteWrite2SPI(u8 * data,
                  u16 size);

// internal only
void cByteRead(u8 * RxBuf,
               u16 RxMsgLength);
void cByteWrite(u8 * RxBuf,
                u16 RxMsgLength);

void c4ByteRead(u8 * RxBuf,
                u16 RxMsgLength);
void c4ByteWrite(u8 * RxBuf,
                 u16 RxMsgLength);

void cArrayRead(u8 * RxBuf,
                u16 RxMsgLength);
void cArrayWrite(u8 * RxBuf,
                 u16 RxMsgLength);

void ReadCommandHandler(u16 Identifier,
                        u16 Offset,
                        u16 Length);
bool WriteCommandHandler(u16 Identifier,
                         u16 Offset,
                         u16 Length);

#endif /* _COMM_RUN_H_ */
