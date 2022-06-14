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
#include "bootloader_spi.h"
#include "comm_protocol.h"

#include "storage.h"
#include "storage_flash.h"
#include "storage_spi_flash.h"
#include "bootloader_helpers.h"
#include "bootloader_spi_helpers.h"
#include "bootloader_usb_helpers.h"

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
static struct spi_ctxt_t _spi0_ctxt;
static struct spi_ctxt_t _spi1_ctxt;

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
static int _initialize_board(void) {
    uint8_t revision = BOARD_GetRevision();

    (void)revision;                 // Probably we'll need to use this at some point

    BOARD_BootInitBootPins();     // !< Init bootloader pins
    BOARD_BootInitClocks();         // !< Initialize Board Clocks
    BOARD_BootInitPeripherals();  // !< Init bootloader peripherals

    if (_gdata.reboot_counter == 0) // to do: check cold restart instead
        wipe_sram(0);
    else
        wipe_sram(sizeof(_gdata));

    SDK_DelayAtLeastUs(500000, 96000000U);

    /* GO GOWIN GO! */
    BOARD_PwrOn_MainCPU(500000);

    logger_init();

    LOG_OK("%s Board initialized", BOARD_NAME);

    return 0;
}


/**
 * @brief   initialisation/verification bootinfo
 *
 * @returns  bool 1 if we need to stay in bootloader
 */
static bool _initialize_bootinfo(struct storage_driver_t * sdriver,
                                 struct flash_area_t * bootinfo) {
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
    } else {
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

    if (_bootloader_ctxt_equal(&_bctxt_backup, &_bctxt) &&
        bootinfo_crc_ok &&
        bootinfob_crc_ok) {
        // both crc's are valid
        return force_bootloader;
    } else {
        // no need to compare empty partitions
        if (!empty_partition) {
            _bootloader_print_info_header(&_bctxt, "Bootinfo");
            LOG_RAW("----- find the diff -----");
            _bootloader_print_info_header(&_bctxt_backup, "Backup Bootinfo");
        }
    }

    // recovery step backup -> bootinfo
    if (bootinfo_crc_ok == false && bootinfob_crc_ok == true) {
        // Restore backup bootinfo -> bootinfo
        storage_set_flash_area(sdriver, bootinfo);
        _bootloader_store_ctxt(&_bctxt_backup, sdriver);

        if (!_bootloader_ctxt_equal(&_bctxt_backup, &_bctxt) && bootinfob_crc_ok ) {
            LOG_INFO("Bootinfo Backup <> Bootinfo, restoring backup bootinfo");
            storage_set_flash_area(sdriver, bootinfo);
            _bootloader_store_ctxt(&_bctxt_backup, sdriver);
        }
    }
    // check if backup bootinfo is in sync
    if (bootinfo_crc_ok == true && bootinfob_crc_ok == false) {
        if (!_bootloader_ctxt_equal(&_bctxt_backup, &_bctxt) && bootinfo_crc_ok ) {
            LOG_INFO("Bootinfo Backup <> Bootinfo, fix backup bootinfo");
            storage_set_flash_area(sdriver, &bootinfob);
            _bootloader_store_ctxt(&_bctxt, sdriver);
        }
    }

    // partitions are not empty and crc's not valid -> Erase
    if (!empty_partition && !bootinfo_crc_ok && !bootinfob_crc_ok) {
        LOG_WARN("No valid bootinfo crc");
        storage_set_flash_area(sdriver, bootinfo);
        storage_wipe_entire_flash(sdriver);
        storage_set_flash_area(sdriver, &bootinfob);
        storage_wipe_entire_flash(sdriver);
        force_bootloader = true;
    }

    return force_bootloader;
}

/**
 * @brief   initialisation/verification spi context
 *
 * @returns  bool
 */
