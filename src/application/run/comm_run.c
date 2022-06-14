/*********************** (C) COPYRIGHT BARCO 2021 ******************************
* File Name           : comm_run.c
* Author              : Barco
* created             : 25/05/2021
* Description         : CommandHandler for MainCPU communication
*                       No Gowin Protocol
* History:
* 25/05/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/

#define _COMM_RUN_C_
#include <stdbool.h>
#include <stdlib.h>
#include "string.h"

#include "data_map.h"
#include "logger.h"
#include "comm_run.h"
#include "comm/protocol.h"
#include "comm/comm.h"
#include "comm/gowin_protocol.h"
#include "softversions.h"
#include "spi/spi_master.h"

#ifndef UNIT_TEST
#include <board.h>
#endif

// expected incoming cmd lengths
#define CMD_WRITE_BYTE_LENGTH  4
#define CMD_WRITE_4BYTE_LENGTH 7
#define CMD_READ_ARRAY_LENGTH  5

/*******************************************************************************
 * Variables
 ******************************************************************************/
// -----------------------------------------------------------------------------
// Serial command table
//  Cmd - mandatory command
//  MsgLength - 0: message length won't be checked, >0: message length must fit
//  Function - function that need to be called
//
// Add serial commands here together with the processing method
// ------------------------------------------------------------------------------
const tCommand Command[] = {
    // { Cmd,             MsgLength,                Function }
    { CMD_READ_BYTE,   0,                      cByteRead                          },
    { CMD_WRITE_BYTE,  CMD_WRITE_BYTE_LENGTH,  cByteWrite                         },
    { CMD_READ_4BYTE,  0,                      c4ByteRead                         },
    { CMD_WRITE_4BYTE, CMD_WRITE_4BYTE_LENGTH, c4ByteWrite                        },
    { CMD_READ_ARRAY,  CMD_READ_ARRAY_LENGTH,  cArrayRead                         },
    { CMD_WRITE_ARRAY, 0,                      cArrayWrite                        },
    // keep last entry zero's!
    { 0,               0,                      0                                  }
};

