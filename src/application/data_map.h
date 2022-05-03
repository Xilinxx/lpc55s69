#ifndef _DATA_MAP_H_
#define _DATA_MAP_H_

#include "bootloader.h"

#define READ                  0x00u
#define WRITE                 0x01u

#define IDENTIFICATION_SIZE   0x10
#define SPIFLASHID_SIZE       4
#define MESSAGELIST_SIZE      10
#define GITVERSION_SIZE       25
#define EDID_SIZE             256
#define SPIFLASHPAGE_SIZE     256
#define EEPROM_SIZE           24

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;

/* Board Identification */
typedef struct __attribute__((packed))
{
  u8 bootloader[3];  // major,minor,build
  u8 application[3]; // major,minor,build
  u8 gitbootloader[GITVERSION_SIZE];
  u8 gitapplication[GITVERSION_SIZE];
  u8 hwid;
}VersionInfo_t;

/* Board Identification */
typedef struct __attribute__((packed))
{
  u8 identification[IDENTIFICATION_SIZE];
  u8 SpiFlash_Identification[SPIFLASHID_SIZE];
} BoardIdentification_t;

/* MessageList definition */
typedef struct __attribute__((packed)) // pack to 1-byte structure!
{
  u8 MessageCodeMsb;
  u8 MessageCodeLsb;
}MessageList_t;

/* Debug definition */
typedef struct __attribute__((packed)) // pack to 1-byte structure!
{
  u8 byteTestRegister;
  u32 intTestRegister;
  u8 SpiFlashPage[SPIFLASHPAGE_SIZE];
  u8 eeprom[EEPROM_SIZE];
  u8 eeprom_address_cnt; // auto increment for reading eeprom 24c02 emulation
} Debug_t;

/* Diagnostics definition */
typedef struct __attribute__((packed)) // pack to 1-byte structure!
{
  u8 NumberOfMessages;
  MessageList_t MessageList[MESSAGELIST_SIZE];
  bool WatchDogRunout; // Trigger wdog reset by not refreshing
  bool WatchDogDisabled; // for gdb purpose we might want to disable wdog
  bool ReconfigureGowin;
  bool InLowPowerMode;  // indicates the MainCpu is turned off
  uint8_t PendingGowinCmd; // pass through from Main to GowinProtocol -> wakeup
} Diagnostics_t;

/* DeviceState definition */
typedef struct __attribute__((packed)) // pack to 1-byte structure!
{
  u8 State;
  u32 ErrorCode;
  u16 PanelCurrent;
  u8 EthernetStatusBits; //2bit per channel = 4 bits Connected&signalPresent
} DeviceState_t;

// copy from src/drivers/interfaces/logger_mem.c
typedef struct __attribute__((packed))  {
  u32 *	mem_base;       //!< Base address of Shared memory
  u32 *	mem_start;      //!< Start of logging region (after config struct)
  u32 *	mem_end;        //!< End of shared memory
  u32 *	curr_offset;    //!< Current log pointer
  u32	  count;          //!< Number of messages logged
}log_mem_ctxt_t;

/* Edid definition */
typedef struct __attribute__((packed)) // pack to 1-byte structure!
{
  u8 Edid1[EDID_SIZE];
  u8 Edid2[EDID_SIZE];
  u8 Edid3[EDID_SIZE];
}Edid_t;

/* Dpcd definition */
typedef struct __attribute__((packed)) // pack to 1-byte structure!
{
  u8 Dpcd1[EDID_SIZE];
  u8 Dpcd2[EDID_SIZE];
  u8 Dpcd3[EDID_SIZE];
}Dpcd_t;

/* MainMcuRegisterStructure definition */
struct bootloader_ctxt_t; //type defined in bootloader.h
typedef struct
{
  Debug_t Debug __attribute__((aligned));
  VersionInfo_t VersionInfo __attribute__((aligned));
  BoardIdentification_t BoardIdentification __attribute__((aligned));
  DeviceState_t DeviceState __attribute__((aligned));
  Diagnostics_t Diagnostics __attribute__((aligned));
  bootloader_ctxt_t PartitionInfo __attribute__((aligned));
  Edid_t Edid __attribute__((aligned));
  Dpcd_t Dpcd __attribute__((aligned));
  log_mem_ctxt_t sramLogData __attribute__((aligned));
}MainMcuRegisterStructure_t;

/* Address identifier definition */
typedef enum AddressIdentifier
{
  VersionInfo,
  BoardIdentification,
  DeviceState,
  Diagnostics,
  // As from here we add our own custom identifiers
  PartitionInfo,
  Edid,
  Dpcd,
  DebugLog, // SharedRam with boot-code
  // add here
  NumberOfIdentifiers
}AddressIdentifier_t;

/* Forward declarations */
extern MainMcuRegisterStructure_t Main;
extern const u8* IdentifierInternalAddress[];
extern const u8  IdentifierAccessType[];
extern const u32 IdentifierMaxLength[];

//Forward declarations of bootloader_helpers.c
struct storage_driver_t;
extern int  _bootloader_retrieve_ctxt(struct bootloader_ctxt_t *bctxt, struct storage_driver_t *sdriver);
extern int  _bootloader_store_ctxt(struct bootloader_ctxt_t *bctxt, struct storage_driver_t *sdriver);
extern void _bootloader_swap_partitions(struct bootloader_ctxt_t *bctxt);

/**
 * @brief  initialize_global_data_map
 *         The global data structure initialisation
 * @returns void
 */
extern void initialize_global_data_map();
extern app_partition_t switch_boot_partition();
extern void store_edid_data(u8 index, u8 *data);
extern void store_dpcd_data(u8 index, u8 *data);

#endif /* _DATA_MAP_H_ */