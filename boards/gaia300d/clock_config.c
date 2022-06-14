/**
 * @file clock_config.c
 * @brief  Clock configuration for Gaia300D board
 * @version v0.1
 * @date 2022-03-09
 * @history
 */

/*
 * How to set up clock using clock driver functions:
 *
 * 1. Setup clock sources.
 * 2. Set up wait states of the flash.
 * 3. Set up all dividers.
 * 4. Set up all selectors to provide selected clocks.
 */

#include "clock_config.h"

#include "fsl_power.h"
#include "fsl_clock.h"
#include "fsl_wwdt.h"

#include "logger.h"
/*******************************************************************************
 * Variables
 ******************************************************************************/
/**< System clock frequency. */
extern uint32_t SystemCoreClock;
bool g_Verbose;

static inline void _setup_pll_divider() {
    CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 0U, true);   /**< Reset PLL0DIV divider counter and halt it */
    CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 2U, false);  /**< Set PLL0DIV divider to value 2 */
}

#define _setup_flexcomm_divider(flexid, divval)                \
    {                                                            \
        CLOCK_SetClkDiv(kCLOCK_DivFlexFrg ## flexid, 0U, true);      \
        CLOCK_SetClkDiv(kCLOCK_DivFlexFrg ## flexid, divval, false); \
    }

static inline void _attach_peripherial_clocks() {
    // Attach Clock for 3 UART (0:Zynq2, 1:Zynq1, 3:debugPort, 5:Gowin)
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM3);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM5);
    // I2C - USB HUB
    CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM2);
    CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM6);
}

/*******************************************************************************
 * Code for BOARD_BootClockFRO12M configuration
 ******************************************************************************/
static inline void BOARD_BootClockFRO12M(void) {
#ifndef SDK_SECONDARY_CORE
    /*!< Set up the clock sources */
    /*!< Configure FRO192M */
    POWER_DisablePD(kPDRUNCFG_PD_FRO192M);          /*!< Ensure FRO is on  */
    CLOCK_SetupFROClocking(12000000U);              /*!< Set up FRO to the 12 MHz, just for sure */
    CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);           /*!< Switch to FRO 12MHz first to ensure we can change the clock setting */

    CLOCK_SetupFROClocking(96000000U);              /* Enable FRO HF(96MHz) output */

    POWER_SetVoltageForFreq(12000000U);             /*!< Set voltage for the one of the fastest clock outputs: System clock output */
    CLOCK_SetFLASHAccessCyclesForFreq(12000000U);   /*!< Set FLASH wait states for core */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false); /*!< Set AHBCLKDIV divider to value 1 */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(kFRO12M_to_MAIN_CLK); /*!< Switch MAIN_CLK to FRO12M */

    /*< Set SystemCoreClock variable. */
    SystemCoreClock = BOARD_BOOTCLOCKFRO12M_CORE_CLOCK;

    if (g_Verbose) {
        LOG_DEBUG("BOARD_BootClockFRO12M Activated");
    }
#endif
}

/*******************************************************************************
 * Code for BOARD_BootClockFROHF96M configuration
 ******************************************************************************/
static inline void BOARD_BootClockFROHF96M(void) {
#ifndef SDK_SECONDARY_CORE
    /*!< Set up the clock sources */
    /*!< Configure FRO192M */
    POWER_DisablePD(kPDRUNCFG_PD_FRO192M);          /*!< Ensure FRO is on  */
    CLOCK_SetupFROClocking(12000000U);              /*!< Set up FRO to the 12 MHz, just for sure */
    CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);           /*!< Switch to FRO 12MHz first to ensure we can change the clock setting */

    CLOCK_SetupFROClocking(96000000U);              /*!< Enable FRO HF(96MHz) output */

    POWER_SetVoltageForFreq(96000000U);             /*!< Set voltage for the one of the fastest clock outputs: System clock output */
    CLOCK_SetFLASHAccessCyclesForFreq(96000000U);   /*!< Set FLASH wait states for core */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);   /*!< Set AHBCLKDIV divider to value 1 */
    /*!< create 16Mhz Clk for Gowin */
    CLOCK_SetClkDiv(kCLOCK_DivMClk, 0U, true);      /*!< Reset MCLKDIV divider counter and halt it */
    CLOCK_SetClkDiv(kCLOCK_DivMClk, 6U, false);     /*!< Set MCLKDIV divider to value 6 */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);   /*!< Switch MAIN_CLK to FRO_HF */
    CLOCK_AttachClk(kFRO_HF_to_MCLK);       /*!< Switch MCLK to FRO_HF */

    /*< Set SystemCoreClock variable. */
    SystemCoreClock = BOARD_BOOTCLOCKFROHF96M_CORE_CLOCK;

    if (g_Verbose) {
        LOG_DEBUG("BOARD_BootClockPLL96M Activated");
    }
