/**
 * @file comm_i2c.h
 * @brief i2c communication towards the gpmcu - SPI Flash related
 * @author David Thio <david.thio@barco.com>
 * @version
 * @date 2022-Jan
 */

#ifndef _TOOLS_COMM_I2C_GPMCU_H_
#define _TOOLS_COMM_I2C_GPMCU_H_

#define I2C_DEVICE "/dev/i2c-5"  // default device if not overruled

typedef enum {
    SPI_NO_CMD       = 0x00u,
    SPI_IS25_PP      = 0x02u,
    SPI_IS25_RDSR    = 0x05u,
    SPI_IS25_READ    = 0x03u,
    SPI_IS25_CER     = 0xC7u,
    SPI_IS25_RDID    = 0x9fu,
    SPI_IS25_SE      = 0x20u,
    SPI_IS25_SE_     = 0xd7u,
    SPI_IS25_BER32   = 0x52,
    SPI_IS25_BER64   = 0xd8,
    SPI_IS25_DP      = 0xB9, // deep powerdown
    SPI_IS25_RDP     = 0xAB, // release deep sleep
    // custom commands
    SPI_IS25_VERBOSE = 0xDE,
} gp_flash_command;

typedef struct {
    gp_flash_command cmd;
    uint8_t param[3]; // Address var
} gp_flash_msg;

int open_i2c_device(const char * device);

int i2c_spi_flash_erase64k(const int eraseblocks);
int i2c_isp_flash_set_device(const char * devicename);
int i2c_spi_flash_cmd(gp_flash_msg cmd);
int i2c_spi_flash_cmd_set_page_access(bool PageAccessOn);
int i2c_spi_flash_program(const char * filename);

int i2c_isp_flash_calculate_crc(const char * filename);
int i2c_spi_flash_readback_page(const uint16_t address, int * startCRC, const uint16_t length);

#endif // _TOOLS_COMM_I2C_GPMCU_H_