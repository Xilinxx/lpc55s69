#include <fcntl.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "crc_linux.h"
#include "logger.h"


#include "comm_i2c_gpmcu.h"

#define I2C_DEVICE_NAME_LENGTH 15
#define I2C_ADDRESS 0x60      // the gpmcu-device command interface
#define I2C_ADDRESS_BIS 0x61  // gpmcu-device - readback of spi flash pages
#define I2C_FLASH_CMD_IDENTIFIER (0x50u)
#define I2C_DATA_LENGTH (256) // MAX is 256

static int fd;
static uint32_t crc_input_file;  // CRC calculation of the input file
static char i2c_device_name[I2C_DEVICE_NAME_LENGTH] = I2C_DEVICE; // set i2c device

int i2c_isp_flash_set_device(const char *devicename)
{
	LOG_DEBUG("checking i2c device [%s] size[%d]",devicename, strlen(devicename));
	if(strlen(devicename)>I2C_DEVICE_NAME_LENGTH) {
		LOG_ERROR("selected i2c device is oversized > %d",I2C_DEVICE_NAME_LENGTH);
		return -1;
	}
	if (open_i2c_device(devicename) > 0)
	{
		strncpy(i2c_device_name, devicename, I2C_DEVICE_NAME_LENGTH);
		i2c_device_name[I2C_DEVICE_NAME_LENGTH-1] = '\0';
		close(fd);
	}
	else // error opening device
		return -1;

	LOG_INFO("selected i2c device [%s]",i2c_device_name);
	return 0;
}

/*!
 * @brief open_i2c_device()
 * Opens the specified I2C device.
 * @param device the i2c device to access
 * @return a non-negative file descriptor on success, or -1 on failure.
 */
int open_i2c_device(const char * device)
{
  int fd = open(device, O_RDWR);
  if (fd == -1)
  {
    perror(device);
    return -1;
  }
  return fd;
}

/*!
 * @brief i2c_spi_flash_cmd()
 * Generic command for 1 byte or SPI_IS25_READ access.
 * @param cmd the implemented spi_flash cmd from the datasheet
 * @return a non-negative on success, or -1 on failure.
 */
int i2c_spi_flash_cmd(gp_flash_msg cmd)
{
	fd = open_i2c_device(i2c_device_name);
	if (fd < 0) {
		LOG_ERROR("i2c device %s missing", i2c_device_name);
		return -1;
	}

	int result;
	struct i2c_msg message= { I2C_ADDRESS, 0, 0, 0};
	struct i2c_rdwr_ioctl_data ioctl_data = { 0, 1 };
	uint8_t command[4];
	switch(cmd.cmd) {
		case SPI_IS25_READ: // 4 byte cmd
			LOG_INFO("SPI_IS25_READ");
		case SPI_IS25_VERBOSE:
			command[0] = (uint8_t)I2C_FLASH_CMD_IDENTIFIER;
			command[1] = (uint8_t)(cmd.cmd);
			command[2] = (uint8_t)(cmd.param[0]);
			command[3] = (uint8_t)(cmd.param[1]);
			message.len = 4;
			message.buf = command;
			ioctl_data.msgs = &message;
			break;
		default: // 2 byte cmds
			command[0] = (uint8_t)I2C_FLASH_CMD_IDENTIFIER;
			command[1] = (uint8_t)(cmd.cmd);
			message.len = 2;
			message.buf = command;
			ioctl_data.msgs = &message;
			break;
		}

	result = ioctl(fd, I2C_RDWR, &ioctl_data);

	close(fd);

	if (result != 1)
	{
    perror("failed to write i2c cmd");
    return -1;
  }
  return 0;
}

/*!
 * @brief i2c_spi_flash_cmd_set_page_access() on 0x61
 * Specific command for Enabling block reads on 0x61
 * @param PageAccessOn 1:On(=256byte) , 0:Off(=1byte)
 * @return a non-negative on success, or -1 on failure.
 */
