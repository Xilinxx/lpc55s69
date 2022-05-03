/**
 * @file startup.c
 * @brief  Cleaned up version of NXP's startup code for application portion
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-18
 */

#include <stdint.h>

#include "logger.h"
#include "lpc55s69_irqs.h"

#include "peripherals.h"

#include "memory_map.h"

extern int main(void);
extern void _vStackTop(void);
extern struct ssram_data_t _gdata;

void ResetISR(void)
{
  /* Disable IRQ's */
  __asm volatile ("cpsid i");

  /* Relocate the Vector Table */
  uint32_t *vector_table = (uint32_t *)&_stext;
  uint32_t *vtor = (uint32_t *)VTOR;
  uint32_t oldvtor = *vtor; //!< Store old vtor for bootloader

  *vtor = ((uint32_t)vector_table & 0xFFFFFFF8);

  uint32_t *init_values_ptr = &_sidata;
  uint32_t *data_ptr = &_sdata;

  /* If this is equal there are no variables to be
   * Initialized in the data segment */
  if (init_values_ptr != data_ptr) {
    for (; data_ptr < &_edata;) {
      /* Copy the data */
      *data_ptr++ = *init_values_ptr++;
    }
  }

  /* Clear the BSS segment */
  for (uint32_t *bss_ptr = &_sbss; bss_ptr < &_ebss;) {
    *bss_ptr++ = 0;
  }

  /* Enable IRQ's */
  __asm volatile ("cpsie i");

#define MAX_REBOOT_RETRIES 250

  if (_gdata.reboot_counter < MAX_REBOOT_RETRIES) {
    _gdata.err_code = (uint32_t)main();
  }
  else {
    LOG_WARN("Max reboot retries %d reached.", MAX_REBOOT_RETRIES);
  }

  /* Hand back some data to the bootloader */
  /* Probably should restore the bootloader's vector_table here?... */

  *vtor = oldvtor;        // Restore vtable before jumping too bootloader

  LOG_INFO("*** Vector table restored, back to bootcode ");
  LOG_RAW("\n");
  //NVIC_SystemReset();     //!< Run NVIC Reset!
}

void NMI_Handler(void)
{
  LOG_ERROR("NMI Fault");
  while (1) {
  }
}

void HardFault_Handler(void)
{
  LOG_ERROR("Hardfault! %x", SCB->SHCSR);
  _gdata.reboot_counter++;
  _gdata.err_code = 100;
  BOARD_AppDeinitPeripherals();
  SCB->SHCSR = (SCB->SHCSR & ~SCB_SHCSR_HARDFAULTACT_Msk);
  ResetISR();
}

void MemManage_Handler(void)
{
  LOG_ERROR("MemManage fault! %x", SCB->SHCSR);
  _gdata.reboot_counter++;
  _gdata.err_code = 100;

  BOARD_AppDeinitPeripherals();
  ResetISR();
}

void BusFault_Handler(void)
{
  LOG_ERROR("Busfault! %x", SCB->SHCSR);
  _gdata.reboot_counter++;
  _gdata.err_code = 100;
  BOARD_AppDeinitPeripherals();
  SCB->SHCSR = (SCB->SHCSR & ~SCB_SHCSR_BUSFAULTACT_Msk);
  ResetISR();
}

void UsageFault_Handler(void)
{
  LOG_ERROR("UsageFault!");
  while (1) {
  }
}

void SecureFault_Handler(void)
{
  LOG_ERROR("Secure fault!");
  _gdata.reboot_counter++;
  _gdata.err_code = 100;

  BOARD_AppDeinitPeripherals();
  ResetISR();

  while (1) {
  }
}

__attribute__((weak)) void SVC_Handler(void)
{
  while (1) {
  }
}

__attribute__((weak)) void DebugMon_Handler(void)
{
  while (1) {
  }
}

__attribute__((weak)) void PendSV_Handler(void)
{
  while (1) {
  }
}

__attribute__((weak)) void SysTick_Handler(void)
{
  while (1) {
  }
}

void DefaultHandler(void)
{
  while (1) {
  }
}