// ! Byte swap unsigned int
uint32_t swap_uint32(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

/*******************************************************************************
 * Code
 ******************************************************************************/
// ------------------------------------------------------------------------------
// Run_CommHandler()
//  called from comm.c
//  Called for user specific commands: uses the command table at the beginning
//  of this file to check the command and to call the corresponding function
//  Gowin commands are not handled here.
// ------------------------------------------------------------------------------
void Run_CommHandler(u8 * Rxdata, u16 MsgLength) {
    u8 i = 0;
    bool found = false;

    LOG_DEBUG("Run_CommHandler");
    while (Command[i].cmd != 0) { // reached end of the command table?
        if (Command[i].cmd == *(Rxdata + PROTOCOL_RX_OFFSET_CMD)) { // compare byte in table with rx
            if ((Command[i].byMsgLength) &&        // need to check??
                (Command[i].byMsgLength != (MsgLength))) { // check if MsgLength is valid
                LOG_DEBUG("Run_CommHandler- MsgLength fault %d<>%d", Command[i].byMsgLength, MsgLength);
                *(Rxdata + PROTOCOL_RX_OFFSET_ADR) = COMM_NACK_MSG_LENGTH;
                found = false;
                break;
            }
            // if yes, call the corresponding f()
            (*Command[i].Action)(Rxdata, MsgLength);
            found = true;
            break;
        }
        i++; // check next table entry
    }

    if (!found) { // return NACK...
        LOG_DEBUG("Run_CommHandler - unknown message 0x%X", *(Rxdata + PROTOCOL_RX_OFFSET_CMD));
        *(Rxdata + PROTOCOL_RX_OFFSET_CMD) = COMM_CMD_NACK;
        PROTO_TX_SendMsg(UART1, Rxdata, COMM_NACK_TX_LENGTH);
    }
}

// ------------------------------------------------------------------------------
// byteReadbyId
// Also used for i2c-slave-communication
// ------------------------------------------------------------------------------
u8 byteReadbyId(u8 Identifier) {
    u8 ret;

    switch (Identifier) {
        case 0 ... 2: // Software version
            ret = Main.VersionInfo.application[Identifier];
            break;

        case 3: // test register
            ret = Main.Debug.byteTestRegister;
            break;

        case 4 ... 6: // Boot Software version
            ret = Main.VersionInfo.application[Identifier - 4];
            break;

        case 7: // hardware id
            ret = Main.VersionInfo.hwid;
            break;

        case 9: // defined boot-partition (-1:None, 0:part0, 1:Part1)
            ret = Main.PartitionInfo.part;
            break;

        // 0x21..0x26 edid/dpcd-cmd handled one level up
        case 0x21 ... 0x26:
            LOG_WARN("ReadByte - edid/dpcd not implemented for i2c");
            break;

        case 0x30: // Readback for the bootlader requested partition to boot (RAM)
            ret = sharedram_bootloader_request_read();
            break;

        case 0x31: // Readback for WatchDogRunout (5s to readback before reboot)
            ret = Main.Diagnostics.WatchDogRunout;
            break;

        case 0x32: // Readback WDOG disable
            ret = Main.Diagnostics.WatchDogDisabled;
            break;

        // 0x40.. reserved for Gowin data
        case 0x41: // Ethernet status register;
            ret = Main.DeviceState.EthernetStatusBits;
            break;

        // 0x50.. SPI_FLASH
        case 0x50 ... 0x53: // SDI Flash - Manufacturer ID
            ret = Main.BoardIdentification.SpiFlash_Identification[Identifier - 0x50];
            break;

        default:
            // default statement - reply 0xff
            ret = 0xff;
            break;
    }
    return ret;
}

// ------------------------------------------------------------------------------
// cByteRead(u8 *RxBuf)
//
//  This f() allows the user to read any variable defined in the 'Main'-struct
//  We don't take endianness into account, this has to be done by the master
//
//  NOTE: not applicable for all overriden
//
//  MsgLength: address and checksum excluded!
//  *RxBuff: Points to the first command byte
// ------------------------------------------------------------------------------
void cByteRead(u8 * RxBuf,
               __attribute__((unused)) u16 RxMsgLength) {
    u8 Identifier;
    bool _ACK = true;

    // fetch our address
    Identifier = RxBuf[PROTOCOL_RX_OFFSET_ADR];
    RxBuf[PROTOCOL_RX_OFFSET_ADR] = byteReadbyId(Identifier);
    if (RxBuf[PROTOCOL_RX_OFFSET_ADR] == 0xff) {
        _ACK = false;
    }

    // reply the 1 byte data
    if (_ACK) {
        PROTO_TX_SendMsg(UART1, RxBuf, COMM_NACK_TX_LENGTH);
    } else {
        PROTO_TX_SendMsg(UART1, RxBuf - 1, 1);
    }
}

// ------------------------------------------------------------------------------
// c4ByteRead(u8 *RxBuf)
//
//  This f() allows the user to read integers defined in the 'Main'-struct
//  We don't take endianness into account, this has to be done by the master
//
//
//  MsgLength: address and checksum excluded!
//  *RxBuff: Points to the first command byte
// ------------------------------------------------------------------------------
void c4ByteRead(u8 * RxBuf,
                __attribute__((unused)) u16 RxMsgLength) {
    bool _ACK = true;
    const u8 SIZE = 4; // integer takes 4 byte
    u32 value = 0;

    // fetch our address/identifier
    u8 Identifier = RxBuf[PROTOCOL_RX_OFFSET_ADR];

    switch (Identifier) {
        case 0 ... 7: // Partition info - start address
            memcpy(&value,
                   IdentifierInternalAddress[PartitionInfo] + RxBuf[PROTOCOL_RX_OFFSET_ADR] * 4,
                   SIZE);
            break;

        case 0x08: // bootloader crc
            value = Main.PartitionInfo.crc;
            break;

        case 0x09: // defined boot-partition (-1:None, 0:part0, 1:Part1)
            // this is actually only 1 byte but kept here for consistency
            value = Main.PartitionInfo.part;
            break;

        case 0x10: // Panel backlight current
            value = Main.DeviceState.PanelCurrent;
            break;

        case 0xAA:
            LOG_DEBUG("Read4Byte - testregister");
            value = Main.Debug.intTestRegister;
            break;

        default:
            _ACK = false;
            break;
    }
    // reply the 4 byte data
    if (_ACK) {
        RxBuf[PROTOCOL_RX_OFFSET_ADR + 0] = (u8)(value >> 24);
        RxBuf[PROTOCOL_RX_OFFSET_ADR + 1] = (u8)(value >> 16);
        RxBuf[PROTOCOL_RX_OFFSET_ADR + 2] = (u8)(value >> 8);
        RxBuf[PROTOCOL_RX_OFFSET_ADR + 3] = (u8)(value);
        PROTO_TX_SendMsg(UART1, RxBuf, COMM_4BYTE_TX_LENGTH);
    } else {
        PROTO_TX_SendMsg(UART1, RxBuf - 1, 1);
    }
}

// ------------------------------------------------------------------------------
// byteWritebyId
// Also used for i2c-slave-communication
// ------------------------------------------------------------------------------
bool byteWritebyId(u8 Identifier, u8 data) {
    switch (Identifier) {
        case 0x03: // test register
            LOG_DEBUG("Write Identifier %d,val:%X", Identifier, data);
            Main.Debug.byteTestRegister = data;
            break;

        // 0x21..0x26 edid/dpcd-cmd handled in comm.c
        case 0x21 ... 0x26:
            LOG_WARN("WriteByte - edid/dpcd not implemented for i2c");
            break;

        case 0x30: // bootloader boot partition info
            return sharedram_bootloader_request_write(data);
            break;
        case 0x31: // reset processor
            LOG_DEBUG("Bootmode request - Trigger watchdog");
            Main.Diagnostics.WatchDogRunout = true; // handled in main-loop
            break;
        case 0x32: // disable/enable watchdog
            if (data == 1) {
                Main.Diagnostics.WatchDogDisabled = true;
                LOG_DEBUG("watchdog disabled");
#ifndef UNIT_TEST
                NVIC_DisableIRQ(WDT_BOD_IRQn);
#endif
            } else {
                Main.Diagnostics.WatchDogDisabled = false;
#ifndef UNIT_TEST
                NVIC_EnableIRQ(WDT_BOD_IRQn);
#endif
            }
            break;

        // 0x40.. Gowin
        case 0x40: // any Gowin cmd
            GOWIN_TX_SendMsg(data);
            break;

        case 0x41: // trigger gowin reconfigure
            Main.Diagnostics.ReconfigureGowin = true;
            break;

        // 0x50.. SPI_FLASH cmds -> byteWrite2SPI()
        case 0x50:
            LOG_ERROR("..should go over byteWrite2SPI()");
            break;

        default:
            // default statement
            return false;
            break;
    }
    return true;
}

/*!
 * @brief byteWrite2SPI
 * All SPI Flash access commands are channeled through here
 * Called from i2c_slave
 */
u16 byteWrite2SPI(u8 * data, u16 size) {
    static u16 remainingbytes = 0;
    static u8 _prevCmd = SPI_NO_CMD;
    static u8 _AddressBuf[4] = { SPI_NO_CMD, 0, 0, 0 };
    const u16 I2C_DATA_LENGTH = 256;
    const u8 MORE_BYTE_MARKER = 0xbe;

    if ((data[0] != 0x50u) && (_prevCmd == SPI_NO_CMD)) { // SPI_FLASH cmd
        LOG_ERROR("..should be 0x50, got [%02X]", data[0]);
        return 0;
    }
/*//for debug purpose :
 * if (size==2)
 *  LOG_INFO("byteWrite2SPI size[%d] remainingbyte[%d] - data[%02X ,%02X]", size, remainingbytes, data[0], data[1]);
 * else
 *  LOG_INFO("byteWrite2SPI size[%d] remainingbyte[%d] - data[%02X ,%02X, %02X]", size, remainingbytes, data[0], data[1], data[2]);
 */
    if (remainingbytes > 0) { // 2nd incoming part
        switch (_prevCmd) {
            case SPI_IS25_READ:
            case SPI_IS25_SE:
            case SPI_IS25_SE_:
            case SPI_IS25_BER32:
            case SPI_IS25_BER64:
            case SPI_IS25_VERBOSE:
                _AddressBuf[1] = data[0];  // msb-adr
                _AddressBuf[2] = data[1];  // lsb-adr
                spi_queue_msg_param(_AddressBuf, 3); // cmd + 2 data bytes
                remainingbytes = 0;
                _prevCmd = SPI_NO_CMD;
                break;
            case SPI_IS25_PP:
                if (_AddressBuf[1] == MORE_BYTE_MARKER && _AddressBuf[2] == MORE_BYTE_MARKER) {
                    // LOG_INFO("SPI_IS25_PP-Part1");
                    _AddressBuf[1] = data[0]; // msb-adr
                    _AddressBuf[2] = data[1]; // lsb-adr
                    remainingbytes = I2C_DATA_LENGTH;
                } else {
                    // we create a new buffer for adding 3 bytes in front...
                    u8 _DataBuf[I2C_DATA_LENGTH + 3]; // Max transfert size of i2c + 3 bytes
                    _DataBuf[0] = _AddressBuf[0];
                    _DataBuf[1] = _AddressBuf[1];
                    _DataBuf[2] = _AddressBuf[2];
                    // LOG_INFO("SPI_IS25_PP-Part2 data[%02X @ %02X, %02X]", _DataBuf[0], _DataBuf[1], _DataBuf[2]);
                    // for (int i = 0; i < I2C_DATA_LENGTH; i++)
                    if (size > I2C_DATA_LENGTH) {
                        LOG_WARN("Oversized data block");
                    }
                    for (int i = 0; i < size; i++) {
                        _DataBuf[i + 3] = data[i];
                    }
                    spi_queue_msg_param(_DataBuf, (u16)(3 + size));
                    _prevCmd = SPI_NO_CMD;
                    remainingbytes = 0;
                }
                break;
            default:
                LOG_WARN("No incoming cmd");
                break;
        }
    } else { // 1st incoming bytes
        switch (data[1]) {
            case SPI_IS25_SE: // Page Erase 4k
            case SPI_IS25_READ: // Page Read
            case SPI_IS25_SE_: // Page Erase 4k
            case SPI_IS25_BER32: // Block Erase 32k
            case SPI_IS25_BER64: // Block Erase 64k
            case SPI_IS25_VERBOSE:// Verbose gets 1 param
                remainingbytes = 2; // address (u16)
                _AddressBuf[0] = data[1];
                _prevCmd = data[1];
                break;
            case SPI_IS25_PP:
                remainingbytes = 2; // PP requires address u16 (and data(u8[256]) part2)
                _AddressBuf[0] = data[1];
                _AddressBuf[1] = MORE_BYTE_MARKER;
                _AddressBuf[2] = MORE_BYTE_MARKER;
                _prevCmd = data[1];
                break;
            default:
                // only a command byte no data
                _AddressBuf[0] = data[1];
                spi_queue_msg_param(_AddressBuf, 1);
                break;
        }
    }

    return remainingbytes;
}

// ------------------------------------------------------------------------------
// cByteWrite(u8 *RxBuf)
// ------------------------------------------------------------------------------
void cByteWrite(u8 * RxBuf,
                __attribute__((unused)) u16 RxMsgLength) {
    bool _ACK = true;

    u8 Identifier = RxBuf[PROTOCOL_RX_OFFSET_ADR];
    u8 data = RxBuf[PROTOCOL_RX_OFFSET_ADR + 1];

    _ACK = byteWritebyId(Identifier, data);

    // overwrite address DATA[0] with ACK/NACK
    if (_ACK)
        RxBuf[PROTOCOL_RX_OFFSET_ADR] = COMM_REPLY_ACK;
    else
        RxBuf[PROTOCOL_RX_OFFSET_ADR] = COMM_REPLY_NACK;

    PROTO_TX_SendMsg(UART1, RxBuf, COMM_NACK_TX_LENGTH);
}

// ------------------------------------------------------------------------------
// cIntegerWrite(u8 *RxBuf)
//
//  This f() allows the user to write any variable defined in the 'Main'-struct
//  We don't take endianness into account, this has to be done by the master
//
//  NOTE: not applicable for all overriden commands in comm.c
//
//  MsgLength: address and checksum excluded!
//  *RxBuff: Points to the first command byte
// ------------------------------------------------------------------------------
void c4ByteWrite(u8 * RxBuf,
                 __attribute__((unused)) u16 RxMsgLength) {
    bool _ACK = true;
    const u8 SIZE = 4;

    u8 Identifier = RxBuf[PROTOCOL_RX_OFFSET_ADR];
    u32 value = 0, swapped_value = 0;

    switch (Identifier) {
        case 0xAA: // test register
            memcpy(&value, RxBuf + 3, SIZE);
            swapped_value = swap_uint32(value);
            LOG_DEBUG("c4ByteWrite - 0xAA test register write 0x%X", swapped_value);
            Main.Debug.intTestRegister = swapped_value;
            break;
        default:
            _ACK = false;
            break;
    }
    // overwrite address DATA[0] with ACK/NACK
    if (_ACK)
        RxBuf[PROTOCOL_RX_OFFSET_ADR] = COMM_REPLY_ACK;
    else
        RxBuf[PROTOCOL_RX_OFFSET_ADR] = COMM_REPLY_NACK;

    PROTO_TX_SendMsg(UART1, RxBuf, COMM_NACK_TX_LENGTH);
}

// ------------------------------------------------------------------------------
// cArrayRead(u8 *RxBuf,u16 RxMsgLength)
//
//  This f() allows the user to read any variable defined in the 'Main'-struct
//  We don't take endianness into account, this has to be done by the master
//
//  NOTE: not applicable for all overriden (fe BOOTLOG)
//
//  MsgLength: address and checksum excluded!
//  *RxBuff: Points to the first command byte
//  Offset is calculted relative to the data_map - Main.<struct>
// ------------------------------------------------------------------------------
void cArrayRead(u8 * RxBuf,
                __attribute__((unused)) u16 RxMsgLength) {
    AddressIdentifier_t Identifier;
    u8 Length, Address;
    unsigned int sizeRestriction, Offset = 0;
    bool _ACK = true;

    // fetch our 6byte header
    Address = RxBuf[PROTOCOL_RX_OFFSET_ADR];  // identifier/address defined on wiki
    Offset = RxBuf[PROTOCOL_RX_OFFSET_ADR + 1]; // offset in data array
    Length = RxBuf[PROTOCOL_RX_OFFSET_ADR + 2]; // requested amount of bytes

    LOG_DEBUG("cArrayRead - Identifier 0x%X, offset %d, length %d",
              Address, Offset, Length);

    switch (Address) {
        case CMD_ID_GITBOOTVER: // Bootloader git version
            Identifier = VersionInfo;
            Offset += sizeof(Main.VersionInfo.bootloader);
            Offset += sizeof(Main.VersionInfo.application);
            sizeRestriction = sizeof(Main.VersionInfo.gitbootloader);
            break;
        case CMD_ID_GITAPPVER: // Application git version
            Identifier = VersionInfo;
            Offset += sizeof(Main.VersionInfo.bootloader);
            Offset += sizeof(Main.VersionInfo.application);
            Offset += sizeof(Main.VersionInfo.gitbootloader);
            sizeRestriction = sizeof(Main.VersionInfo.gitapplication);
            break;
        // --------------------------------------------------------------------------
        case CMD_ID_EDID1: // edid1  , Offset = 0
            Identifier = Edid;
            Offset *= 128;
            sizeRestriction = sizeof(Main.Edid.Edid1);
            break;
        case CMD_ID_EDID2: // edid2
            Identifier = Edid;
            Offset *= 128;
            Offset += sizeof(Main.Edid.Edid1);
            sizeRestriction = sizeof(Main.Edid.Edid2);
            break;
        case CMD_ID_EDID3: // edid2
            Identifier = Edid;
            Offset *= 128;
            Offset += sizeof(Main.Edid.Edid1);
            Offset += sizeof(Main.Edid.Edid2);
            sizeRestriction = sizeof(Main.Edid.Edid3);
            break;
        // --------------------------------------------------------------------------
        case CMD_ID_DPCD1:
            Identifier = Edid;
            Offset *= 128;
            sizeRestriction = sizeof(Main.Dpcd.Dpcd1);
            break;
        case CMD_ID_DPCD2:
            Identifier = Edid;
            Offset *= 128;
            Offset += sizeof(Main.Dpcd.Dpcd1);
            sizeRestriction = sizeof(Main.Dpcd.Dpcd2);
            break;
        case CMD_ID_DPCD3:
            Identifier = Edid;
            Offset *= 128;
            Offset += sizeof(Main.Dpcd.Dpcd1);
            Offset += sizeof(Main.Dpcd.Dpcd2);
            sizeRestriction = sizeof(Main.Dpcd.Dpcd3);
            break;
        // --------------------------------------------------------------------------
        case CMD_ID_BOOTLOG: // BOOTLOG
        // processed in comm.c, does not apply to Main-struct

        default:
            _ACK = false;
            break;
    }

    if (!_ACK) {
        LOG_DEBUG("cArrayRead - unknow identifier");
        RxBuf[PROTOCOL_RX_OFFSET_ADR] = COMM_REPLY_NACK; // overwrite address DATA[0] with 0 = NACK
        PROTO_TX_SendMsg(UART1, RxBuf, COMM_NACK_TX_LENGTH);
        return;
    }

    // length size check
    if (((Offset + Length) <= IdentifierMaxLength[Identifier]) // whole struct
        && (Length <= sizeRestriction)) {                    // item in struct
        // read request does not exceed identifier boundary
        // execute read command handler - some read operations might require an
        // extra step before data can be returned.
        ReadCommandHandler(Identifier, (u16)Offset, Length);
        LOG_DEBUG("Identifier 0x%x, struct offset 0x%x, length %d", Identifier, Offset, Length);

        // copy memory content into RxBuf
        memcpy(RxBuf + 2, IdentifierInternalAddress[Identifier] + Offset, Length);

        PROTO_TX_SendMsg(UART1, RxBuf, (u16)(Length + PROTOCOL_RX_OFFSET_ADR));
    } else {
        LOG_DEBUG("cArrayRead - out of boundary >0x%X >0x%X", sizeRestriction,
                  IdentifierMaxLength[Identifier]);
        // read request exceeds identifier boundary -> echo command without data
        PROTO_TX_SendMsg(UART1, RxBuf, 2);
    }
}

// ------------------------------------------------------------------------------
// cArrayWrite(u8 *RxBuf,u16 RxMsgLength)
//
//  This f() allows the user to write data in the 'Main'-struct
//  We don't take endianness into account, this has to be done by the master
//
//  NOTE: not applicable for all overriden
//
//  MsgLength: address and checksum excluded!
//  *RxBuff: Points to the first command byte
// ------------------------------------------------------------------------------
void cArrayWrite(u8 * RxBuf,
                 __attribute__((unused)) u16 RxMsgLength) {
    AddressIdentifier_t Identifier;
    u8 Length, Address;
    unsigned int sizeRestriction, Offset = 0;
    bool _ACK = true;

    Address = RxBuf[PROTOCOL_RX_OFFSET_ADR];  // identification address from wiki
    Offset = RxBuf[PROTOCOL_RX_OFFSET_ADR + 1]; // offset in data array
    Length = RxBuf[PROTOCOL_RX_OFFSET_ADR + 2]; // following amount of bytes

    LOG_DEBUG("cArrayWrite - Identifier %X, offset %d, length %d", Address, Offset, Length);

    if ((RxMsgLength < CMD_READ_ARRAY_LENGTH) || (RxMsgLength < Length)) {
        if (RxMsgLength < CMD_READ_ARRAY_LENGTH)
            LOG_DEBUG("cArrayWrite - message length %d < %d ", RxMsgLength, CMD_READ_ARRAY_LENGTH);
        else
            LOG_DEBUG("cArrayWrite - message length = %d < %d ", RxMsgLength, Length);

        RxBuf[1] = COMM_CMD_NACK;
        RxBuf[2] = COMM_NACK_MSG_LENGTH; // 2nd byte, error info
        PROTO_TX_SendMsg(UART1, RxBuf, 3);
        return;
    }

    switch (Address) {
        case CMD_ID_EDID1: // edid1
            Identifier = Edid;
            Offset *= 128;
            sizeRestriction = sizeof(Main.Edid.Edid1) - Offset;
            break;
        case CMD_ID_EDID2: // edid2
            Identifier = Edid;
            sizeRestriction = sizeof(Main.Edid.Edid2) - Offset;
            Offset *= 128;
            Offset += sizeof(Main.Edid.Edid1);
            break;
        case CMD_ID_EDID3: // edid3
            Identifier = Edid;
            sizeRestriction = sizeof(Main.Edid.Edid3) - Offset;
            Offset *= 128;
            Offset += sizeof(Main.Edid.Edid1);
            Offset += sizeof(Main.Edid.Edid2);
            break;
        // --------------------------------------------------------------------------
        case CMD_ID_DPCD1: // DPCP1
            Identifier = Dpcd;
            Offset *= 128;
            sizeRestriction = sizeof(Main.Dpcd.Dpcd1) - Offset;
            break;
        case CMD_ID_DPCD2:
            Identifier = Edid;
            sizeRestriction = sizeof(Main.Dpcd.Dpcd2) - Offset;
            Offset *= 128;
            Offset += sizeof(Main.Dpcd.Dpcd1);
            break;
        case CMD_ID_DPCD3:
            Identifier = Edid;
            sizeRestriction = sizeof(Main.Dpcd.Dpcd3) - Offset;
            Offset *= 128;
            Offset += sizeof(Main.Dpcd.Dpcd1);
            Offset += sizeof(Main.Dpcd.Dpcd2);
            break;
        // --------------------------------------------------------------------------
        default:
            _ACK = false;
            break;
    }

    if (!_ACK) {
        LOG_DEBUG("cArrayWrite - unknow identifier %X", Address);
        PROTO_TX_SendMsg(UART1, RxBuf, 2);
        return;
    }

    // length size check
    if (((Offset + Length) <= IdentifierMaxLength[Identifier]) // whole struct
        && (Length <= sizeRestriction)) {                    // item in struct
                                                             // write request does not exceed identifier boundary
        // execute write command handler - some write operations might require an
        // extra step before data can be returned.
        WriteCommandHandler(Identifier, (u16)Offset, Length);
        LOG_DEBUG("Identifier 0x%x, struct offset 0x%X, length %d", Identifier, Offset, Length);
        // for (auto i = 0u; i < 10;i++)
        //   LOG_RAW("Index %d, data 0x%x", i, *(RxBuf + i));

        // copy RxBuf into internal memory
        memcpy((u8 *)IdentifierInternalAddress[Identifier] + Offset, RxBuf + 5, Length);

        RxBuf[PROTOCOL_RX_OFFSET_ADR] = COMM_REPLY_ACK;
        PROTO_TX_SendMsg(UART1, RxBuf, COMM_NACK_TX_LENGTH);
    } else {
        RxBuf[PROTOCOL_RX_OFFSET_ADR] = COMM_REPLY_NACK;
        LOG_DEBUG("cArrayWrite - out of boundary >0x%X >0x%X", sizeRestriction,
                  IdentifierMaxLength[Identifier]);
        // read request exceeds identifier boundary -> echo command without data
        PROTO_TX_SendMsg(UART1, RxBuf, COMM_NACK_TX_LENGTH);
    }
}

// ------------------------------------------------------------------------------
// bool WriteCommandHandler(u16 Identifier, u16 Offset, u16 Length)
//
// Description: This will execute some tasks which might be required after a
//              write command has been received
//
// Parameters:  - Identifier field of received command
//              - Offset field of received command
//              - Length field of received command
// ------------------------------------------------------------------------------

bool WriteCommandHandler(u16 Identifier,
                         __attribute__((unused)) u16 Offset,
                         __attribute__((unused)) u16 Length) {
    bool error;

    error = false; // clear error flag

    LOG_DEBUG("comm_run::WriteCommandHandler");
    // evaluate identifier and offset to find out if we need to trigger some task
    switch (Identifier) {
        case VersionInfo:
        case BoardIdentification:
        case DeviceState:
        case Diagnostics:
            break;

        // all registers have been overwritten with the received content here!
        // we might want to check if the received data/request is valid/allowed
        // (eg. state change request might be invalid!)
        // => on invalid/not allowed request -> return error and
        // the registers will be restored to the original values!

        // trigger configuration task here...

        case PartitionInfo:
            break;

        case DebugLog:
            break;

        default: // no task to be executed/triggered
            break;
    }
    return error;
}

// ------------------------------------------------------------------------------------
// void ReadCommandHandler(u16 Identifier, u16 Offset, u16 Length)
//
// Description: This will execute some tasks which might be required after a read command has been received
//
// Parameters:  - Identifier field of received command
//              - Offset field of received command
//              - Length field of received command
// ------------------------------------------------------------------------------------

void ReadCommandHandler(u16 Identifier,
                        __attribute__((unused)) u16 Offset,
                        __attribute__((unused)) u16 Length) {
    LOG_DEBUG("comm_run::ReadCommandHandler");

    // evaluate identifier and offset to find out if we need to trigger some (sync) task
    switch (Identifier) {
        case VersionInfo:
        case BoardIdentification:
        case DeviceState:
            break;

        case Diagnostics:
            break;

        case PartitionInfo:
            break;

        case DebugLog:
            break;

        default: // no task to be executed/triggered -> data will be read directly from ram
            break;
    }
}