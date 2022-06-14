/**
 * @file main.c
 * @brief Main file for Green Power MCU application
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-15
 * @changes
 *____date______author__description_____________________________________________
 * 2021-09-15 | davth | wdog and usb initialisation
 * 2021-06-08 | davth | added irq based comm protocol
 * 2022-01-10 | davth | spi_master added on Flexcomm8
 *______________________________________________________________________________
 */

#include <stdio.h>

#include "board.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "clock_config.h"

#include "logger.h"
#include "version.h"
#include "memory_map.h"

#include "comm/protocol.h"
#include "comm/gowin_protocol.h"
#include "comm/comm.h"
#include "softversions.h"
#include "data_map.h"
#include "i2c/i2c_master.h"
#include "i2c/i2c_slave.h"
#include "spi/spi_master.h"

#include "../bootloader/bootloader_usb_helpers.h"

#include "fsl_wwdt.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifdef ENABLE_SEMIHOSTING
extern void initialise_monitor_handles(void);
#endif

/*******************************************************************************
 * GLOBAL Variables
 ******************************************************************************/
struct ssram_data_t _gdata __attribute__((section(".ssram_sect")));

struct ssram_data_t * SharedRamStruct_pointer() {
    return &_gdata;
}

u8 g_tipString[] = "Usart Flexcomm functional test\r\n";

// uart ring buffer - raw data -> holds any incoming byte!
u8 MCU_RXBUF[MCU_RXBUF_SIZE];

// uart ring buffer - raw data -> holds any incoming byte!
u8 GOWIN_RXBUF[GOWIN_RXBUF_SIZE];

// initialize raw uart data structures
#ifdef BOARD_GAIA
u8 MCU2_RXBUF[MCU_RXBUF_SIZE];
volatile t_uart_data_raw UART_DATA[NUMBER_OF_PROTOCOL_UARTS] = {
    { (u8 *)&MCU_RXBUF,   MCU_RXBUF_SIZE,   (u8 *)&MCU_RXBUF,    (u8 *)&MCU_RXBUF,   0   },
    { (u8 *)&GOWIN_RXBUF, GOWIN_RXBUF_SIZE, (u8 *)&GOWIN_RXBUF,  (u8 *)&GOWIN_RXBUF, 0   },
    { (u8 *)&MCU2_RXBUF,  MCU_RXBUF_SIZE,   (u8 *)&MCU2_RXBUF,   (u8 *)&MCU2_RXBUF,  0   }
};
#else
volatile t_uart_data_raw UART_DATA[NUMBER_OF_PROTOCOL_UARTS] = {
    { (u8 *)&MCU_RXBUF,   MCU_RXBUF_SIZE,   (u8 *)&MCU_RXBUF,   (u8 *)&MCU_RXBUF,   0   },
    { (u8 *)&GOWIN_RXBUF, GOWIN_RXBUF_SIZE, (u8 *)&GOWIN_RXBUF, (u8 *)&GOWIN_RXBUF, 0   }
};
#endif

// watchdog configuration : g_main_loop exits when wdog IRQ runs out
wwdt_config_t g_wwdt_config;
bool g_main_loop = true;

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

    (void)revision;             // Probably we'll need to use this at some point

    BOARD_AppInitBootPins();    // !< Initialize Board Pinout
    BOARD_AppInitClocks(FRO96, false); // !< Initialize Board Clocks
    BOARD_AppInitPeripherals(); // !< Initialize Board peripherals

    // !< Note: logger init was done in bootloader

    return 0;
}

/**
 * @brief  _handle_gpio_request
 *   the main-data.map can set GPIO requests which are handled here
 *   or need update from gpio used further in the program.
 *   Handle the pin_mux access here (prevents unittest dependency problems)
 * @returns void
 */
