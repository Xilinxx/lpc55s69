#define _REGISTERMAP_C_
/*********************** (C) COPYRIGHT BARCO **********************************
* File Name           : data_map.c
* Author              : Barco , David Thio <david.thio@barco.com>
* created             : May 2021
* Description         : Global Data overview
* History:
* 25/05/2021 : introduced in gpmcu code (DAVTH)
*******************************************************************************/
#include <string.h>
#include "logger.h"

#ifndef UNIT_TEST
#include "board.h"
#include "flash_helper.h"
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
/* surpress /home/dev/app/vendor/CMSIS/core_cm33.h:2421:23:
 * warning: cast to pointer from integer of different size [-Wint-to-pointer-cast]
 */
#include "memory_map.h"
#pragma GCC diagnostic pop

#include "softversions.h"
#include "version.h"

#include "data_map.h"

MainMcuRegisterStructure_t Main;

/* Internal register mapping of 'Main' structure and access type
 ---------------------------------------------------------------*/
const u8 *IdentifierInternalAddress[] = {
    (u8 *)&Main.VersionInfo,
    (u8 *)&Main.BoardIdentification,
    (u8 *)&Main.DeviceState,
    (u8 *)&Main.Diagnostics,
    (u8 *)&Main.PartitionInfo,
    (u8 *)&Main.Edid,
    (u8 *)&Main.Dpcd,
    0 //debugLog
};

const u8 IdentifierAccessType[NumberOfIdentifiers] = {
    READ,  //VersionInfo
    READ,  //BoardIdentification
    READ,  //DeviceState
    READ,  //Diagnostics
    READ,  //PartitionInfo
    WRITE, //Edid
    WRITE, //Dpcd
    0      //DebugLog
};

const u32 IdentifierMaxLength[NumberOfIdentifiers] = {
    sizeof(Main.VersionInfo),
    sizeof(Main.BoardIdentification),
    sizeof(Main.DeviceState),
    sizeof(Main.Diagnostics),
    sizeof(Main.PartitionInfo),
    sizeof(Main.Edid),
    sizeof(Main.Dpcd),
    0};

/** DisplayPort Configuration Data **
 * Initialization values for DPCD & EDID ram for Gowin FPGA controlling DISPLAYPORT (not HDMI)
 * copy from https://mstsvn002.barco.com/websvn/filedetails.php?repname=projects&path=%2Ftrunk%2Ffun100%2Fhardware%2Ff200%2Ffpga_10m08dau324c8g_f200%2Fsource%2FNios%2Fsoftware%2Fmainapp%2Fsource%2Fi2c.c
 *
 * The following section describes the DPCD fields for link initialization:
     Receiver Capability Field
     Link Configuration Field
     Link/Sink Status Field
  Note: Gowin FPGA will present this data as requested by the Graphic card per address index!
 * */