__attribute__ ((used, section(".isr_vector")))
void(*const g_pfnVectors[])(void) = {
  // Core Level - CM33
  &_vStackTop,                    // The initial stack pointer
  ResetISR,                       // The reset handler
  NMI_Handler,                    // The NMI handler
  HardFault_Handler,              // The hard fault handler
  MemManage_Handler,              // The MPU fault handler
  BusFault_Handler,               // The bus fault handler
  UsageFault_Handler,             // The usage fault handler
  SecureFault_Handler,            // The secure fault handler
  0,                              // ECRP
  0,                              // Reserved
  0,                              // Reserved
  SVC_Handler,                    // SVCall handler
  DebugMon_Handler,               // Debug monitor handler
  0,                              // Reserved
  PendSV_Handler,                 // The PendSV handler
  SysTick_Handler,                // The SysTick handler

  WDT_BOD_IRQHandler,             // 16: Windowed watchdog timer, Brownout detect, Flash interrupt
  DMA0_IRQHandler,                // 17: DMA0 controller
  GINT0_IRQHandler,               // 18: GPIO group 0
  GINT1_IRQHandler,               // 19: GPIO group 1
  PIN_INT0_IRQHandler,            // 20: Pin interrupt 0 or pattern match engine slice 0
  PIN_INT1_IRQHandler,            // 21: Pin interrupt 1or pattern match engine slice 1
  PIN_INT2_IRQHandler,            // 22: Pin interrupt 2 or pattern match engine slice 2
  PIN_INT3_IRQHandler,            // 23: Pin interrupt 3 or pattern match engine slice 3
  UTICK0_IRQHandler,              // 24: Micro-tick Timer
  MRT0_IRQHandler,                // 25: Multi-rate timer
  CTIMER0_IRQHandler,             // 26: Standard counter/timer CTIMER0
  CTIMER1_IRQHandler,             // 27: Standard counter/timer CTIMER1
  SCT0_IRQHandler,                // 28: SCTimer/PWM
  CTIMER3_IRQHandler,             // 29: Standard counter/timer CTIMER3
  FLEXCOMM0_IRQHandler,           // 30: Flexcomm Interface 0 (USART, SPI, I2C, I2S, FLEXCOMM)
  FLEXCOMM1_IRQHandler,           // 31: Flexcomm Interface 1 (USART, SPI, I2C, I2S, FLEXCOMM)
  FLEXCOMM2_IRQHandler,           // 32: Flexcomm Interface 2 (USART, SPI, I2C, I2S, FLEXCOMM)
  FLEXCOMM3_IRQHandler,           // 33: Flexcomm Interface 3 (USART, SPI, I2C, I2S, FLEXCOMM)
  FLEXCOMM4_IRQHandler,           // 34: Flexcomm Interface 4 (USART, SPI, I2C, I2S, FLEXCOMM)
  FLEXCOMM5_IRQHandler,           // 35: Flexcomm Interface 5 (USART, SPI, I2C, I2S, FLEXCOMM)
  FLEXCOMM6_IRQHandler,           // 36: Flexcomm Interface 6 (USART, SPI, I2C, I2S, FLEXCOMM)
  FLEXCOMM7_IRQHandler,           // 37: Flexcomm Interface 7 (USART, SPI, I2C, I2S, FLEXCOMM)
  ADC0_IRQHandler,                // 38: ADC0
  Reserved39_IRQHandler,          // 39: Reserved interrupt
  ACMP_IRQHandler,                // 40: ACMP  interrupts
  Reserved41_IRQHandler,          // 41: Reserved interrupt
  Reserved42_IRQHandler,          // 42: Reserved interrupt
  USB0_NEEDCLK_IRQHandler,        // 43: USB Activity Wake-up Interrupt
  USB0_IRQHandler,                // 44: USB device
  RTC_IRQHandler,                 // 45: RTC alarm and wake-up interrupts
  Reserved46_IRQHandler,          // 46: Reserved interrupt
  MAILBOX_IRQHandler,             // 47: WAKEUP,Mailbox interrupt (present on selected devices)
  PIN_INT4_IRQHandler,            // 48: Pin interrupt 4 or pattern match engine slice 4 int
  PIN_INT5_IRQHandler,            // 49: Pin interrupt 5 or pattern match engine slice 5 int
  PIN_INT6_IRQHandler,            // 50: Pin interrupt 6 or pattern match engine slice 6 int
  PIN_INT7_IRQHandler,            // 51: Pin interrupt 7 or pattern match engine slice 7 int
  CTIMER2_IRQHandler,             // 52: Standard counter/timer CTIMER2
  CTIMER4_IRQHandler,             // 53: Standard counter/timer CTIMER4
  OS_EVENT_IRQHandler,            // 54: OSEVTIMER0 and OSEVTIMER0_WAKEUP interrupts
  Reserved55_IRQHandler,          // 55: Reserved interrupt
  Reserved56_IRQHandler,          // 56: Reserved interrupt
  Reserved57_IRQHandler,          // 57: Reserved interrupt
  SDIO_IRQHandler,                // 58: SD/MMC
  Reserved59_IRQHandler,          // 59: Reserved interrupt
  Reserved60_IRQHandler,          // 60: Reserved interrupt
  Reserved61_IRQHandler,          // 61: Reserved interrupt
  USB1_PHY_IRQHandler,            // 62: USB1_PHY
  USB1_IRQHandler,                // 63: USB1 interrupt
  USB1_NEEDCLK_IRQHandler,        // 64: USB1 activity
  SEC_HYPERVISOR_CALL_IRQHandler, // 65: SEC_HYPERVISOR_CALL interrupt
  SEC_GPIO_INT0_IRQ0_IRQHandler,  // 66: SEC_GPIO_INT0_IRQ0 interrupt
  SEC_GPIO_INT0_IRQ1_IRQHandler,  // 67: SEC_GPIO_INT0_IRQ1 interrupt
  PLU_IRQHandler,                 // 68: PLU interrupt
  SEC_VIO_IRQHandler,             // 69: SEC_VIO interrupt
  HASHCRYPT_IRQHandler,           // 70: HASHCRYPT interrupt
  CASER_IRQHandler,               // 71: CASPER interrupt
  PUF_IRQHandler,                 // 72: PUF interrupt
  PQ_IRQHandler,                  // 73: PQ interrupt
  DMA1_IRQHandler,                // 74: DMA1 interrupt
  FLEXCOMM8_IRQHandler,           // 75: Flexcomm Interface 8 (SPI, , FLEXCOMM)
};                                      /* End of g_pfnVectors */