void _handle_gpio_request() {
    if (Main.Diagnostics.ReconfigureGowin) {
        LOG_DEBUG("Request: Reconfigure Gowin");
        BOARD_Reconfigure_Gowin(200000);
        // MainCPU is off here
        LOG_DEBUG("Gowin Ready State is [%s]", (BOARD_Ready_Gowin() == 1 ? "OK" : "NOK"));

        // trigger total reset by leaving Main
        g_main_loop = false;

        Main.Diagnostics.ReconfigureGowin = false;
    }

    static int autorecoverDelay = 1000; // About 10s
    // auto recover from Standby
    if (Main.Diagnostics.InLowPowerMode) {
        SDK_DelayAtLeastUs(10000, 96000000U); // 10ms sleep
        autorecoverDelay--;
        if (autorecoverDelay == 0) {
            LOG_DEBUG("Recover from Standby");
            Main.Diagnostics.PendingGowinCmd = SET_STANDBY_OFF;
            comm_handler_gowin();
            autorecoverDelay = 1000;

            // The uart cmd is not functional -> recover via pins toggle
            BOARD_PwrOn_MainCPU(500000);

            // We need to reinit the USB as well
            while (i2c_master_initialize_usb() < 0) {
                SDK_DelayAtLeastUs(1000000, 96000000U);
                LOG_INFO("retrying usb initialisation..");
                // will auto reboot via wdog
            }
            ;
        }
    }
}

/**
 * @brief  MainCPU UART IRQ
 *
 * @returns void
 */
void BOARD_MAINCPU_FLEXCOMM_IRQ(void) {
    // If new data arrived
    UART_DATA[UART1].flags = USART_GetStatusFlags(BOARD_MAINCPU_USART);
    if ((kUSART_RxFifoNotEmptyFlag | kUSART_RxError) & UART_DATA[UART1].flags) {
        *((u8 *)UART_DATA[UART1].RxBufWr) = USART_ReadByte(BOARD_MAINCPU_USART);
        UART_DATA[UART1].RxBufWr++;

        if (UART_DATA[UART1].RxBufWr >= (UART_DATA[UART1].RxBuf +
                                         UART_DATA[UART1].RxBufSize)) {
            UART_DATA[UART1].RxBufWr = UART_DATA[UART1].RxBuf; // reset write pointer
        }
    }
    /*
     * if (kUSART_RxFifoFullFlag & UART_DATA[UART1].flags)
     * {
     * // We get here if 0xf bytes are received
     * LOG_DEBUG("MainCPU FLEXCOMM is full, fifo cnt [0x%X]", USART_GetRxFifoCount(BOARD_MAINCPU_USART));
     * }
     */

    SDK_ISR_EXIT_BARRIER;
}

#ifdef BOARD_GAIA
void BOARD_CPU2_FLEXCOMM_IRQ(void) {
    // If new data arrived
    UART_DATA[UART3].flags = USART_GetStatusFlags(BOARD_CPU2_USART);
    if ((kUSART_RxFifoNotEmptyFlag | kUSART_RxError) & UART_DATA[UART3].flags) {
        *((u8 *)UART_DATA[UART3].RxBufWr) = USART_ReadByte(BOARD_CPU2_USART);
        UART_DATA[UART3].RxBufWr++;

        if (UART_DATA[UART3].RxBufWr >= (UART_DATA[UART3].RxBuf +
                                         UART_DATA[UART3].RxBufSize)) {
            UART_DATA[UART3].RxBufWr = UART_DATA[UART3].RxBuf; // reset write pointer
        }
    }
    /*
     * if (kUSART_RxFifoFullFlag & UART_DATA[UART3].flags)
     * {
     * // We get here if 0xf bytes are received
     * LOG_DEBUG("CPU2 FLEXCOMM is full, fifo cnt [0x%X]", USART_GetRxFifoCount(BOARD_CPU2_USART));
     * }
     */

    SDK_ISR_EXIT_BARRIER;
}
#endif

/**
 * @brief  FPGA/Gowin UART IRQ
 *
 * @returns void
 */