static bool _initialize_spi_info(struct storage_driver_t * spidriver,
                                 struct spi_flash_area_t * spiflash0,
                                 struct spi_flash_area_t * spiflash1) {
    // !< no need to switch spi regions, 2nd partition is meant for backup
    storage_set_spi_flash_area(spidriver, spiflash0);

    if (_bootloader_spi_retrieve_ctxt(&_spi0_ctxt, spidriver) > 0) {
        _bootloader_spi_ctxt_validate_partition(&_spi0_ctxt, spidriver, GOWIN_PARTITION_0);
    } else {
        // initialisation in case context is empty
        _bootloader_spi_initialize_ctxt(&_spi0_ctxt, GOWIN_PARTITION_0);
    }

    storage_set_spi_flash_area(spidriver, spiflash1);

    if (_bootloader_spi_retrieve_ctxt(&_spi1_ctxt, spidriver) > 0) {
        _bootloader_spi_ctxt_validate_partition(&_spi1_ctxt, spidriver, GOWIN_PARTITION_1);
    } else {
        // initialisation in case context is empty
        _bootloader_spi_initialize_ctxt(&_spi1_ctxt, GOWIN_PARTITION_1);
    }

    if (_bootloader_spi_ctxt_equal(&_spi0_ctxt, &_spi1_ctxt)) {
        LOG_OK("Spi context-data checked out identical");
    } else {
        LOG_WARN("Spi context-data not identical");
        _bootloader_spi_print_info(&_spi0_ctxt);
        LOG_RAW("----- find the diff -----");
        _bootloader_spi_print_info(&_spi1_ctxt);
    }

    return true;
}

/**
 * @brief  Watchdog interrupt handler
 *
 * @returns void
 */
void WDT_BOD_IRQHandler(void) {
    LOG_DEBUG("bootloader IRQ Watchdog Handler entered");

    uint32_t wdtStatus = WWDT_GetStatusFlags(WWDT);
    if (wdtStatus & kWWDT_TimeoutFlag) {
        WWDT_ClearStatusFlags(WWDT, kWWDT_TimeoutFlag);
        LOG_DEBUG("IRQ Watchdog timeout-flag cleared");
    }

    /* Handle warning interrupt */
    if (wdtStatus & kWWDT_WarningFlag) {
        /* A watchdog feed didn't occur prior to warning timeout */
        WWDT_ClearStatusFlags(WWDT, kWWDT_WarningFlag);
        LOG_INFO("Bootloader watchdog warning.. ");
        /* User code. User can do urgent case before timeout reset.*/
        /* Activate the HW-Reset now for next WDT_BOD_IRQHandler() Entry*/
        wwdt_config.enableWatchdogReset = true;
        WWDT_Init(WWDT, &wwdt_config);
        /* Remark: ResetISR(); does not work for subsequent use */
    }
    SDK_ISR_EXIT_BARRIER;
}


/**
 * @brief  bootloader main
 *
 * @returns int
 */
int main(void) {
    bool force_bootloader = false;
    bool end_bootloader = false;
    bool reset_bootloader = false;
    int pstate_0 = 0, pstate_1 = 0;

    _initialize_board();

    LOG_INFO("Bootloader (%s)", BOOTLOADER_VERSION_GIT);

    struct flash_area_t approm0 = storage_new_flash_area(FLASH_HELPER(approm0));
    struct flash_area_t approm1 = storage_new_flash_area(FLASH_HELPER(approm1));
    struct flash_area_t bootinfo = storage_new_flash_area(FLASH_HELPER(bootinfo));
    struct flash_area_t * ptr_bootinfo = &bootinfo;

    struct storage_driver_t * sdriver = storage_new_flash_driver();

    // spi flash = 4 blocks of 64K  (Actual gowin .bin file size is 0x360b8)
    struct spi_flash_area_t spiflash0 = storage_new_spi_flash_area("spi0",
                                                                   SPI0_START_ADDR, SPI_PART_SIZE);
    struct spi_flash_area_t spiflash1 = storage_new_spi_flash_area("spi1",
                                                                   SPI1_START_ADDR, SPI_PART_SIZE);

    struct storage_driver_t * spi_flash_driver = storage_new_spi_flash_driver();

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

    logger_set_loglvl(LOG_LVL_ALL);

    _initialize_spi_info(spi_flash_driver, &spiflash0, &spiflash1);

    // !< Validate pstate's and select according to state
    if (pstate_0 != 1) {
        /* No valid partition found on part0 */
        force_bootloader = true;
        storage_set_flash_area(sdriver, &approm0);
        _bctxt.part = APP_PARTITION_0;
    } else if (pstate_1 != 1) {
        /* No valid partition found on part1 */
        force_bootloader = true;
        storage_set_flash_area(sdriver, &approm1);
        _bctxt.part = APP_PARTITION_1;
    }
    /* both partitions are valid check the application's request */
    else if (_gdata.application_request == UPDATE) {
        LOG_OK("Application requests bootloader mode for update");
        force_bootloader = true;
    } else if ((_gdata.application_request == BOOT_PARTITION_0) && pstate_0) {
        LOG_OK("Application requests booting partition 0");
        _bctxt.part = APP_PARTITION_0; // not persistent in flash!
    } else if ((_gdata.application_request == BOOT_PARTITION_1) && pstate_1) {
        LOG_OK("Application requests booting partition 1");
        _bctxt.part = APP_PARTITION_1; // not persistent in flash!
    }
    // !< clear the application's request
    _gdata.application_request = NONE;

    /* debugging ! */
// #define DEBUG_COMMP_STACK
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
    } else if (boot_pins == 0b0010) {
        LOG_INFO("Bootmode: 0x%X (QSPI32)", boot_pins);
    } else if (boot_pins == 0b0110) {
        LOG_INFO("Bootmode: 0x%X (emmc)", boot_pins);
    } else {
        LOG_INFO("Bootmode: 0x%X (?)", boot_pins);
    }

    // !< Initialize Watchdog
    LOG_INFO("Bootloader Watchdog initialised");
    // BOARD_WWDT_init(&wwdt_config);