static const u8 dpcdInitValues[] =
    {
        /*0x600..0x600 --> 0x0*/ 0x1,
        /*fill @ 0x1*/ 0,
        /*0x2002..0x2003 --> 0x2*/ 0x1, 0x0,
        /*0x1a0..0x1a3 --> 0x4*/ 0x0, 0x0, 0x0, 0x0,
        /*0x1c0..0x1c3 --> 0x8*/ 0x0, 0x0, 0x0, 0x0,
        /*0x200c..0x200f --> 0xc*/ 0x77, 0x77, 0x81, 0x3,
        /*0x200..0x207 --> 0x10*/ 0x1, 0x0, 0x77, 0x77, 0x81, 0x3, 0x0, 0x0,
        /*fill @ 0x18*/ 0, 0, 0, 0, 0, 0, 0, 0,
        /*0x0..0xf --> 0x20*/ 0x12, 0x14, 0xc4, 0x1, 0x1, 0x1, 0x1, 0x80, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        /*0x30..0x3f --> 0x30*/ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        /*0x300..0x30f --> 0x40*/ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        /*0x400..0x40f --> 0x50*/ 0x0, 0x1b, 0xc5, 0x42, 0x49, 0x54, 0x45, 0x43, 0x0, 0x12, 0x1, 0x2, 0x0, 0x0, 0x0, 0x0,
        /*0x100..0x11f --> 0x60*/ 0x6, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

static const u8 edidInitValues[] =
    { // Block 0, Base EDID:
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x08, 0x93, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x0b, 0x01, 0x04, 0xb5, 0x3c, 0x22, 0x82, 0xc2, 0xdf, 0x78, 0xae, 0x4d, 0x44, 0xa7, 0x26,
        0x0a, 0x4e, 0x51, 0x21, 0x08, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x25, 0xcc, 0x00, 0x50, 0xf0, 0x70, 0x3e, 0x80, 0x08, 0x20,
        0x08, 0x0c, 0x54, 0x4f, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x33, 0x38, 0x34,
        0x34, 0x38, 0x32, 0x35, 0x31, 0x31, 0x32, 0x30, 0x32, 0x60, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x4d,
        0x44, 0x41, 0x43, 0x2d, 0x55, 0x31, 0x32, 0x37, 0x45, 0x50, 0x0a, 0x20, 0x00, 0x00, 0x00, 0xfd,
        0x08, 0x0f, 0x91, 0x19, 0x4f, 0x46, 0x01, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x26,
        // Block 1, Video Timing Extension Block:
        0x10, 0x01, 0x04, 0x00, 0x00, 0x25, 0xcc, 0x00, 0x50, 0xf0, 0x70, 0x3e, 0x80, 0x08, 0x20, 0x08,
        0x0c, 0x54, 0x4f, 0x21, 0x00, 0x00, 0x1a, 0x1a, 0x68, 0x00, 0xa0, 0xf0, 0x38, 0x1f, 0x40, 0x30,
        0x20, 0x3a, 0x00, 0x54, 0x4f, 0x21, 0x00, 0x00, 0x7b, 0x1a, 0x36, 0x80, 0xa0, 0x70, 0x38, 0x1f,
        0x40, 0x30, 0x20, 0x35, 0x00, 0x54, 0x4f, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x19, 0x00, 0xa0, 0x50,
        0xd0, 0x15, 0x20, 0x30, 0x20, 0x35, 0x00, 0x54, 0x4f, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0xb5};

static const u8 eepromInitValues[] = {
    // Mac address eth0: 0004A50F1360
    0x30, 0x30, 0x30, 0x34, 0x41, 0x35, 0x30, 0x46, 0x31, 0x33, 0x36, 0x30,
    // Mac address eth1: 0004A50F135F
    0x30, 0x30, 0x30, 0x34, 0x41, 0x35, 0x30, 0x46, 0x31, 0x33, 0x35, 0x46};

static void initializeEdidDpcd(void)
{
  LOG_INFO("Edid size (%d) - Dpcd size (%d)", sizeof(edidInitValues), sizeof(dpcdInitValues));
  for (unsigned i = 0u; i < sizeof(dpcdInitValues); i++)
  {
    Main.Dpcd.Dpcd1[i] = dpcdInitValues[i];
    Main.Dpcd.Dpcd2[i] = 0x00;
    Main.Dpcd.Dpcd3[i] = 0x00;
  }

  for (unsigned i = 0u; i < sizeof(edidInitValues); i++)
  {
    Main.Edid.Edid1[i] = edidInitValues[i];
    Main.Edid.Edid2[i] = 0x00;
    Main.Edid.Edid3[i] = 0x00;
  }
}

void initialize_global_data_map()
{
  Main.Diagnostics.WatchDogDisabled = false;
  Main.Diagnostics.ReconfigureGowin = false;
  Main.Diagnostics.WatchDogRunout = false;
  // SW Version info
  Main.VersionInfo.bootloader[0] = SOFTVERSIONBOOTMAJOR; // Major version boot
  Main.VersionInfo.bootloader[1] = SOFTVERSIONBOOTMINOR; // Minor version boot
  Main.VersionInfo.bootloader[2] = SOFTVERSIONBOOTBUILD; // Minor version boot

  Main.VersionInfo.application[0] = SOFTVERSIONRUNMAJOR; // Major version run
  Main.VersionInfo.application[1] = SOFTVERSIONRUNMINOR; // Minor version run
  Main.VersionInfo.application[2] = SOFTVERSIONRUNBUILD; // Minor version run
  Main.VersionInfo.gitbootloader[0] = '\0';
  Main.VersionInfo.gitapplication[0] = '\0';

  if (sizeof(BOOTLOADER_VERSION_GIT) >= GITVERSION_SIZE)
  {
    strncpy((char *)Main.VersionInfo.gitbootloader, BOOTLOADER_VERSION_GIT,
            GITVERSION_SIZE);
  }
  else
  {
    strncpy((char *)Main.VersionInfo.gitbootloader, BOOTLOADER_VERSION_GIT,
            sizeof(BOOTLOADER_VERSION_GIT));
  }
  Main.VersionInfo.gitbootloader[GITVERSION_SIZE - 1] = '\0';
  if (sizeof(APPLICATION_VERSION_GIT) >= GITVERSION_SIZE)
  {
    strncpy((char *)Main.VersionInfo.gitapplication, APPLICATION_VERSION_GIT,
            GITVERSION_SIZE);
  }
  else
  {
    strncpy((char *)Main.VersionInfo.gitapplication, APPLICATION_VERSION_GIT,
            sizeof(APPLICATION_VERSION_GIT));
  }
  Main.VersionInfo.gitapplication[GITVERSION_SIZE - 1] = '\0';
  LOG_DEBUG("Application Version [%s]", Main.VersionInfo.gitapplication);
  LOG_DEBUG("Boot Version [%s]", Main.VersionInfo.gitbootloader);

  // initialize general device state
  memset((u8 *)&Main.DeviceState, 0x00, sizeof(Main.DeviceState)); // device ready, no error
  Main.DeviceState.PanelCurrent = 0;
  Main.DeviceState.EthernetStatusBits = 0;

  initializeEdidDpcd();

  memset(Main.Debug.eeprom, 0, EEPROM_SIZE);
  memcpy(Main.Debug.eeprom, eepromInitValues, sizeof(eepromInitValues) * sizeof(u8));

#ifndef UNIT_TEST
  Main.VersionInfo.hwid = BOARD_GetRevision(); // from board.h
  // Board Identification
  // Hard-coded Board Identification (board.h)
  u8 _size_2_copy = IDENTIFICATION_SIZE;
  if (BOARD_GetNameSize() < IDENTIFICATION_SIZE)
    _size_2_copy = BOARD_GetNameSize();
  strncpy((char *)Main.BoardIdentification.identification, BOARD_GetName(), _size_2_copy);
  Main.BoardIdentification.identification[_size_2_copy] = '\0';
  for (u8 i = 0u; i < SPIFLASHID_SIZE; i++)
    Main.BoardIdentification.SpiFlash_Identification[i] = 0;

  if (init_flash())
  {
    struct bootloader_ctxt_t bctxt = get_bootloader_context();
    Main.PartitionInfo.part = bctxt.part; // active partition 0 or 1
    LOG_INFO("Boot partition is %d", Main.PartitionInfo.part);
    Main.PartitionInfo.crc = bctxt.crc; // crc of bootcode reserved memory area
    Main.PartitionInfo.apps[0] = bctxt.apps[0];
    Main.PartitionInfo.apps[1] = bctxt.apps[1];
  }

  //Set LOGGING SRAM pointers - predefined shared memory with bootloader
  Main.sramLogData.mem_base = (u32 *)&__ssram_start__;
  Main.sramLogData.mem_start = (u32 *)&__ssram_log_start__;
  Main.sramLogData.mem_end = (u32 *)&__ssram_end__;
  //to do: how to get this aligned with the actual logger ?
  Main.sramLogData.curr_offset = NULL;
  Main.sramLogData.count = 0;
#else
  Main.VersionInfo.hwid = 0;
  Main.PartitionInfo.apps[0].start_addr = 0xaabbccdd; // first u32 of struct
  Main.PartitionInfo.part = APP_PARTITION_NONE;
  //only check crc in unit-tests
  Main.PartitionInfo.crc = 0x01020304;
  Main.PartitionInfo.apps[0].crc = 0x05060708;
  Main.PartitionInfo.apps[1].crc = 0x090a0b0c;

  //protocol_array unit test checks for ordered values
  for (u16 i = 0u; i < sizeof(Main.Edid.Edid1); i++)
  {
    Main.Edid.Edid1[i] = (u8)i;
  }
#endif
}

/** store_edid_data()
 * @brief  Fill in the EDID data in the selected EDID buffer
 *
 * @returns  void
 */
void store_edid_data(u8 index, u8 *data)
{
  //LOG_DEBUG("store_edid_data() 1st byte %x at address 0x%02X", *data, data);
  for (u16 i = 0u; i < EDID_SIZE; i++)
  {
    if (index == 0)
    {
      Main.Edid.Edid1[i] = data[i];
    }
    else if (index == 1)
    {
      Main.Edid.Edid2[i] = data[i];
    }
    else if (index == 2)
    {
      Main.Edid.Edid3[i] = data[i];
    }
    else
    {
      LOG_WARN("data_map - store_edid_data() wrong index %d", index);
      return;
    }
  }
}

/** store_dpcd_data()
 * @brief  Fill in the DPCD data in the selected buffer
 *
 * @returns  void
 */
void store_dpcd_data(u8 index, u8 *data)
{
  //LOG_DEBUG("store_dpcd_data() 1st byte %x at address 0x%02X, index[%d]", *data, data, index);
  for (u16 i = 0u; i < EDID_SIZE; i++)
  {
    if (index == 0)
    {
      Main.Dpcd.Dpcd1[i] = data[i];
    }
    else if (index == 1)
    {
      Main.Dpcd.Dpcd2[i] = data[i];
    }
    else if (index == 2)
    {
      Main.Dpcd.Dpcd3[i] = data[i];
    }
    else
    {
      LOG_WARN("data_map - store_dpcd_data() wrong index %d", index);
      return;
    }
  }
}