void BOARD_GOWIN_FLEXCOMM_IRQ(void) {
    static int cnt = 0;

    /* Get the 8-bit data from the receiver */
    UART_DATA[UART2].flags = USART_GetStatusFlags(BOARD_GOWIN_USART);
    // LOG_DEBUG("FLEXCOMM1 flag is %X",flags);  <-- DO NOT LOG in IRQ!!
    if ((kUSART_RxFifoNotEmptyFlag | kUSART_RxError) & UART_DATA[UART2].flags) {
        *((u8 *)UART_DATA[UART2].RxBufWr) = USART_ReadByte(BOARD_GOWIN_USART);
        UART_DATA[UART2].RxBufWr++;

        if (UART_DATA[UART2].RxBufWr >= (UART_DATA[UART2].RxBuf +
                                         UART_DATA[UART2].RxBufSize)) {
            UART_DATA[UART2].RxBufWr = UART_DATA[UART2].RxBuf; // reset write pointer
        }
    }
    if (kUSART_RxFifoFullFlag & UART_DATA[UART2].flags) {
        wdog_refresh(); // Irq prevents wdog clear in main
        cnt++;
        if (cnt < 20) {
            LOG_WARN("Gowin FLEXCOMM overrun, fifo cnt [0x%X], rx byte[0x%X]",
                     USART_GetRxFifoCount(BOARD_GOWIN_USART), USART_ReadByte(BOARD_GOWIN_USART));
        } else {
            USART_Deinit(BOARD_GOWIN_USART);
            LOG_ERROR("Gowin FLEXCOMM disabled");
        }
    }

    SDK_ISR_EXIT_BARRIER;
}

/**
 * @brief  Watchdog interrupt handler
 *         Watchdog IRQ can be disabled with byte write cmd 0x32!
 * @returns void
 */
void WDT_BOD_IRQHandler(void) {
    LOG_DEBUG("IRQ watchdog entered");

    if (Main.Diagnostics.WatchDogDisabled) {
        WWDT_ClearStatusFlags(WWDT, kWWDT_TimeoutFlag);
        wdog_refresh();
        SDK_ISR_EXIT_BARRIER;
        LOG_DEBUG("IRQ watchdog globally disabled");
    }

    uint32_t wdtStatus = WWDT_GetStatusFlags(WWDT);

    if (wdtStatus & kWWDT_TimeoutFlag) {
        WWDT_ClearStatusFlags(WWDT, kWWDT_TimeoutFlag);
        LOG_DEBUG("IRQ Watchdog timeout-flag cleared");
    }

    /* Handle warning interrupt */
    if (wdtStatus & kWWDT_WarningFlag) {
        /* A watchdog feed didn't occur prior to warning timeout */
        WWDT_ClearStatusFlags(WWDT, kWWDT_WarningFlag);
        LOG_INFO("App watchdog activated.. terminate mainloop");
        /* User code. User can do urgent case before timeout reset.
         * IE. user can backup the ram data or ram log to flash.
         */
        disable_user_irq();
        i2c_slave_disable();
        g_main_loop = false;
    }
    SDK_ISR_EXIT_BARRIER;
}

/**
 * @brief  Main function of the application software
 *
 * @returns On watchdog runout
 */