#endif
}

/*******************************************************************************
 * Code for BOARD_BootClockPLL100M configuration
 ******************************************************************************/
static inline void BOARD_BootClockPLL100M(void) {
#ifndef SDK_SECONDARY_CORE
    /*!< Set up the clock sources */
    /*!< Configure FRO192M */
    POWER_DisablePD(kPDRUNCFG_PD_FRO192M);  /*!< Ensure FRO is on  */
    CLOCK_SetupFROClocking(12000000U);      /*!< Set up FRO to the 12 MHz, just for sure */
    CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);   /*!< Switch to FRO 12MHz first to ensure we can change the clock setting */

    CLOCK_SetupFROClocking(96000000U);      /* Enable FRO HF(96MHz) output */

    /*!< Configure XTAL32M */
    POWER_DisablePD(kPDRUNCFG_PD_XTAL32M);                                  /* Ensure XTAL32M is powered */
    POWER_DisablePD(kPDRUNCFG_PD_LDOXO32M);                                 /* Ensure XTAL32M is powered */
    CLOCK_SetupExtClocking(16000000U);                                      /* Enable clk_in clock */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK;                 /* Enable clk_in from XTAL32M clock  */
    ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;   /* Enable clk_in to system  */

    POWER_SetVoltageForFreq(100000000U);                                    /*!< Set voltage for the one of the fastest clock outputs: System clock output */
    CLOCK_SetFLASHAccessCyclesForFreq(100000000U);                          /*!< Set FLASH wait states for core */

    /*!< Set up PLL */
    CLOCK_AttachClk(kEXT_CLK_to_PLL0);  /*!< Switch PLL0CLKSEL to EXT_CLK */
    POWER_DisablePD(kPDRUNCFG_PD_PLL0); /* Ensure PLL is on  */
    POWER_DisablePD(kPDRUNCFG_PD_PLL0_SSCG);
    const pll_setup_t pll0Setup = {
        .pllctrl = SYSCON_PLL0CTRL_CLKEN_MASK | SYSCON_PLL0CTRL_SELI(53U) | SYSCON_PLL0CTRL_SELP(
            26U),
        .pllndec = SYSCON_PLL0NDEC_NDIV(4U),
        .pllpdec = SYSCON_PLL0PDEC_PDIV(2U),
        .pllsscg = { 0x0U, (SYSCON_PLL0SSCG1_MDIV_EXT(100U) |
                            SYSCON_PLL0SSCG1_SEL_EXT_MASK) },
        .pllRate = 100000000U,
        .flags   = PLL_SETUPFLAG_WAITLOCK
    };
    CLOCK_SetPLL0Freq(&pll0Setup); /*!< Configure PLL0 to the desired values */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false); /*!< Set AHBCLKDIV divider to value 1 */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(kPLL0_to_MAIN_CLK); /*!< Switch MAIN_CLK to PLL0 */

    /*< Set SystemCoreClock variable. */
    SystemCoreClock = BOARD_BOOTCLOCKPLL100M_CORE_CLOCK;

    if (g_Verbose) {
        LOG_DEBUG("BOARD_BootClockPLL100M Activated");
    }
#endif
}

/*******************************************************************************
 * Code for BOARD_BootClockPLL150M configuration
 ******************************************************************************/
static inline void BOARD_BootClockPLL150M(void) {
#ifndef SDK_SECONDARY_CORE
    /*!< Set up the clock sources */
    /*!< Configure FRO192M */
    POWER_DisablePD(kPDRUNCFG_PD_FRO192M);  /*!< Ensure FRO is on  */
    CLOCK_SetupFROClocking(12000000U);      /*!< Set up FRO to the 12 MHz, just for sure */
    CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);   /*!< Switch to FRO 12MHz first to ensure we can change the clock setting */

    /*!< Configure XTAL32M */
    POWER_DisablePD(kPDRUNCFG_PD_XTAL32M);                                  /* Ensure XTAL32M is powered */
    POWER_DisablePD(kPDRUNCFG_PD_LDOXO32M);                                 /* Ensure XTAL32M is powered */
    CLOCK_SetupExtClocking(16000000U);                                      /* Enable clk_in clock */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK;                 /* Enable clk_in from XTAL32M clock  */
    ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;   /* Enable clk_in to system  */

    POWER_SetVoltageForFreq(150000000U);                                    /*!< Set voltage for the one of the fastest clock outputs: System clock output */
    CLOCK_SetFLASHAccessCyclesForFreq(150000000U);                          /*!< Set FLASH wait states for core */

    /*!< Set up PLL */
    CLOCK_AttachClk(kEXT_CLK_to_PLL0);  /*!< Switch PLL0CLKSEL to EXT_CLK */
    POWER_DisablePD(kPDRUNCFG_PD_PLL0); /* Ensure PLL is on  */
    POWER_DisablePD(kPDRUNCFG_PD_PLL0_SSCG);
    const pll_setup_t pll0Setup = {
        .pllctrl = SYSCON_PLL0CTRL_CLKEN_MASK | SYSCON_PLL0CTRL_SELI(53U) | SYSCON_PLL0CTRL_SELP(
            31U),
        .pllndec = SYSCON_PLL0NDEC_NDIV(8U),
        .pllpdec = SYSCON_PLL0PDEC_PDIV(1U),
        .pllsscg = { 0x0U, (SYSCON_PLL0SSCG1_MDIV_EXT(150U) |
                            SYSCON_PLL0SSCG1_SEL_EXT_MASK) },
        .pllRate = 150000000U,
        .flags   = PLL_SETUPFLAG_WAITLOCK
    };
    CLOCK_SetPLL0Freq(&pll0Setup); /*!< Configure PLL0 to the desired values */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false); /*!< Set AHBCLKDIV divider to value 1 */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(kPLL0_to_MAIN_CLK); /*!< Switch MAIN_CLK to PLL0 */

    /*< Set SystemCoreClock variable. */
    SystemCoreClock = BOARD_BOOTCLOCKPLL150M_CORE_CLOCK;

    if (g_Verbose) {
        LOG_DEBUG("BOARD_BootClockPLL150M Activated");
    }
