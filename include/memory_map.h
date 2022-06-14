/**
 * @file memory_map.h
 * @brief  Memory map of the binary defined in the linkerscript
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-19
 */

#ifndef _MEMORY_MAP_H_
#define _MEMORY_MAP_H_

#include <stdint.h>
#include "LPC55S69_cm33_core0.h" // for RTC_Type

#define VTOR 0xE000ED08                 // !< Vector Table Offset Register

extern uint32_t __bootrom_start__;      // !< Start of bootrom
extern uint32_t __bootrom_size__;       // !< Bootrom size

extern uint32_t __approm0_start__;      // !< Start of approm0
extern uint32_t __approm0_size__;       // !< Approm0 size

extern uint32_t __approm1_start__;      // !< Start of approm1
extern uint32_t __approm1_size__;       // !< Approm1 size

extern uint32_t __approm_size__;        // !< Approm0+Approm1 size

extern uint32_t __bootinfo_start__;     // !< Start of bootinfo
extern uint32_t __bootinfo_size__;      // !< bootinfo size

extern uint32_t __bootinfob_start__;     // !< Start of bootinfo backup
extern uint32_t __bootinfob_size__;      // !< bootinfo size

extern uint32_t __ssram_start__;        // !< Start of Shared SRAM
extern uint32_t __ssram_size__;         // !< Size of Shared SRAM
extern uint32_t __ssram_end__;          // !< End of Shared SRAM

extern uint32_t __ssram_log_start__;    // !< Start of Shared SRAM
extern uint32_t __ssram_log_size__;     // !< Size of Shared SRAM
extern uint32_t __ssram_log_end__;      // !< End of Shared SRAM

// extern uint32_t __wflash_size__;

extern uint32_t _stext;                 // !< Start of text segment
extern uint32_t _etext;                 // !< End of text segment

extern uint32_t _sbss;                  // !< Start of bss segment
extern uint32_t _ebss;                  // !< End of bss segment

extern uint32_t _sidata;                // !< Start of init data
extern uint32_t _sdata;                 // !< Start of data segment
extern uint32_t _edata;                 // !< End of data segment

extern uint32_t _sstack;                // !< Start of stack
extern uint32_t _estack;                // !< End of stack

#define FLASH_HELPER(nme) #nme, (uint32_t)&__ ## nme ## _start__, \
    (uint32_t)&__ ## nme ## _size__

#define __maybe_unused __attribute__((unused))


typedef enum __attribute__ ((__packed__)) { // !< Force to 1 byte
    NONE = 0,
    UPDATE,
    BOOT_PARTITION_0,
    BOOT_PARTITION_1
} application_request_t;

struct ssram_data_t {
    uint8_t reboot_counter;
    uint32_t err_code;
    application_request_t application_request;
};

/**
 * @brief  Jump to a given program counter with a given stack pointer
 *
 * @param pc Program counter to which we'll jump
 * @param sp Stack pointer to which SP will be relocated
 *
 */
static inline void jump_to(__maybe_unused uint32_t pc,
                           __maybe_unused uint32_t sp) {
    __asm(
        "           \n\
            msr msp, r1 /* load r1 into MSP */\n\
            bx r0       /* branch to the address at r0 */\n"    );
}

/**
 * @brief  read the program counter
 */

static inline uint32_t get_pc_reg() {
    uint32_t pcReg;

    // The Program Counter (PC) is register R15 in arm cortex0 assembly
    __asm("mov %[pcReg], r15;" :[pcReg] "=r" (pcReg));
    return(pcReg);
}

/**
 * @brief  routine for cleaning all ssram with an offset for saving _gdata
 */

static inline void wipe_sram(uint32_t offset) {
    uint32_t * curr = (uint32_t *)&__ssram_start__;

    while (curr != (uint32_t *)&__ssram_log_start__) {
        if (offset == 0)
            *curr++ = 0x0U;       // !< wipe data
        else {
            curr++;
            offset--;
        }
    }
}

/*  put/get_RTC_GPREG()
 *
 * General purpose backup registers
 * The general purpose registers retain data through the deep power-down mode or loss of
 * main power. Only a complete removal of power from the chip or a software reset of the
 * RTC can clear the general purpose registers.
 * The RTC module offers a set of registers as backup storage, and is reset only on a
 * software reset of the RTC. These registers may be used to store critical data through deep
 * power-down mode.
 */

static inline void put_RTC_GPREG(uint8_t reg, uint32_t data) {
    if (reg > 7)
        return;

    RTC_Type * base;
    base->GPREG[reg] = data;
}

static inline uint32_t get_RTC_GPREG(uint8_t reg) {
    if (reg > 7)
        return 0xffffffff;

    RTC_Type * base;
    return base->GPREG[reg];
}

#endif /* _MEMORY_MAP_H_ */
