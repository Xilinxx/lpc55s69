
/*********************** (C) COPYRIGHT BARCO 2021 ******************************
* File Name           : spi_master.c
* Author              : Barco
* created             : 13/12/2021
* Description         : spi
*   - Spi communication is meant to program/verify the SPI Flash holding the
*     Gowin bitfile.
*   - Communication is Blocking (not IRQ based - to do)
* History:
* 13/12/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/

#define _SPI_MASTER_C_

#include <stdio.h> // for sprintf

// run code includes
#include "spi_master.h"
#include "data_map.h"
#include "is25xp.h"

#ifndef UNIT_TEST
  #include "fsl_spi.h"
  #include <board.h>
  #include <pin_mux.h>
  #include "peripherals.h"
#endif

#include "logger.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define SPI_MASTER            SPI8
#define SPI_MASTER_IRQ        LSPI_HS_IRQn // = FLEXCOMM8_IRQn
#define SPI_MASTER_CLK_SRC    kCLOCK_Flexcomm8
#define SPI_MASTER_CLK_FREQ   CLOCK_GetHsLspiClkFreq()
#define SPI_SSEL              0
#define SPI_MASTER_IRQHandler FLEXCOMM8_IRQHandler
#define SPI_SPOL              kSPI_SpolActiveAllLow
// #define SPI_BAUDRATE        500000U // uncomment to slow down to 5MHz

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static volatile u8 MsgIndex = 0;

static volatile bool SpiMuxSelected = 0;

static t_spi_msg SPI_MSG_QUEUE[SPI_MSG_QUEUE_LENGTH];


/*******************************************************************************
 * Code
 ******************************************************************************/

/*  Code prepared for IRQ based communication, we'd need to supply this struct to is25xp... TO DO
 *
 * static SpiBuffer_t SPIBuffer={{}, {}, 0, 0, true}; // IRQ based buffer
 *
 * void FLEXCOMM8_IRQHandler(void)
 * {
 * // Check for oversize
 * if(SPIBuffer.rxIndex>BUFFER_SIZE)
 *  SPIBuffer.rxIndex = BUFFER_SIZE;
 * if(SPIBuffer.txIndex>BUFFER_SIZE)
 *  SPIBuffer.txIndex = BUFFER_SIZE;
 *
 * if ((SPI_GetStatusFlags(SPI_MASTER) & kSPI_RxNotEmptyFlag) && (SPIBuffer.rxIndex != 0))
 * {
 *  SPIBuffer.rxIndex--;
 *  SPIBuffer.rxBuff[SPIBuffer.rxIndex] = (u8) SPI_ReadData(SPI_MASTER);
 *  LOG_DEBUG("SPI_ReadData 0x%02X index=%d", SPIBuffer.rxBuff[SPIBuffer.rxIndex], SPIBuffer.rxIndex);
 * }
 *
 * if ((SPI_GetStatusFlags(SPI_MASTER) & kSPI_TxNotFullFlag) && (SPIBuffer.txIndex != 0))
 * {
 *    SPIBuffer.txIndex--;
 *    if (SPIBuffer.txIndex == 0)
 *    {
 *        if(SPIBuffer.txBuff[SPIBuffer.txIndex]!=IS25_DUMMY)
 *          LOG_DEBUG("SPI_WriteData last byte , 0x%02X",SPIBuffer.txBuff[SPIBuffer.txIndex]);
 *        // need to disable interrupts before write last data
 *        SPI_DisableInterrupts(SPI8, kSPI_TxLvlIrq);
 *        // param: kSPI_FrameAssert will take CS down
 *        SPI_WriteData(SPI_MASTER, SPIBuffer.txBuff[SPIBuffer.txIndex], kSPI_FrameAssert);
 *    }
 *    else
 *    {
 *        SPI_WriteData(SPI_MASTER, SPIBuffer.txBuff[SPIBuffer.txIndex], 0);
 *        if (SPIBuffer.txBuff[SPIBuffer.txIndex]!=IS25_DUMMY)
 *          LOG_DEBUG("SPI_WriteData, %02X",SPIBuffer.txBuff[SPIBuffer.txIndex]);
 *    }
 * }
 *
 * if ((SPIBuffer.txIndex == 0U) && (SPIBuffer.rxIndex == 0U))
 * {
 *    LOG_DEBUG("masterFinished");
 *    SPIBuffer.masterFinished = true;
 *    SPI_DisableInterrupts(SPI_MASTER, kSPI_RxLvlIrq);
 * }
 * SDK_ISR_EXIT_BARRIER;
 * }
 *
 *
 * void masterCallback(SPI_Type *base, spi_master_handle_t *masterHandle, status_t status, void *userData)
 * {
 *  masterFinished = true;
 * }
 */

