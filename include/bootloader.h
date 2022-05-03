/**
 * @file    bootloader.h
 * @brief   Bootloader info flash partition
 * @author  Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date    2020-08-19
 * @history 2021-06-15 (davth) typedef
 */

#ifndef _BOOTLOADER_FLASH_H_
#define _BOOTLOADER_FLASH_H_

#include <stdint.h>

#define BTLR_NR_APPS 2 //!< 2 App partition

#define BOOTLOADER_MAGIC_CRC 0xDEADBEEF

#pragma pack(push, 1)
/**
 * @brief  Application information context
 */
typedef struct app_ctxt_t {
  uint32_t  start_addr;             //!< Start address of Application
  uint32_t  application_size;       //!< Actual address size
  uint32_t  partition_size;         //!< Size of the application partition
  uint32_t  crc;                    //!< CRC of the image
} app_ctxt_t;

typedef enum {
  APP_PARTITION_NONE = -1,
  APP_PARTITION_0,
  APP_PARTITION_1,
} app_partition_t;

/**
 * @brief  Bootloader context
 */
typedef struct bootloader_ctxt_t {
  struct app_ctxt_t apps[BTLR_NR_APPS];     //!< app contexts
  app_partition_t   part;                   //!< Current APP Partition
  uint32_t          crc;                    //!< Boot info ctxt crc
} bootloader_ctxt_t;
#pragma pack(pop)

#endif /* _BOOTLOADER_FLASH_H_ */