int i2c_spi_flash_cmd_set_page_access(bool PageAccessOn)
{
	fd = open_i2c_device(i2c_device_name);
	if (fd < 0) {
		LOG_ERROR("i2c device %s missing", i2c_device_name);
		return -1;
	}

	int result;
	struct i2c_msg message= { I2C_ADDRESS_BIS, 0, 0, 0};
	struct i2c_rdwr_ioctl_data ioctl_data = { 0, 1 };
	uint8_t command[2];
	command[0] = (uint8_t)0xff;
	command[1] = (uint8_t)((PageAccessOn)?0x01:0x00);
	message.len = 2;
	message.buf = command;
	ioctl_data.msgs = &message;

	result = ioctl(fd, I2C_RDWR, &ioctl_data);

	close(fd);

	if (result != 1)
	{
    perror("failed to write i2c cmd");
    return -1;
  }
  return 0;
}


/*!
 * @brief i2c_spi_flash_erase()
 * erase in blocks of 64k
 * @param eraseblocks nr of blocks to erase
 * @return a non-negative on success, or -1 on failure.
 */
int i2c_spi_flash_erase64k(const int eraseblocks)
{
	fd = open_i2c_device(i2c_device_name);
	if (fd < 0) {
		LOG_ERROR("i2c device %s missing", i2c_device_name);
		return 0;
	}

	uint16_t erase_address = 0;
	LOG_INFO("Erase %d blocks of 64k starting at adr[0x%X]", eraseblocks, erase_address);

	for (int i = 0; i <= eraseblocks; i++)
	{
		uint8_t address_high_byte = (uint8_t)(erase_address>>8);
		uint8_t address_low_byte = (uint8_t)(erase_address&0xff);
		uint8_t command[] = {(uint8_t)I2C_FLASH_CMD_IDENTIFIER,
												(uint8_t)SPI_IS25_BER64,
												address_high_byte, address_low_byte };  // dummy byte
		struct i2c_msg message = {I2C_ADDRESS, 0, sizeof(command), command};
		struct i2c_rdwr_ioctl_data ioctl_data = {&message, 1};
		int result = ioctl(fd, I2C_RDWR, &ioctl_data);
		LOG_DEBUG("Erasing 64K Block: %d , AdrHiByte[0x%X]\t result:%d", i, address_high_byte, result);
		usleep(500 * 1e3); // Erasing these blocks takes some time, full chip erase is over 30s.
		erase_address += 0x100; //next 64k
	}

	close(fd);
}


/*!
 * @brief i2c_spi_flash_program()
 * Complete cycle of erase and write a binary firmware file
 * @param filename the file reference
 * @return a non-negative on success, or -1 on failure.
 */