#endif
}

/*******************************************************************************
 * Code for BOARD_AppInitClocks
 ******************************************************************************/
void BOARD_AppInitClocks(enum bootClock Clk, bool verbose) {
    g_Verbose = verbose;
    switch (Clk) {
        case FRO12:
            BOARD_BootClockFRO12M();
            break;

        case FRO96:
            BOARD_BootClockFROHF96M();
            break;

        case PLL100:
            BOARD_BootClockPLL100M();
            break;

        case PLL150:
        default:
            BOARD_BootClockPLL150M();
            break;
    }

    _attach_peripherial_clocks();
}

/*******************************************************************************
 * Code for BOARD_BootInitClocks configuration
 ******************************************************************************/
void BOARD_BootInitClocks(void) {
    /*!< Boot needs to access flash, flash operations must happen at  <= 100 MHz */
    BOARD_BootClockFROHF96M();

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);   /*!< Switch MAIN_CLK to FRO_HF */

    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);  // 2nd CPU
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);  // Main cpu
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM3);  // debug uart
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);  // i2c usb hub
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM6);  // i2c maincpu
}

/*******************************************************************************
 * Code for BOARD_WWDT_init configuration
 ******************************************************************************/
void BOARD_WWDT_init(wwdt_config_t * config) {
    uint32_t wdtFreq;

    /* Enable FRO 1M clock for WWDT module. */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;

    /* Set clock divider for WWDT clock source. */
    CLOCK_SetClkDiv(kCLOCK_DivWdtClk, 1U, true);

#if !defined(FSL_FEATURE_WWDT_HAS_NO_PDCFG) || (!FSL_FEATURE_WWDT_HAS_NO_PDCFG)
    POWER_DisablePD(kPDRUNCFG_PD_WDT_OSC);
#endif

    /* Check if reset is due to Watchdog */
    if (WWDT_GetStatusFlags(WWDT) & kWWDT_TimeoutFlag) {
        LOG_WARN("Watchdog reset occurred");
    }

    /* The timeout flag can only clear when and after wwdt intial. */
    wdtFreq = CLOCK_GetWdtClkFreq() / 4;

    WWDT_GetDefaultConfig(config);
    /*
     * Set watchdog feed time constant to approximately 4s
     * Set watchdog warning time to 512 ticks after feed time constant
     * Set watchdog window time disabled
     */
    config->timeoutValue = wdtFreq * 4;
    config->warningValue = 512;
    config->windowValue = 0xffffff;
    /* Configure WWDT to go to IRQ on timeout , instead of reset */
    config->enableWatchdogReset = false;
    /* Setup watchdog clock frequency(Hz). */
    config->clockFreq_Hz = CLOCK_GetWdtClkFreq();

    WWDT_Init(WWDT, config);

    NVIC_EnableIRQ(WDT_BOD_IRQn);
}

/*******************************************************************************
 * Code for BOARD_WWDT disable
 ******************************************************************************/
void BOARD_WWDT_deinit() {
    NVIC_DisableIRQ(WDT_BOD_IRQn);
    WWDT_Deinit(WWDT);
}

void BOARD_WWDT_disable(wwdt_config_t * config) {
    config->enableWwdt = false;
    WWDT_Init(WWDT, config);
}

void BOARD_WWDT_enable(wwdt_config_t * config) {
    config->enableWwdt = true;
    WWDT_Init(WWDT, config);
}
