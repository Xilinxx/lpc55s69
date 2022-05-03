/**
 * @file bootloader.c
 * @brief  Main source for bootloader
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-13
 */

#include "crc.h"
#include "board.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "clock_config.h"

#include "bootloader.h"
#include "comm_protocol.h"
#include "si5341.h"
#include "usb5807c.h"
#include "storage.h"
#include "storage_flash.h"
#include "storage_spi_flash.h"
#include "bootloader_helpers.h"

#include "logger.h"
#include "version.h"

#include "memory_map.h"
#include "fsl_gpio.h"
#include "fsl_wwdt.h"

#include <stdint.h>


struct ssram_data_t _gdata __attribute__((section(".ssram_sect")));
static struct bootloader_ctxt_t _bctxt;
static struct bootloader_ctxt_t _bctxt_in_Flash;
static struct bootloader_ctxt_t _bctxt_backup;

extern void ResetISR(void);

// watchdog configuration : Hard Reset is initialized when wdog is triggered
wwdt_config_t wwdt_config;

/**
 * @brief  Setup the board
 *
 * Run the setup of the selected board. BOARD_* functions change according
 * to selected target
 *
 * @returns  -1 if something fails
 */
static int _initialize_board(void)
{
  uint8_t revision = BOARD_GetRevision();

  (void)revision;                 // Probably we'll need to use this at some point

  BOARD_BootInitBootPins();       //!< Init bootloader pins
  BOARD_BootInitClocks();         //!< Initialize Board Clocks
  BOARD_BootInitPeripherals();    //!< Init bootloader peripherals

  if (_gdata.reboot_counter == 0) //to do: check cold restart instead
    wipe_sram(0);
  else
    wipe_sram(sizeof(_gdata));

  SDK_DelayAtLeastUs(500000, 96000000U);

  /* GO GOWIN GO! */
  BOARD_PwrOn_MainCPU(500000);

  logger_init();

  LOG_OK("Board initialized");

  return 0;
}

/**
 * @brief  usb initialisation
 *
 * @returns -1 if failure
 */
static int _initialize_usb(void)
{
  i2c_master_handle_t i2cmh;
  I2C_MasterTransferCreateHandle(BOARD_I2C_USB_PERIPHERAL, &i2cmh, NULL, NULL);

  usb5807c_init_ctxt(BOARD_I2C_USB_PERIPHERAL, &i2cmh);

  LOG_INFO("USB5807C Setting ResetN");
  BOARD_Set_USB_Reset(1);

  SDK_DelayAtLeastUs(100000, 96000000U);
  if (usb5807c_enable_runtime_with_smbus() < 0)
  {
    return -1;
  }
  SDK_DelayAtLeastUs(100000, 96000000U);
  if (usb5807c_set_upstream_port(USB5807C_FLEXPORT1) < 0)
  {
    return -1;
  }
  int id = usb5807c_retrieve_id();
  if (id != -1) {
    LOG_INFO("USB ID: %d", id);
    LOG_INFO("USB VID: %x", usb5807c_retrieve_usb_vid());

    int rev = usb5807c_retrieve_revision();
    LOG_INFO("USB Revision: 0x%x", rev);
  }
  else {
    LOG_ERROR("USB ID: %d", id);
    LOG_ERROR("USB VID: %x", usb5807c_retrieve_usb_vid());
    return -1;
  }
  // Clock generator
  si5341_init_ctxt(FLEXCOMM5_PERIPHERAL, &i2cmh);
  return si5341_dump_initial_regmap();
}

/**
 * @brief   initialisation/verification bootinfo
 *
 * @returns  bool 1 if we need to stay in bootloader
 */
static bool _initialize_bootinfo(struct storage_driver_t *sdriver,
                                 struct flash_area_t *bootinfo)
{
  bool force_bootloader = false;
  bool empty_partition = false;
  struct flash_area_t bootinfob = storage_new_flash_area(FLASH_HELPER(bootinfob));

  // the bootinfo
  bool bootinfo_crc_ok = false;
  storage_set_flash_area(sdriver, bootinfo);
  if (storage_is_empty_partition(sdriver)) {
    LOG_WARN("Bootinfo partition is empty");
    _bootloader_initialize_ctxt(&_bctxt, sdriver);
    _bctxt_in_Flash = _bctxt;
    force_bootloader = true;
    empty_partition = true;
  }
  else {
    _bootloader_retrieve_ctxt(&_bctxt, sdriver);
    _bctxt_in_Flash = _bctxt;
    bootinfo_crc_ok = _bootloader_verify_bootinfo_crc(&_bctxt, 1);
  }