int i2c_spi_flash_program(const char * filename)
{
	// file check
	int fp = open(filename, O_RDONLY);
	if (fp < 0) {
		LOG_ERROR("Failed to open %s bin file",filename);
		return -1;
	}

	struct stat st;
	fstat(fp, &st);

	char *buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fp, 0);
	if (buf == MAP_FAILED) {
		LOG_ERROR("Failed to mmap file");
		return -1;
	}

	crc_input_file = calculate_bin_crc(filename);
	LOG_INFO("File(%s)-CRC32: %x", filename, crc_input_file);

	i2c_spi_flash_cmd_set_page_access(true);

	// i2c part starts here
	fd = open_i2c_device(i2c_device_name);
	if (fd < 0) {
		LOG_ERROR("i2c device %s missing", i2c_device_name);
		return 0;
	}

	// Step1: Erase the needed amout of blocks per 64K
	const int eraseblocks = (st.st_size / 65535) + 1;
	uint16_t erase_address = 0;
	LOG_INFO("Erase %d blocks of 64k starting at adr[0x%X]", eraseblocks, erase_address);
	// Erase needed blocks
	for (int i = 0; i <= eraseblocks; i++)
	{
		uint8_t address_high_byte = (uint8_t)(erase_address>>8);
		uint8_t address_low_byte = (uint8_t)(erase_address&0xff);
		uint8_t command[] = {(uint8_t)I2C_FLASH_CMD_IDENTIFIER,
												(uint8_t)SPI_IS25_BER64,
												address_high_byte, address_low_byte };  // dummy byte
		struct i2c_msg message = {I2C_ADDRESS, 0, sizeof(command), command};
		struct i2c_rdwr_ioctl_data ioctl_data = {&message, 1};
		int result = ioctl(fd, I2C_RDWR, &ioctl_data);
		LOG_DEBUG("Erasing 64K Block: %d , AdrHiByte[0x%X]\t result:%d", i, address_high_byte, result);
		usleep(500 * 1e3); // Erasing these blocks takes some time, full chip erase is over 30s.
		erase_address += 0x100; //next 64k
	}

	// Step2: Program per page(256bytes)
	const int blocks = st.st_size / I2C_DATA_LENGTH;
	LOG_INFO("Program %d[0x%X] blocks of 256bytes", blocks, blocks);

	for (int i = 0; i <= blocks; i++)
	{
		int offset_in_file = i * I2C_DATA_LENGTH;
		uint8_t cnt_low_byte = i & 0xFF;
		uint8_t cnt_hi_byte = i >> 8;
		// Page program cmd
		uint8_t command[] = {(uint8_t)I2C_FLASH_CMD_IDENTIFIER,
												 (uint8_t)SPI_IS25_PP,
												 cnt_hi_byte,
												 cnt_low_byte };
		struct i2c_msg message = {I2C_ADDRESS, 0, sizeof(command), command};
		struct i2c_rdwr_ioctl_data ioctl_data = {&message, 1};
		int result = ioctl(fd, I2C_RDWR, &ioctl_data);

		// The actual data
		uint8_t data[I2C_DATA_LENGTH] = {0xff}; // All 0xff
		for (int j = 0; j < I2C_DATA_LENGTH; j++)
		{
			data[j] = (uint8_t)buf[offset_in_file + j];
		}
		struct i2c_msg msgdata = {I2C_ADDRESS, 0, sizeof(data), data};
		ioctl_data.msgs = &msgdata;
		ioctl_data.nmsgs = 1;
		int resultdata = ioctl(fd, I2C_RDWR, &ioctl_data);
		if ((resultdata == -1) || (result == -1)) {
			LOG_DEBUG("Flashed Block: %d/%d \t i2c returned cmd[%d] datawrite[%d]", i, blocks, result, resultdata);
			LOG_ERROR("i2c failure");
			close(fd);
			return -1;
		}
		if( i%100==0 || i==blocks ) {
			LOG_DEBUG("Flashed Block: %d/%d", i, blocks);
		}
	}
	close(fd);

	i2c_spi_flash_cmd_set_page_access(false);
}

//------------------------------------------------------------------------------

/*!
 * @brief i2c_spi_flash_readback_page()
 * We can read back per 256byte of fetched data from flash
 * @param address - block of 256bytes to read
 * @param * startCRC - for calculation
 * @param length - amount of bytes to check (only used for crc calculation)
 * @return a non-negative on success, or -1 on failure.
 */