int main(void) {
    // !< Run Board initialization
    _initialize_board();

    initialize_global_data_map();

    Main.VersionInfo.hwid = BOARD_GetRevision();

    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
        SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_SECUREFAULTENA_Msk;

    // !< This clock is to fast for Flash access!
    // BOARD_AppInitClocks(PLL150, true);

    // !< Initialize Watchdog
    BOARD_WWDT_init(&g_wwdt_config);

    // !< Initialize usb-hub over i2c-master
    while (i2c_master_initialize_usb() < 0) {
        SDK_DelayAtLeastUs(1000000, 96000000U);
        LOG_INFO("retrying usb initialisation..");
        // will auto reboot via wdog
    }
    ;

    // !< Enable i2c-slave interrupts
    i2c_slave_setup();

    // !< Enable Flexcomm interrupts
    enable_user_irq();


#ifdef ENABLE_SEMIHOSTING
    initialise_monitor_handles();
#endif

    LOG_INFO("Application v%d.%d.%d", SOFTVERSIONRUNMAJOR,
             SOFTVERSIONRUNMINOR,
             SOFTVERSIONRUNBUILD);
    LOG_INFO("   git (%s)", APPLICATION_VERSION_GIT);
    LOG_INFO("Bootmode is  [0x%X]", BOARD_read_fgpa_boot_mode());
    LOG_INFO("Gowin State is [%s]", (BOARD_Ready_Gowin() == 1 ? "OK" : "NOK"));

    // !< Debug output for Flexcomm to MainCPU testing
    USART_WriteBlocking(BOARD_MAINCPU_USART, g_tipString, (sizeof(g_tipString) / sizeof(g_tipString[0])) - 1);

    uint64_t pc = get_pc_reg();

    if ( (pc > Main.PartitionInfo.apps[1].start_addr) && (Main.PartitionInfo.part == 0) ) {
        // setting application_request does not change the Flash_info!
        LOG_WARN("PC(%X) outside partition range!", pc);
        Main.PartitionInfo.part = 1;
    } else if ( (pc < Main.PartitionInfo.apps[1].start_addr) && (Main.PartitionInfo.part == 1) ) {
        LOG_WARN("PC(%X) outside partition range!", pc);
        Main.PartitionInfo.part = 0;
    }

    spi_master_setup();

    // !< Gowin initialisation
    GOWIN_Queue_Init();

    // !< Panel initialisation tasks to be transfered to linux boot
    GOWIN_Queue_Tx_Msg(PANEL_VOLTAGEHI);
    GOWIN_Queue_Tx_Msg(PANEL_PWR_ON);
    GOWIN_Queue_Tx_Msg(BACKLIGHT_ON);

    GOWIN_Edid_Initialize(true);

    char application = (Main.PartitionInfo.part == 1 ? '1' : '0');
    // !< postpone print of application if we are going to print EDID's
    if (!GOWIN_DATA.verifyReadBack)
        LOG_INFO("Appli%c up and running", application);

    while (g_main_loop) {
        if (!Main.Diagnostics.WatchDogRunout ) {
            wdog_refresh();
        }

        // !< MainCPU
        COMM_Protocol(UART1); // process incoming data Main CPU
        if (COMM_DATA[UART1].MsgCount) { // handle received messages...
            comm_handler();
            COMM_DATA[UART1].MsgCount = 0; // release RX message buffer
        }

#ifdef BOARD_GAIA
        // !< 2nd CPU
        COMM_Protocol(UART3); // process incoming data 2nd CPU
        if (COMM_DATA[UART3].MsgCount) { // handle received messages...
            comm_handler();
            COMM_DATA[UART3].MsgCount = 0; // release RX message buffer
        }

        _detect_cable_change_gaia();
#endif

        // !< GOWIN FPGA
        if (GOWIN_Protocol(UART2) == GOWIN_RETURN_TIMEOUT) {
            LOG_WARN("No Reply from Gowin");
            LOG_INFO("Gowin Ready State is [%s]", (BOARD_Ready_Gowin() == 1 ? "OK" : "NOK"));
            GOWIN_Queue_Init();
        }
        if (GOWIN_DATA.MsgCount) { // handle received messages...
            comm_handler_gowin();
            GOWIN_DATA.MsgCount = 0; // release RX message buffer
            if ((GOWIN_Queue_size() == 0) && (GOWIN_DATA.verifyReadBack == true)) {
                // Gowin tx finished, let's verify the readback
                GOWIN_Compare_EdidDpcd(Main.Edid.Edid1, Main.Edid.Edid3, "Edid1==Edid3");
                GOWIN_Compare_EdidDpcd(Main.Dpcd.Dpcd1, Main.Dpcd.Dpcd3, "Dpcd1==Dpcd3");
                GOWIN_DATA.verifyReadBack = false;
                LOG_INFO("Appli%c up and running", application);
            }
        }

        // !< I2C
        i2c_update();

        // !< SPI
        spi_master_update();

        // !< GPIO request
        _handle_gpio_request();
    }

    disable_user_irq();
    BOARD_AppInitClocks(FRO96, true);
    _gdata.reboot_counter++; // !< Cold start detection
    LOG_DEBUG("*** reboot counter [%d]\t***", _gdata.reboot_counter);
    LOG_DEBUG("*** error was code [%d]\t***", _gdata.err_code);
    LOG_RAW("*** Exit application%c mainloop ***", application);

    ResetISR();

    // !< back to bootcode, this return value gets stored in _gdata.error_code
    return 0;
}