  // backup bootinfo
  bool bootinfob_crc_ok = false;
  storage_set_flash_area(sdriver, &bootinfob);
  if (storage_is_empty_partition(sdriver)) {
    LOG_WARN("Bootinfo backup partition is empty");
    _bootloader_initialize_ctxt(&_bctxt_backup, sdriver);
    empty_partition = true;
  } else {
    _bootloader_retrieve_ctxt(&_bctxt_backup, sdriver);
    bootinfob_crc_ok = _bootloader_verify_bootinfo_crc(&_bctxt_backup, 1);
  }

  if (_bootloader_ctxt_equal(&_bctxt_backup,&_bctxt) &&
    bootinfo_crc_ok &&
    bootinfob_crc_ok) {
    // both crc's are valid
    return force_bootloader;
  }
  else {
    _bootloader_print_info_header(&_bctxt,"Bootinfo");
    LOG_RAW("");
    _bootloader_print_info_header(&_bctxt_backup, "Backup Bootinfo");
  }

  // recovery step backup -> bootinfo
  if (bootinfo_crc_ok == false && bootinfob_crc_ok == true) {
    // Restore backup bootinfo -> bootinfo
    storage_set_flash_area(sdriver, bootinfo);
    _bootloader_store_ctxt(&_bctxt_backup, sdriver);

    if(! _bootloader_ctxt_equal(&_bctxt_backup,&_bctxt) && bootinfob_crc_ok ) {
      LOG_INFO("Bootinfo Backup <> Bootinfo, restoring backup bootinfo");
      storage_set_flash_area(sdriver, bootinfo);
      _bootloader_store_ctxt(&_bctxt_backup, sdriver);
    }
  }
  //check if backup bootinfo is in sync
  if (bootinfo_crc_ok == true && bootinfob_crc_ok == false) {
    if(! _bootloader_ctxt_equal(&_bctxt_backup,&_bctxt) && bootinfo_crc_ok ) {
      LOG_INFO("Bootinfo Backup <> Bootinfo, fix backup bootinfo");
      storage_set_flash_area(sdriver, &bootinfob);
      _bootloader_store_ctxt(&_bctxt, sdriver);
    }
  }

  // partitions are not empty and crc's not valid -> Erase
  if (!empty_partition && !bootinfo_crc_ok && !bootinfob_crc_ok) {
    LOG_WARN("No valid bootinfo crc, wipe entire application flash");
    storage_wipe_entire_flash(sdriver);
    force_bootloader = true;
  }

  return force_bootloader;
}

/**
 * @brief  Watchdog interrupt handler
 *
 * @returns void
 */
void WDT_BOD_IRQHandler(void)
{
  LOG_DEBUG("bootloader IRQ Watchdog Handler entered");

  uint32_t wdtStatus = WWDT_GetStatusFlags(WWDT);
  if (wdtStatus & kWWDT_TimeoutFlag)
  {
    WWDT_ClearStatusFlags(WWDT, kWWDT_TimeoutFlag);
    LOG_DEBUG("IRQ Watchdog timeout-flag cleared");
  }

  /* Handle warning interrupt */
  if (wdtStatus & kWWDT_WarningFlag)
  {
    /* A watchdog feed didn't occur prior to warning timeout */
    WWDT_ClearStatusFlags(WWDT, kWWDT_WarningFlag);
    LOG_INFO("Bootloader watchdog warning.. ");
    /* User code. User can do urgent case before timeout reset.*/
    /* Activate the HW-Reset now for next WDT_BOD_IRQHandler() Entry*/
     wwdt_config.enableWatchdogReset = true;
     WWDT_Init(WWDT, &wwdt_config);
     /* Remark: ResetISR(); does not to work for subsequent use */
  }
  SDK_ISR_EXIT_BARRIER;
}


/**
 * @brief  bootloader main
 *
 * @returns int
 */