/*!
 * @brief spi_master_setup()
 */
void spi_master_setup() {
    /* SPI init */
    uint32_t srcClock_Hz = 0U;
    spi_master_config_t masterConfig;

    srcClock_Hz = SPI_MASTER_CLK_FREQ;

    SPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.sselNum = (spi_ssel_t)SPI_SSEL;
    masterConfig.sselPol = (spi_spol_t)SPI_SPOL;
#if defined(SPI_BAUDRATE)
    masterConfig.baudRate_Bps = SPI_BAUDRATE;
#else
    masterConfig.baudRate_Bps = 25000000U;
#endif
    SPI_MasterInit(SPI_MASTER, &masterConfig, srcClock_Hz);

    BOARD_SetSPIMux(1);

    LOG_DEBUG("spi_master_setup on flexcomm8");

    for (int i = 0; i < SPI_MSG_QUEUE_LENGTH; i++) {
        SPI_MSG_QUEUE[i].cmd = SPI_NO_CMD;
        SPI_MSG_QUEUE[i].param[0] = 0;
        SPI_MSG_QUEUE[i].param[1] = 0;
        SPI_MSG_QUEUE[i].param[2] = 0;
    }

    // initialisation of SPI Flash struct
    is25xp_readid(Main.BoardIdentification.SpiFlash_Identification, 1);

    BOARD_SetSPIMux(0);
}

/*!
 * @brief spi_read_page()
 * Helper function for reading back data inside the SPI Flash with printout
 */
void spi_read_page(u16 startpage, bool verbose) {
    int ret = is25xp_bread(startpage, 1, Main.Debug.SpiFlashPage, verbose);

    if (!verbose)
        return;

    LOG_DEBUG("is25xp_bread startblock: %04lx returned [%d]", (long)startpage, ret);

    // print the read buffer = 1page
    u8 * data;
    data = Main.Debug.SpiFlashPage;
    for (int j = 0; j <= (8 * 2) - 1; j++) {
        char buf[128], * pos = buf;
        for (int i = 0; i != 16; i++) {
            pos += sprintf(pos, "%02x ", *data);
            data++;
        }
        LOG_RAW("\t%04X : %s", j * 0x10, buf);
    }
}

/*!
 * @brief spi_master_update()
 */
void spi_master_update() {
    if (spi_queue_size(false) > 0) {
        BOARD_SetSPIMux(1);
        SpiMuxSelected = true;
        spi_queue_process();
    } else {
        if (SpiMuxSelected) {
            BOARD_SetSPIMux(0);
            SpiMuxSelected = false;
            // LOG_DEBUG("SPI Mux off");
        }
    }
}

/*!
 * @brief spi_master_disable
 */
void spi_master_disable() {
    LOG_DEBUG("spi_master_setup on flexcomm8 disabled");
    /* Enable interrupt, first enable slave and then master. */
    DisableIRQ(SPI_MASTER_IRQ);
    SPI_DisableInterrupts(SPI_MASTER, kSPI_TxLvlIrq | kSPI_RxLvlIrq);
    BOARD_SetSPIMux(0);
}

