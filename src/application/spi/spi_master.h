#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#undef EXTERN
#ifdef _SPI_MASTER_C_
#define EXTERN
#else
#define EXTERN extern
#endif

typedef enum
{
  SPI_NO_CMD = 0x00u,
  SPI_IS25_PP = 0x02u,
  SPI_IS25_RDSR = 0x05u,
  SPI_IS25_READ = 0x03u,
  SPI_IS25_CER = 0xC7u,
  SPI_IS25_RDID = 0x9fu,
  SPI_IS25_SE = 0x20u,
  SPI_IS25_SE_ = 0xd7u,
  SPI_IS25_BER32 = 0x52,
  SPI_IS25_BER64 = 0xd8,
  SPI_IS25_DP = 0xB9, // deep powerdown
  SPI_IS25_RDP = 0xAB, // release deep sleep
  // custom commands
  SPI_IS25_VERBOSE = 0xDE,
}
t_spi_command;

typedef struct __attribute__((packed))
{
  t_spi_command cmd;
  u8 param[3];  // Address var
  u8 data[256]; // for Page Program data
} t_spi_msg;

#define SPI_MSG_QUEUE_LENGTH 5
#define SPI_BUFFER_SIZE 256

/* SPI buffer */
#define BUFFER_SIZE (10)
typedef struct __attribute__((packed))
{
  u8 txBuff[BUFFER_SIZE];
  u8 rxBuff[BUFFER_SIZE];
  u16 txIndex;
  u16 rxIndex;
  volatile bool masterFinished;
} SpiBuffer_t;

/*
 * @brief spi_master_setup
 * setup and initialize SPI
 * @return void
 */
void spi_master_setup();

/*
 * @brief spi_master_disable
 * Disable the SPI IRQ
 */
void spi_master_disable();

/*
 * @brief spi_master_update
 * Called from main routin to check for queued messages
 * Takes care of disabling the MUX-select when no data needs to be sent
 * @return void
 */
void spi_master_update();

void spi_read_page(u16 startpage, bool print);
EXTERN void spi_queue_msg_param(u8 *param, u16 size);
EXTERN void spi_queue_msg(t_spi_command cmd);
void spi_queue_process();
u8 spi_queue_size(bool verbose);

#endif //__SPI_MASTER_H__