int i2c_spi_flash_readback_page(const uint16_t address, int * startCRC, const uint16_t length)
{
	int ret = 0;
	fd = open(i2c_device_name, O_RDWR);
	if (fd < 0)
	{
		LOG_ERROR("i2c device %s missing", i2c_device_name);
		return -1;
	}

	if (length>I2C_DATA_LENGTH)
	{
		LOG_ERROR("i2c not reading more then %d bytes!",I2C_DATA_LENGTH);
		close(fd);
		return -2;
	}

	unsigned char cblock[I2C_DATA_LENGTH];
	memset(cblock, 0x00, sizeof(cblock));

	// trigger the readback to buffer, available on i2c adr 0x61
	uint8_t command1[] = { (uint8_t)I2C_FLASH_CMD_IDENTIFIER,
												(uint8_t)SPI_IS25_READ,
												(uint8_t)(address >> 8),
												(uint8_t)(address & 0xff) };
	struct i2c_msg messages[] = {
			{I2C_ADDRESS, 0, sizeof(command1), command1}, // read from flash
			//{I2C_ADDRESS_BIS, I2C_SLAVE_FORCE , length, cblock }, //!!! not functional
	};
	struct i2c_rdwr_ioctl_data ioctl_data = { messages, 1 };
	int result = ioctl(fd, I2C_RDWR , &ioctl_data);
	if (result < 0 || result != 1)
		LOG_ERROR("ioctl call returned = %X", result);

  // Give the Gpmcu time to read the new page
	usleep(30 * 1e3);

	// change the slave addressing

	result = ioctl(fd, I2C_SLAVE, I2C_ADDRESS_BIS);
	if (result < 0)
		LOG_ERROR("ioctl call returned = %X", result);

	int i = 0;
	// Important: we must read per 256 bytes!
	// The gp page-read implementation demands 256 byte read (=faster than byte)
	// Anyhow the kernel reads per 32 bytes and we should follow this amount
	// to prevent getting read-errors
	const uint8_t BLOCK_SIZE = 32;
	for (int res = 0; res < I2C_DATA_LENGTH; res += i)
	{
		// Reading more than 1 byte does not work -> results in 0xff
		i = i2c_smbus_read_i2c_block_data(fd, res, BLOCK_SIZE, cblock + res);
		if (i <= 0) {
			res = i;
			LOG_ERROR("i2c read error %d",i);
			ret = -4;
			break;
		}
	}
  /*
	printf("-----------------------------------------------------------");
	for (int i = 0; i < I2C_DATA_LENGTH; i++)
	{
		if(i%16==0)
			printf("\n%x0:\t",(uint8_t)(i/15));
    printf("%02X ", cblock[i]);
	}
	printf("\n----------------------------------------------------------\n");
  */
	uint32_t _startCRC = *startCRC;
	*startCRC = _crc32(_startCRC, cblock, length);

	close(fd);
	return ret;
}

/*!
 * @brief i2c_isp_flash_calculate_crc()
 * @param *filename - compare against this file
 *
 * @return crc on success or negative on failure.
 */
int i2c_isp_flash_calculate_crc(const char * filename)
{
	uint32_t crc_input_file = calculate_bin_crc(filename);
	LOG_INFO("File(%s)-CRC32: %x", filename, crc_input_file);

	// file check
	int fp = open(filename, O_RDONLY);
	if (fp < 0) {
		LOG_ERROR("Failed to open %s bin file",filename);
		return -1;
	}

	i2c_spi_flash_cmd_set_page_access(true);

	struct stat st;
	fstat(fp, &st);
	const int BLOCKS = (st.st_size / I2C_DATA_LENGTH);
	const int remainder = st.st_size - BLOCKS * I2C_DATA_LENGTH;
	LOG_INFO("Going to read %d blocks of 256byte + remainder of %d bytes", BLOCKS, remainder);

	gp_flash_msg cmd;
	cmd.cmd = SPI_IS25_VERBOSE;
	cmd.param[0] = 0; // disable verbosity on gp side
	i2c_spi_flash_cmd(cmd);

	uint32_t crc = 0;
	uint32_t *ptr = &crc;
	uint16_t i = 0;
	int ret = 0;
	for (i = 0; i < BLOCKS; i++)
	{
		ret = i2c_spi_flash_readback_page(i, ptr, I2C_DATA_LENGTH);
		if (ret < 0)
			break;
		if ((i%100==0) || (i==BLOCKS))
			LOG_DEBUG("...reading %d/%d blocks", i, BLOCKS);
	}

	if (ret >= 0) {
		i2c_spi_flash_readback_page(i, ptr, remainder);

		LOG_DEBUG("Calculated SPI Flash crc = %x", crc);

		if( crc!=crc_input_file) {
			LOG_WARN("CRC's do not match!");
		}
		else LOG_INFO("CRC's match!");
	}

	cmd.param[0] = 1; // re-enable verbosity on gp side
	i2c_spi_flash_cmd(cmd);

	i2c_spi_flash_cmd_set_page_access(false);

	return crc;
}