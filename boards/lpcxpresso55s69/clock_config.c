/**
 * @file clock_config.c
 * @brief  Clock configuration for LPCXpresso55S69 board
 *
 * Clock configuration for LPCXpresso55S69 board.
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-27
 */

#include "clock_config.h"

#include "fsl_power.h"
#include "fsl_clock.h"

/**< System clock frequency. */
extern uint32_t SystemCoreClock;

static inline void _setup_pll_divider()
{
	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 0U, true);           /**< Reset PLL0DIV divider counter and halt it */
	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 2U, false);          /**< Set PLL0DIV divider to value 2 */
}

#define _setup_flexcomm_divider(flexid, divval) \
	{ \
		CLOCK_SetClkDiv(kCLOCK_DivFlexFrg ## flexid, 0U, true);    \
		CLOCK_SetClkDiv(kCLOCK_DivFlexFrg ## flexid, divval, false); \
	}

static inline void _setup_flexcomm_dividers()
{
	/* Set up dividers */
	_setup_flexcomm_divider(1, 400U);
	_setup_flexcomm_divider(2, 400U);
	_setup_flexcomm_divider(3, 400U);
	_setup_flexcomm_divider(4, 400U);
	_setup_flexcomm_divider(6, 400U);
}

static inline void _attach_peripherial_clocks()
{
	CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM1);        /**< Switch FLEXCOMM1 to PLL0_DIV */
	CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM0);        /**< Switch FLEXCOMM2 to FRO12M */
	CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM2);        /**< Switch FLEXCOMM2 to FRO12M */
}

static inline void _setup_main_pll_150Mhz()
{
	/** Set up the clock sources */
	/**< Configure FRO192M */
	POWER_DisablePD(kPDRUNCFG_PD_FRO192M);  /**< Ensure FRO is on  */
	CLOCK_SetupFROClocking(12000000U);      /**< Set up FRO to the 12 MHz, just for sure */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);   /**< Switch to FRO 12MHz first to ensure we can change the clock setting */

	/**< Configure fro_1m */
	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;   /**< Ensure fro_1m is on */

	POWER_SetVoltageForFreq(150000000U);                            /**< Set voltage for the one of the fastest clock outputs:
	                                                                 * System clock output */
	CLOCK_SetFLASHAccessCyclesForFreq(150000000U);                  /**< Set FLASH wait states for core */

	/**< Set up PLL */
	CLOCK_AttachClk(kFRO1M_to_PLL0);        /**< Switch PLL0CLKSEL to FRO1M */
	POWER_DisablePD(kPDRUNCFG_PD_PLL0);     /**< Ensure PLL is on  */
	POWER_DisablePD(kPDRUNCFG_PD_PLL0_SSCG);
	const pll_setup_t pll0Setup = {
		.pllctrl	= SYSCON_PLL0CTRL_CLKEN_MASK |
				  SYSCON_PLL0CTRL_SELI(26U) |
				  SYSCON_PLL0CTRL_SELP(31U),
		.pllndec	= SYSCON_PLL0NDEC_NDIV(1U),
		.pllpdec	= SYSCON_PLL0PDEC_PDIV(1U),
		.pllsscg	= { 0x0U,
				    (SYSCON_PLL0SSCG1_MDIV_EXT(300U) |
				     SYSCON_PLL0SSCG1_SEL_EXT_MASK) },
		.pllRate	= 150000000U,
		.flags		= PLL_SETUPFLAG_WAITLOCK
	};

	CLOCK_SetPLL0Freq(&pll0Setup); /**< Configure PLL0 to the desired values */

	_setup_pll_divider();

	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);           /**< Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivWdtClk, 0U, true);            /**< Reset WDTCLKDIV divider counter and halt it */
	CLOCK_SetClkDiv(kCLOCK_DivWdtClk, 1U, false);           /**< Set WDTCLKDIV divider to value 1 */

	_setup_flexcomm_dividers();

	CLOCK_AttachClk(kPLL0_to_MAIN_CLK);             /**< Switch MAIN_CLK to PLL0 */

	/* Set SystemCoreClock variable. */
	SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;
}

static inline void _setup_main_fro_96Mhz()
{
	POWER_DisablePD(kPDRUNCFG_PD_FRO192M);  /*!< Ensure FRO is on  */
	CLOCK_SetupFROClocking(12000000U);      /*!< Set up FRO to the 12 MHz, just for sure */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);   /*!< Switch to FRO 12MHz first to ensure we can change the clock setting */

	CLOCK_SetupFROClocking(96000000U);      /* Enable FRO HF(96MHz) output */

	POWER_SetVoltageForFreq(
		96000000U);                             /*!< Set voltage for the one of the fastest clock outputs: System clock output */
	CLOCK_SetFLASHAccessCyclesForFreq(96000000U);   /*!< Set FLASH wait states for core */

	/*!< Set up dividers */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false); /*!< Set AHBCLKDIV divider to value 1 */

	/*!< Set up clock selectors - Attach clocks to the peripheries */
	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK); /*!< Switch MAIN_CLK to FRO_HF */

	/*< Set SystemCoreClock variable. */
	SystemCoreClock = BOARD_BOOTCLOCKFROHF96M_CORE_CLOCK;
}

void BOARD_AppInitClocks(void)
{
	_setup_main_fro_96Mhz();
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);  /**< Setup FLEXCOMM0 clock */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);  /**< Setup FLEXCOMM0 clock */
}

void BOARD_BootInitClocks(void)
{
	_setup_main_fro_96Mhz();
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);  /**< Setup FLEXCOMM0 clock */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);  /**< Setup FLEXCOMM0 clock */
}

void BOARD_BootDeInitClocks(void)
{
}
