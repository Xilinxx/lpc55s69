/**
 * @file lcp55s69_irqs.h
 * @brief  Weak definitions for LPC55S69 IRQ's
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-18
 */

#ifndef _LPC55S69_IRQS_H_
#define _LPC55S69_IRQS_H_

void DefaultHandler();

__attribute__((weak)) void WDT_BOD_IRQHandler(void);
void WDT_BOD_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void DMA0_IRQHandler(void);
void DMA0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void GINT0_IRQHandler(void);
void GINT0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void GINT1_IRQHandler(void);
void GINT1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT0_IRQHandler(void);
void PIN_INT0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT1_IRQHandler(void);
void PIN_INT1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT2_IRQHandler(void);
void PIN_INT2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT3_IRQHandler(void);
void PIN_INT3_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void UTICK0_IRQHandler(void);
void UTICK0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void MRT0_IRQHandler(void);
void MRT0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void CTIMER0_IRQHandler(void);
void CTIMER0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void CTIMER1_IRQHandler(void);
void CTIMER1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void SCT0_IRQHandler(void);
void SCT0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void CTIMER3_IRQHandler(void);
void CTIMER3_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM0_IRQHandler(void);
void FLEXCOMM0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM1_IRQHandler(void);
void FLEXCOMM1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM2_IRQHandler(void);
void FLEXCOMM2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM3_IRQHandler(void);
void FLEXCOMM3_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM4_IRQHandler(void);
void FLEXCOMM4_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM5_IRQHandler(void);
void FLEXCOMM5_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM6_IRQHandler(void);
void FLEXCOMM6_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM7_IRQHandler(void);
void FLEXCOMM7_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void ADC0_IRQHandler(void);
void ADC0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved39_IRQHandler(void);
void Reserved39_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void ACMP_IRQHandler(void);
void ACMP_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved41_IRQHandler(void);
void Reserved41_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved42_IRQHandler(void);
void Reserved42_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void USB0_NEEDCLK_IRQHandler(void);
void USB0_NEEDCLK_IRQHandler(void) __attribute__((weak,
                                                  alias("DefaultHandler")));

__attribute__((weak)) void USB0_IRQHandler(void);
void USB0_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void RTC_IRQHandler(void);
void RTC_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved46_IRQHandler(void);
void Reserved46_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void MAILBOX_IRQHandler(void);
void MAILBOX_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT4_IRQHandler(void);
void PIN_INT4_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT5_IRQHandler(void);
void PIN_INT5_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT6_IRQHandler(void);
void PIN_INT6_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PIN_INT7_IRQHandler(void);
void PIN_INT7_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void CTIMER2_IRQHandler(void);
void CTIMER2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void CTIMER4_IRQHandler(void);
void CTIMER4_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void OS_EVENT_IRQHandler(void);
void OS_EVENT_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved55_IRQHandler(void);
void Reserved55_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved56_IRQHandler(void);
void Reserved56_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved57_IRQHandler(void);
void Reserved57_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void SDIO_IRQHandler(void);
void SDIO_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved59_IRQHandler(void);
void Reserved59_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved60_IRQHandler(void);
void Reserved60_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void Reserved61_IRQHandler(void);
void Reserved61_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void USB1_PHY_IRQHandler(void);
void USB1_PHY_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void USB1_IRQHandler(void);
void USB1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void USB1_NEEDCLK_IRQHandler(void);
void USB1_NEEDCLK_IRQHandler(void) __attribute__((weak,
                                                  alias("DefaultHandler")));

__attribute__((weak)) void SEC_HYPERVISOR_CALL_IRQHandler(void);
void SEC_HYPERVISOR_CALL_IRQHandler(void) __attribute__((weak,
                                                         alias(
                                                             "DefaultHandler")));

__attribute__((weak)) void SEC_GPIO_INT0_IRQ0_IRQHandler(void);
void SEC_GPIO_INT0_IRQ0_IRQHandler(void) __attribute__((weak,
                                                        alias(
                                                            "DefaultHandler")));

__attribute__((weak)) void SEC_GPIO_INT0_IRQ1_IRQHandler(void);
void SEC_GPIO_INT0_IRQ1_IRQHandler(void) __attribute__((weak,
                                                        alias(
                                                            "DefaultHandler")));

__attribute__((weak)) void PLU_IRQHandler(void);
void PLU_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void SEC_VIO_IRQHandler(void);
void SEC_VIO_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void HASHCRYPT_IRQHandler(void);
void HASHCRYPT_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void CASER_IRQHandler(void);
void CASER_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PUF_IRQHandler(void);
void PUF_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void PQ_IRQHandler(void);
void PQ_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void DMA1_IRQHandler(void);
void DMA1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

__attribute__((weak)) void FLEXCOMM8_IRQHandler(void);
void FLEXCOMM8_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));

#endif /* _LPC55S69_IRQS_H_ */