int main(void)
{
  bool force_bootloader = false;
  bool end_bootloader = false;
  bool reset_bootlader = false;
  int pstate_0 = 0, pstate_1 = 0;

  _initialize_board();

  LOG_INFO("Bootloader (%s)", BOOTLOADER_VERSION_GIT);

  struct flash_area_t approm0 = storage_new_flash_area(FLASH_HELPER(approm0));
  struct flash_area_t approm1 = storage_new_flash_area(FLASH_HELPER(approm1));
  struct flash_area_t bootinfo = storage_new_flash_area(FLASH_HELPER(bootinfo));
  struct flash_area_t *ptr_bootinfo = &bootinfo;

  struct storage_driver_t *sdriver = storage_new_flash_driver();

  // spi flash = 4 blocks of 64K  (Actual gowin .bin file size is 0x360b8)
  struct spi_flash_area_t spiflash = storage_new_spi_flash_area("spi", 0x0, 0x40000);
  struct storage_driver_t *spi_flash_driver = storage_new_spi_flash_driver();

  if (!sdriver || !spi_flash_driver) {
    LOG_ERROR("This is bad...");
    return -1;
  }

  int err = storage_init_storage(sdriver);
  if (err < 0) {
    LOG_ERROR("flash_driver is bad...");
    return -1;
  }

  err = storage_init_storage(spi_flash_driver);
  if (err < 0) {
    LOG_ERROR("spi_flash_driver is bad...");
    return -1;
  }

  force_bootloader = _initialize_bootinfo(sdriver, ptr_bootinfo);

  /* TODO: Remove this later, kept for now for testing. */
  /* err = storage_wipe_entire_flash(sdriver); */
  /* if (err < 0) { */
  /*  LOG_ERROR("Failed to erase flash.."); */
  /*  return -1; */
  /* } */

  storage_set_flash_area(sdriver, &approm0);
  if (storage_is_empty_partition(sdriver)) {
    LOG_WARN("Approm0 partition is empty");
  } else {
    pstate_0 = _bootloader_ctxt_validate_partition(&_bctxt, APP_PARTITION_0);
  }

  storage_set_flash_area(sdriver, &approm1);
  if (storage_is_empty_partition(sdriver)) {
    LOG_WARN("Approm1 partition is empty");
  } else {
    pstate_1 = _bootloader_ctxt_validate_partition(&_bctxt, APP_PARTITION_1);
  }

  storage_set_spi_flash_area(spi_flash_driver, &spiflash);

  //!< Validate pstate's and select according to state
  if (pstate_0!=1) {
    /* No valid partition found on part0 */
    force_bootloader = true;
    storage_set_flash_area(sdriver, &approm0);
    _bctxt.part = APP_PARTITION_0;
  }
  else if(pstate_1!=1) {
    /* No valid partition found on part1 */
    force_bootloader = true;
    storage_set_flash_area(sdriver, &approm1);
    _bctxt.part = APP_PARTITION_1;
  }
  /* both partitions are valid check the application's request */
  else if (_gdata.application_request == UPDATE) {
    LOG_OK("Application requests bootloader mode for update");
    force_bootloader = true;
  }
  else if ((_gdata.application_request == BOOT_PARTITION_0) && pstate_0) {
    LOG_OK("Application requests booting partition 0");
    _bctxt.part = APP_PARTITION_0; // not persistent in flash!
  }
  else if ((_gdata.application_request == BOOT_PARTITION_1) && pstate_1) {
    LOG_OK("Application requests booting partition 1");
    _bctxt.part = APP_PARTITION_1; // not persistent in flash!
  }
  //!< clear the application's request
  _gdata.application_request = NONE;

  /* debugging ! */
//#define DEBUG_COMMP_STACK
#ifdef DEBUG_COMMP_STACK
#warning Force bootloader is enabled!
  force_bootloader = true;
#endif

  /* Bare metal systems don't like logging in between time critical stuff.. */
  /* To debug the protocol, add some delays in the transfer loop at the host */
  /* Side. Otherwise this will just fail with a RX Fifo overflow error.. */
  /* In debug mode loglevel is standard set to LOG_LVL_EXTRA */

  /* NOTE: Move to the app at some point! */
  uint8_t boot_pins = BOARD_read_fgpa_boot_mode();
  if (boot_pins == 0b0000) {
    LOG_INFO("Bootmode: 0x%X (JTAG)", boot_pins);
  }
  else if(boot_pins == 0b0010) {
    LOG_INFO("Bootmode: 0x%X (QSPI32)", boot_pins);
  }
  else {
    LOG_INFO("Bootmode: 0x%X (?)", boot_pins);
  }

  //!< Initialize Watchdog
  LOG_INFO("Bootloader Watchdog initialised");
  //BOARD_WWDT_init(&wwdt_config);

  while (_initialize_usb() < 0) {
    SDK_DelayAtLeastUs(1000000, 96000000U);
    LOG_INFO("retrying usb initialisation..");
    // will auto reboot via wdog
  };
  /* End of what needs to be moved to app! */

  if (force_bootloader) {
    LOG_OK("Forced into bootloader mode");
    comm_protocol_init();
    struct comm_driver_t *cdriver = comm_protocol_find_driver("uart");

    if ((pstate_0 == 1) & (pstate_1 == 1)){
      /* inform user about which partition is selected */
      if (_bctxt.part == APP_PARTITION_0) {
        storage_set_flash_area(sdriver, &approm1);
        _bctxt.part = APP_PARTITION_1; // for correct CRC writing
        LOG_INFO("Partition 0 in use, Writing to partition 1");
      } else if(_bctxt.part == APP_PARTITION_1) {
        storage_set_flash_area(sdriver, &approm0);
        _bctxt.part = APP_PARTITION_0; // for correct CRC writing
        LOG_INFO("Partition 1 in use, Writing to partition 0");
      }
    }
    else {
      LOG_INFO("Partition %d requires flashing", _bctxt.part);
    }

    _bootloader_print_info(&_bctxt_in_Flash);

    while (!end_bootloader) {
      bool swap_part = false;
      bool update_bootinfo = false;

      logger_set_loglvl(LOG_LVL_ALL);
      // logger_set_loglvl(LOG_LVL_EXTRA); // <- to slow for reply in time

      //WWDT_Refresh(WWDT);
      /* for comm_protocol_run() there is a wdog refresh in commp_uart.c */
      int retval = comm_protocol_run(&_bctxt, cdriver, sdriver, spi_flash_driver);

      LOG_INFO("comm_protocol_run returned (%d)", retval);
      switch (retval) {
      case COMMP_CMD_ERROR:
        LOG_ERROR("Protocol run error..");
        break;
      case COMMP_CMD_BOOT:                          // Force application to boot
        LOG_INFO("Forcing bootloader");
        force_bootloader = false;
        end_bootloader = true;
        break;
      case COMMP_CMD_CRC:
        LOG_INFO("No Action for CMD_CRC");
        break;
      case COMMP_CMD_SWAP:
        swap_part = true;
        break;
      case COMMP_CMD_BOOTINFO:
        LOG_INFO("Sending bootinfo to host");
        _bootloader_send_bootinfo(&_bctxt_in_Flash, cdriver);
        break;
      case COMMP_CMD_PWR_ON:
        LOG_INFO("Forcing ZynQMP On");
        BOARD_PwrOn_MainCPU(200000);
        break;
      case COMMP_CMD_PWR_OFF:                  // Not Functional on Alpha board!
        LOG_INFO("Forcing ZynQMP Off");
        BOARD_PwrOff_MainCPU(200000);
        break;
      case COMMP_CMD_END:                                   // Flashing finished
        update_bootinfo = true;
        break;
      case COMMP_CMD_RESET:
        LOG_OK("Reset Requested");
        SDK_DelayAtLeastUs(3000000, 96000000U);
        reset_bootlader = true;
        end_bootloader = true;
        break;
      case COMMP_CMD_TRIGGER_WDOG:                             // wdog reset, 5s
        for (;;)
        {
          LOG_OK("Trigger Watchdog MOD Reg[%02x]", WWDT->MOD);
          SDK_DelayAtLeastUs(1000000, 96000000U);
        }
        break;
      case COMMP_CMD_ERASE_SPI:
        LOG_INFO("SPI Erase requested");
        storage_set_spi_flash_area(spi_flash_driver, &spiflash);
        _bootloader_erase_spi_rom(spi_flash_driver);
        break;
        // CRC and default do nothing.
      default:
        break;
      }

      // Add end_bootloader handling
      logger_set_loglvl(LOG_LVL_EXTRA);

      if (swap_part) {
        //Warning: Software already swapped partitions for flash update
        LOG_DEBUG("Swapped partition to %d",_bctxt.part);
        update_bootinfo = true;
      }

      if (update_bootinfo) {
        LOG_DEBUG("Updating bootinfo on flash");
        storage_set_flash_area(sdriver, &bootinfo);
        _bootloader_store_ctxt(&_bctxt, sdriver);
        struct flash_area_t bootinfob = storage_new_flash_area(FLASH_HELPER(bootinfob));
        storage_set_flash_area(sdriver, &bootinfob);
        _bootloader_store_ctxt(&_bctxt, sdriver);
        _bctxt_in_Flash = _bctxt;
        /* Reset after flashing */
        if (retval == COMMP_CMD_END) {
          LOG_INFO("Performing RESET after flashing");
          SDK_DelayAtLeastUs(1000000, 96000000U);
          reset_bootlader = true;
          end_bootloader = true;
        }
      }

    }
    comm_protocol_close();
  }

  if (!reset_bootlader) {
    uint32_t *app_code = (uint32_t *)_bctxt.apps[_bctxt.part].start_addr;
    uint32_t sp = app_code[0];
    uint32_t pc = app_code[1];

    LOG_INFO("Jmp to application[%d] PC(%X) SP(%X)", _bctxt.part, pc, sp);
    SDK_DelayAtLeastUs(1000000, 96000000U);
    BOARD_BootDeinitPeripherals();
    jump_to(pc, sp);
    // we never get here
    _gdata.reboot_counter++;  //!< Cold start detection
    LOG_INFO("Return from main reboot_counter[%d]",_gdata.reboot_counter);
  }

  ResetISR(); //!< If we return from application, reset the core

  return 0;
}