const char * returnSpiCmdName(t_spi_command val) {
    switch (val) {
        case SPI_NO_CMD:
            return "NO_CMD";
        case SPI_IS25_PP:
            return "IS25_PageProgram";
        case SPI_IS25_RDSR:
            return "IS25_ReadStatusReg";
        case SPI_IS25_READ:
            return "IS25_READ";
        case SPI_IS25_CER:
            return "IS25_ChipErase";
        case SPI_IS25_RDID:
            return "IS25_ReadId";
        case SPI_IS25_SE:
        case SPI_IS25_SE_:
            return "IS25_SectorErase";
        case SPI_IS25_BER32:
            return "IS25_BlockErase 32k";
        case SPI_IS25_BER64:
            return "IS25_BlockErase 64k";
        case SPI_IS25_DP:
            return "IS25_DeepPowerDown";
        case SPI_IS25_RDP:
            return "IS25_ReleasePowerdown";
        case SPI_IS25_VERBOSE:
            return "IS25_Verbosity";

        default:
            return "";
    }
}

/*!
 * @brief invalid_spi_cmd
 * @param cmd command
 */
bool invalid_spi_cmd(t_spi_command cmd) {
    switch (cmd) {
        case SPI_IS25_PP:
        case SPI_IS25_RDSR:
        case SPI_IS25_READ:
        case SPI_IS25_CER:
        case SPI_IS25_RDID:
        case SPI_IS25_SE:
        case SPI_IS25_SE_:
        case SPI_IS25_BER32:
        case SPI_IS25_BER64:
        case SPI_IS25_DP:
        case SPI_IS25_RDP:
        case SPI_IS25_VERBOSE:
            return false;

        default:
            LOG_ERROR("[%02X]is not an SPI command", cmd);
            return true;
    }
}

/*!
 * @brief spi_queue_msg
 *
 * @param data data pointer
 *             The data pointer first 3 bytes are cmd + 2 data bytes
 * @param size size of the data
 */
void spi_queue_msg_param(u8 * data, u16 size) {
    if (invalid_spi_cmd(data[0]))
        return;

    if (spi_queue_size(false) >= SPI_MSG_QUEUE_LENGTH) {
        LOG_WARN("SPI QUEUE is full");
        spi_queue_size(true);
        return;
    }

    u8 newId = MsgIndex;
    for (int i = 0u; i < SPI_MSG_QUEUE_LENGTH; i++) {
        if (SPI_MSG_QUEUE[i].cmd == SPI_NO_CMD) {
            newId = (u8)i;
            break;
        }
    }

    int i = 0; // data iterator
    SPI_MSG_QUEUE[newId].cmd = data[i++];
    for (int j = 0u; i < size; i++) {
        if (data[0] == SPI_IS25_PP) {
            if (i < 3) // u16 address to program
                SPI_MSG_QUEUE[newId].param[i - 1] = data[i];
            else
                SPI_MSG_QUEUE[newId].data[j++] = data[i];
        } else {
            // LOG_DEBUG("adding param %d[%02X]",i-1,data[i]);
            SPI_MSG_QUEUE[newId].param[i - 1] = data[i];
        }
    }

    // LOG_DEBUG("SPI_Queue_Msg[%s] on index[%d]", returnSpiCmdName(data[0]), newId);
}

/*!
 * @brief spi_queue_msg(t_spi_command cmd)
 */
void spi_queue_msg(t_spi_command cmd) {
    if (invalid_spi_cmd(cmd))
        return;

    if (spi_queue_size(false) >= SPI_MSG_QUEUE_LENGTH) {
        LOG_WARN("SPI QUEUE is full");
        return;
    }

    u8 newId = MsgIndex;
    for (int i = 0u; i < SPI_MSG_QUEUE_LENGTH; i++) {
        if (SPI_MSG_QUEUE[i].cmd == SPI_NO_CMD) {
            newId = (u8)i;
            break;
        }
    }

    SPI_MSG_QUEUE[newId].cmd = cmd;

    LOG_DEBUG("SPI_Queue_Msg %s on index %d", returnSpiCmdName(cmd), newId);
}

/*!
 * @brief spi_queue_process
 */