#ifdef BOARD_ZEUS
    uint8_t timeout = 5;
    while (_initialize_usb_zeus() < 0) {
        SDK_DelayAtLeastUs(1000000, 96000000U);
        LOG_INFO("retrying usb initialisation.. %d", timeout);
        if (timeout-- == 0)
            break;
        // will auto reboot via wdog
    }
    ;
#endif

#ifdef BOARD_GAIA
    logger_set_loglvl(LOG_LVL_EXTRA);
    // usb1 comes first because the i2c-adr gets adjusted
    _initialize_usb1_gaia();
    LOG_RAW("");
    // usb0 keeps it's original i2c-address 0xD2
    _initialize_usb0_gaia();
    LOG_RAW("");
#endif

    if (BOARD_Ready_Gowin() == 1)
        LOG_OK("Gowin is Ready");
    else
        LOG_ERROR("Gowin Ready State missing");

    /* End of what needs to be moved to app! */

    if (force_bootloader) {
        LOG_OK("Forced into bootloader mode");
        comm_protocol_init();
        struct comm_driver_t * cdriver = comm_protocol_find_driver("uart");

        if ((pstate_0 == 1) & (pstate_1 == 1)) {
            /* inform user about which partition is selected */
            if (_bctxt.part == APP_PARTITION_0) {
                storage_set_flash_area(sdriver, &approm1);
                _bctxt.part = APP_PARTITION_1; // for correct CRC writing
                LOG_INFO("Partition 0 in use, Writing to partition 1");
            } else if (_bctxt.part == APP_PARTITION_1) {
                storage_set_flash_area(sdriver, &approm0);
                _bctxt.part = APP_PARTITION_0; // for correct CRC writing
                LOG_INFO("Partition 1 in use, Writing to partition 0");
            }
        } else {
            LOG_INFO("Partition %d requires flashing", _bctxt.part);
        }

        _bootloader_print_info(&_bctxt_in_Flash);

        while (!end_bootloader) {
            bool swap_part = false;
            bool update_bootinfo = false;
            bool trigger_reboot = false;

            logger_set_loglvl(LOG_LVL_ALL);
            // logger_set_loglvl(LOG_LVL_EXTRA); // <- to slow for reply in time

            WWDT_Refresh(WWDT);

            /* for comm_protocol_run() there is a wdog refresh in commp_uart.c */
            int retval = comm_protocol_run(&_bctxt, cdriver, sdriver, spi_flash_driver, &_spi0_ctxt);

            LOG_INFO("comm_protocol_run returned (%d)", retval);
            switch (retval) {
                case COMMP_CMD_ERROR:
                    LOG_ERROR("Protocol run error..");
                    break;
                case COMMP_CMD_BOOT:                // Force application to boot
                    if (pstate_0 == 0 && pstate_1 == 0) {
                        LOG_WARN("Can't boot no valid application partition");
                    } else {
                        LOG_INFO("Forcing bootloader to boot app");
                        reset_bootloader = false;
                        end_bootloader = true;
                    }
                    break;
                case COMMP_CMD_CRC:                         // Flashing finished
                    LOG_INFO("Update bootinfo upon CMD_CRC");
                    update_bootinfo = true;
                    break;
                case COMMP_CMD_SWAP:
                    swap_part = true;
                    break;
                case COMMP_CMD_BOOTINFO:
                    LOG_INFO("Sending bootinfo to host");
                    _bootloader_send_bootinfo(&_bctxt_in_Flash, cdriver);
                    break;
                case COMMP_CMD_PWR_ON:
                    LOG_INFO("Forcing ZynQMP On"); // not possible - MainCPU is down
                    BOARD_PwrOn_MainCPU(200000);
                    break;
                case COMMP_CMD_PWR_OFF:            // Not Functional on Alpha board!
                    LOG_INFO("Forcing ZynQMP Off");
                    BOARD_PwrOff_MainCPU(200000);
                    break;
                case COMMP_CMD_RECONFIG_GOWIN:
                    LOG_DEBUG("Reconfigure Gowin -> MainCPU will RESET!");
                    SDK_DelayAtLeastUs(1000000, 96000000U);
                    BOARD_Reconfigure_Gowin(200000);
                    SDK_DelayAtLeastUs(1000000, 96000000U);
                    LOG_INFO("Gowin Ready State is [%s]", (BOARD_Ready_Gowin() == 1 ? "OK" : "NOK"));
                    ResetISR(); // to do: find better way to INSTANTLY restart the MainCPU
                    break;
                case COMMP_CMD_END:                    // communication finished
                    trigger_reboot = true;
                    break;
                case COMMP_CMD_SPI_END:                // Spi Flashing finished
                    break;
                case COMMP_CMD_RESET:
                    LOG_OK("Reset Requested");
                    SDK_DelayAtLeastUs(3000000, 96000000U);
                    reset_bootloader = true;
                    end_bootloader = true;
                    break;
                case COMMP_CMD_TRIGGER_WDOG:                   // wdog reset, 5s
                    for (;;) {
                        LOG_OK("Trigger Watchdog MOD Reg[%02x]", WWDT->MOD);
                        SDK_DelayAtLeastUs(1000000, 96000000U);
                    }
                    break;

                case COMMP_CMD_SET_ROM0:
                    storage_set_spi_flash_area(sdriver, &approm0);
                    _bctxt.part = APP_PARTITION_0;
                    LOG_INFO("Application Rom0 selected");
                    break;

                case COMMP_CMD_SET_ROM1:
                    storage_set_spi_flash_area(sdriver, &approm1);
                    _bctxt.part = APP_PARTITION_1;
                    LOG_INFO("Application Rom1 selected");
                    break;

                /*********************** SPI related ************************************/
                case COMMP_CMD_SET_SPI0:       // Meant for reading back the rom
                    storage_set_spi_flash_area(spi_flash_driver, &spiflash0);
                    LOG_INFO("Spi0 selected");
                    break;

                case COMMP_CMD_SET_SPI1:
                    storage_set_spi_flash_area(spi_flash_driver, &spiflash1);
                    LOG_INFO("Spi1 selected");
                    break;

                case COMMP_CMD_ERASE_SPI0:
                    LOG_INFO("Spi0 rom erase requested");
                    storage_set_spi_flash_area(spi_flash_driver, &spiflash0);
                    _bootloader_spi_erase_spi_rom(spi_flash_driver);
                    _bootloader_spi_initialize_ctxt(&_spi0_ctxt, GOWIN_PARTITION_0);
                    break;

                case COMMP_CMD_ERASE_SPI1:
                    LOG_INFO("Spi1 rom erase requested");
                    storage_set_spi_flash_area(spi_flash_driver, &spiflash1);
                    _bootloader_spi_erase_spi_rom(spi_flash_driver);
                    _bootloader_spi_initialize_ctxt(&_spi0_ctxt, GOWIN_PARTITION_1);
                    break;

                case COMMP_CMD_WRITE_SPI_CTXT:
                    LOG_INFO("Spi write partition context");
                    storage_set_spi_flash_area(spi_flash_driver, &spiflash0);
                    // Put the partition context in place
                    // General _spi0_ctxt was filled upon COMMP_CMD_ERASE_SPI0/1
                    // _spi0_ctxt crc specifics were filled in commp.c
                    _bootloader_spi_store_ctxt(&_spi0_ctxt, spi_flash_driver);
                    // keep data in sync , partition ctxt is identical for both partitions
                    storage_set_spi_flash_area(spi_flash_driver, &spiflash1);
                    _bootloader_spi_store_ctxt(&_spi0_ctxt, spi_flash_driver);
                    _bootloader_spi_print_info_header(&_spi0_ctxt, "spi");
                    break;

                case COMMP_CMD_INFO_SPI:
                    LOG_INFO("Sending spi info to host");
                    _bootloader_spi_send_spiinfo(&_spi0_ctxt, cdriver);
                    _bootloader_spi_print_info(&_spi0_ctxt);
                    break;

                default:
                    break;
            }

            // Add end_bootloader handling
            logger_set_loglvl(LOG_LVL_EXTRA);

            if (swap_part) {
                // Warning: Software already swapped partitions for flash update
                LOG_DEBUG("Swapped partition to %d", _bctxt.part);
                update_bootinfo = true;
            }

            if (update_bootinfo) {
                LOG_DEBUG("Updating bootinfo on flash");
                storage_set_flash_area(sdriver, &bootinfo);
                _bootloader_store_ctxt(&_bctxt, sdriver);
                // backup bootinfo
                struct flash_area_t bootinfob = storage_new_flash_area(FLASH_HELPER(bootinfob));
                storage_set_flash_area(sdriver, &bootinfob);
                _bootloader_store_ctxt(&_bctxt, sdriver);
                _bctxt_in_Flash = _bctxt;

                // COMMP_CMD_END will follow and we will auto-reboot
            }

            if (trigger_reboot) { // we get here after END cmd from file transfer was sent
                struct comm_ctxt_t ctxt = _get_run_transfer_ctxt();
                LOG_INFO("partition from run was %x", ctxt.part_nr);
                if (ctxt.part_nr == COMMP_ROMID_NONE && _bctxt.part == APP_PARTITION_0) { // empty rom, just flashed partition0
                    storage_set_flash_area(sdriver, &approm1);
                    _bctxt.part = APP_PARTITION_1; // for correct CRC writing
                    LOG_INFO("Partition %d requires flashing", _bctxt.part);
                } else if (ctxt.part_nr == COMMP_ROMID_NONE && _bctxt.part == APP_PARTITION_1) {
                    LOG_INFO("Partition %d was flashed, launching APPLICATION", _bctxt.part);
                    _bctxt.part = APP_PARTITION_0;
                    reset_bootloader = false;
                    end_bootloader = true;
                } else if (ctxt.part_nr == COMMP_ROMID_ROM0 || ctxt.part_nr == COMMP_ROMID_ROM1) {
                    LOG_INFO("Performing RESET after flashing");
                    reset_bootloader = true;
                    end_bootloader = true;
                }
            }

        }
        comm_protocol_close();
    }

    if (!reset_bootloader) {
        uint32_t * app_code = (uint32_t *)_bctxt.apps[_bctxt.part].start_addr;
        uint32_t sp = app_code[0];
        uint32_t pc = app_code[1];

        LOG_INFO("Jmp to application[%d] PC(%X) SP(%X)", _bctxt.part, pc, sp);
        BOARD_BootDeinitPeripherals();
        SDK_DelayAtLeastUs(1000000, 96000000U); // delay needed!
        jump_to(pc, sp);
        // we never get here
        _gdata.reboot_counter++; // !< Cold start detection
        LOG_INFO("Return from main reboot_counter[%d]", _gdata.reboot_counter);
    }

    BOARD_WWDT_deinit();
    LOG_RAW("**** BOOTLOADER RESET ****");
    ResetISR(); // !< reset the core

    return 0;
}