void spi_queue_process() {
    static u8 prevIndex = 0;
    static bool verbose = true;

    // set index to next command , try to maintain order of commands
    if (prevIndex < (SPI_MSG_QUEUE_LENGTH - 1)) {
        if (SPI_MSG_QUEUE[prevIndex + 1].cmd != SPI_NO_CMD)
            MsgIndex = (u8)(prevIndex + 1);
        else
            MsgIndex = 0;
    } else {
        MsgIndex = 0;
    }

    if (MsgIndex == 0) {
        for (int i = 0u; i < SPI_MSG_QUEUE_LENGTH; i++) {
            if (SPI_MSG_QUEUE[i].cmd != SPI_NO_CMD) {
                MsgIndex = (u8)i;
                break;
            }
        }
    }

    prevIndex = MsgIndex;

    if (SPI_MSG_QUEUE[MsgIndex].cmd == SPI_NO_CMD)
        return;

    /*
     * LOG_DEBUG("SPI_Process_Queue index[%d] cmd[%s] param[%02x %02x %02x]", MsgIndex,
     *        returnSpiCmdName(SPI_MSG_QUEUE[MsgIndex].cmd),
     *        SPI_MSG_QUEUE[MsgIndex].param[0],
     *        SPI_MSG_QUEUE[MsgIndex].param[1],
     *        SPI_MSG_QUEUE[MsgIndex].param[2]);
     */

    u16 blockNr = (u16)((SPI_MSG_QUEUE[MsgIndex].param[0] << 8) +
                        SPI_MSG_QUEUE[MsgIndex].param[1]);
    u16 sectorNr = blockNr >> 4; // 1 sector is 4k 0x0..0x1000(12bit)

    switch (SPI_MSG_QUEUE[MsgIndex].cmd) {
        case SPI_NO_CMD:
            break;
        case SPI_IS25_PP:
            is25xp_pagewrite(SPI_MSG_QUEUE[MsgIndex].data, blockNr);
            break;
        case SPI_IS25_RDSR:
            is25xp_read_status_register(verbose);
            break;
        case SPI_IS25_READ: // Page Read verbose
            spi_read_page(blockNr, verbose);
            break;
        // ---------------------- ERASE COMMANDS --------------------------------------
        case SPI_IS25_BER32:
            is25xp_sectorerase(sectorNr, IS25_BE32);
            break;
        case SPI_IS25_BER64:
            is25xp_sectorerase(sectorNr, IS25_BE64);
            break;
        case SPI_IS25_SE: // Sector Erase is per 4K
            is25xp_erase(sectorNr, 1);
            break;
        case SPI_IS25_SE_: // Sector Erase is per 4K, identical commands
            is25xp_sectorerase(sectorNr, IS25_SE_);
            break;
        case SPI_IS25_CER: // to be disabled for safeguarding Flash???
            Main.Diagnostics.WatchDogDisabled = true;
            is25xp_bulkerase();
            Main.Diagnostics.WatchDogDisabled = false;
            break;
        // ----------------------------------------------------------------------------
        case SPI_IS25_RDID:
            is25xp_readid(Main.BoardIdentification.SpiFlash_Identification, 1);
            break;
        case SPI_IS25_RDP:
            is25xp_ReleasePowerDownGetDeviceID();
            break;
        case SPI_IS25_DP:
            is25xp_PowerDown();
            break;
        case SPI_IS25_VERBOSE:
            if (SPI_MSG_QUEUE[MsgIndex].param[0] == 0)
                verbose = false;
            else
                verbose = true;
            LOG_DEBUG("input[%x]->spi verbose is %s", SPI_MSG_QUEUE[MsgIndex].param[0], verbose ? "ON" : "OFF");
            break;
        default:
            break;
    }

    SPI_MSG_QUEUE[MsgIndex].cmd = SPI_NO_CMD;
}

u8 spi_queue_size(bool verbose) {
    u8 pending_msg = 0;

    for (int i = 0u; i < SPI_MSG_QUEUE_LENGTH; i++) {
        if (SPI_MSG_QUEUE[i].cmd != SPI_NO_CMD) {
            if (verbose)
                LOG_DEBUG("Queue item[%d] -> [%s]", i, returnSpiCmdName(SPI_MSG_QUEUE[i].cmd));
            pending_msg++;
        }
    }

    if (pending_msg != 0 && verbose)
        LOG_DEBUG("Queue size is %d", pending_msg);

    if (pending_msg >= SPI_MSG_QUEUE_LENGTH && verbose)
        LOG_WARN("SPI message-Queue is FULL!");

    return pending_msg;
}
