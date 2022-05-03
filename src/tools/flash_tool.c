#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>

#include "bootloader.h"
#include "comm_driver.h"
#include "crc_linux.h"
#include "comm_protocol.h"
#include "comm_i2c_gpmcu.h"
#include "socket.h"
#include "logger.h"

#define MAX_DRIVER_NAME 32
#define MAX_FILE_NAME 128
#define MAX_DEV_NAME 64
#define MAX_PARAM_LEN 128
#define MAX_CMD_LEN 128
#define ZEUS_GPMCU_PORT "/dev/ttyPS1:230400"

extern struct comm_driver_t serial_comm;
extern struct comm_driver_t udp_comm;
extern int serial_comm_set_file_path(char *path);

struct bootloader_ctxt_t _bcontext;

int _comm_send_end(struct comm_driver_t *cdriver);
int _comm_send_swap(struct comm_driver_t *cdriver);
int _comm_send_power(struct comm_driver_t *cdriver, bool state);
int _comm_send_boot(struct comm_driver_t *cdriver);
int _comm_send_reset(struct comm_driver_t *cdriver);
int _comm_send_bootinfo(struct comm_driver_t *cdriver);
int _comm_send_wdogreset(struct comm_driver_t *cdriver);
int _comm_send_wipespi(struct comm_driver_t *cdriver);

void _bootloader_print_info(struct bootloader_ctxt_t *bctxt)
{
	LOG_RAW("Bootloader info");
	LOG_RAW("Nr of Partitions: %d", BTLR_NR_APPS);
	LOG_RAW("Current partition: %d", (uint8_t) bctxt->part);
	for (int i = 0; i < BTLR_NR_APPS; i++) {
		LOG_RAW("Partition #%d (%s)", i,
			bctxt->apps[i].crc == BOOTLOADER_MAGIC_CRC ? "unused" : "used");
		LOG_RAW("\t start: 0x%x", bctxt->apps[i].start_addr);
		LOG_RAW("\t size:  0x%x", bctxt->apps[i].partition_size);
		LOG_RAW("\t crc:   0x%x", bctxt->apps[i].crc);
	}
}

enum dev_type_t {
	DEV_TYPE_IP,
	DEV_TYPE_SERIAL,
};

struct ip_params_t {
	char	ip[INET_ADDRSTRLEN + 1];
	int	port;
};

struct serial_params_t {
	char	dev[MAX_DEV_NAME + 1];
	long	baudrate;
};

struct context_t {
	enum dev_type_t dev;
	union {
		struct ip_params_t	ip;
		struct serial_params_t	serial;
	} p;
	char		file[MAX_FILE_NAME + 1];
	char		rxfile[MAX_FILE_NAME + 1];
	bool		file_needed;
	bool		dev_needed;
	bool 		read_back;
};

int _tty_find_params(struct serial_params_t *p, char *params)
{
	int len = strlen(params);
	if (len > MAX_PARAM_LEN) {
		return -1;
	}

	/* Order should be device:baud */
	char *dev = strtok(params, ":");
	if (!dev) {
		return -1;
	}
	strncpy(p->dev, dev, MAX_DEV_NAME);

	char *baudrate = strtok(NULL, ":");
	if (!baudrate) {
		return -1;
	}
	p->baudrate = atoi(baudrate);

	LOG_DEBUG("Serial Params: %s:%d", p->dev, p->baudrate);

	return 0;
}

int _ip_find_params(struct ip_params_t *p, char *params)
{
	int len = strlen(params);
	if (len > MAX_PARAM_LEN) {
		return -1;
	}

	/* Order should be ip:port */
	char *ip = strtok(params, ":");
	if (!ip) {
		return -1;
	}
	strncpy(p->ip, ip, INET_ADDRSTRLEN);

	char *port = strtok(NULL, ":");
	if (!port) {
		return -1;
	}
	p->port = atoi(port);

	LOG_DEBUG("IP Params: %s:%d", p->ip, p->port);

	return 0;
}

int _parse_params(struct context_t *ctxt, char *drv, char *paramstr)
{
  if (ctxt->dev_needed) {
		LOG_DEBUG("%s - %s", drv, paramstr);
		if (!strncmp(drv, "ip", MAX_DRIVER_NAME)) {
			ctxt->dev = DEV_TYPE_IP;
			_ip_find_params(&ctxt->p.ip, paramstr);
		} else if (!strncmp(drv, "serial", MAX_DRIVER_NAME)) {
			ctxt->dev = DEV_TYPE_SERIAL;
			_tty_find_params(&ctxt->p.serial, paramstr);
		} else {
			return -1;
		}
	}
	return 0;
}
int _is_valid_ip(char *ipAddress)
{
	struct sockaddr_in sa;
	return inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
}

int _is_valid_path(char *path)
{
	return !access(path, F_OK);
}

int _validate_ctxt(struct context_t *ctxt)
{
	switch (ctxt->dev) {
	case DEV_TYPE_IP:
	case DEV_TYPE_SERIAL:
		break;
	default:
		return -1;
	}

	if (ctxt->dev_needed) {
		if (DEV_TYPE_IP == ctxt->dev) {
			LOG_INFO("Validating IP settings");
			if (!_is_valid_ip(ctxt->p.ip.ip)) {
				LOG_ERROR("Invalid IP");
				return -1;
			}
			if (ctxt->p.ip.port == 0) {
				LOG_ERROR("No port provided");
				return -1;
			}
		} else if (DEV_TYPE_SERIAL == ctxt->dev) {
			LOG_INFO("Validating Serial settings");
			if (!_is_valid_path(ctxt->p.serial.dev)) {
				return -1;
			}
			if (ctxt->p.serial.baudrate == 0) {
				LOG_ERROR("No baudrate provided");
				return -1;
			}
		}
	}

	if (ctxt->file_needed) {
		if (strlen(ctxt->file) <= 0) {
			LOG_ERROR("File is required");
			return -1;
		}
	}
	return 0;
}

void _send_debug_handler(struct comm_driver_t *cdriver)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = 0x00;
	buffer[1] = COMMP_DEBUG;
	comm_protocol_write_data(cdriver, buffer, 4);
}

void _send_cmd(struct comm_driver_t *cdriver, char *cmd)
{
	uint8_t in_buffer[COMMP_PACKET_SIZE];
	if (!cdriver || !cmd) {
		return;
	}

	/* TODO: This can be done waaaay more efficient.
	 * Lookup-table with switch case
	   param: info , boot, swap, powon, powoff */
	if (!strcmp(cmd, "info")) {
		LOG_INFO("Requesting bootinfo");
		_comm_send_bootinfo(cdriver);
		size_t size = COMMP_PACKET_SIZE;
		int r = comm_protocol_read_data(cdriver, in_buffer, &size);
		if (r < 0) {
			LOG_ERROR("Failed to read bootinfo");
			return;
		}
		memcpy(&_bcontext, &in_buffer[COMMP_CMD_DATA_OFFSET], sizeof(struct
									     bootloader_ctxt_t));
		_bootloader_print_info(&_bcontext);
	} else if (!strcmp(cmd, "boot")) {
		LOG_INFO("Forcing boot");
		_comm_send_boot(cdriver);
	} else if (!strcmp(cmd, "reset")) {
		LOG_INFO("Forcing reset");
		_comm_send_reset(cdriver);
	} else if (!strcmp(cmd, "swap")) {
		LOG_INFO("Forcing swap");
		_comm_send_swap(cdriver);
	} else if (!strcmp(cmd, "powon")) {
		LOG_INFO("Forcing power on");
		_comm_send_power(cdriver, true);
	} else if (!strcmp(cmd, "powoff")) {
		LOG_INFO("Forcing power off");
		_comm_send_power(cdriver, false);
	} else if (!strcmp(cmd, "wdog")) {
		LOG_INFO("Watchdog reset");
		_comm_send_wdogreset(cdriver);
	} else if (!strcmp(cmd, "wipespi")) {
		LOG_INFO("SPI Flash erase");
		_comm_send_wipespi(cdriver);
	} else {
		LOG_ERROR("Invalid command");
		return;
	}
	return;
}

void _udp_comm_set_conn(struct comm_driver_t *cdriver, struct ip_params_t *ip)
{
	struct conn_t *conn = socket_init_connection(ROLE_CLIENT, IPPROTO_UDP,
						     ip->ip, ip->port);
	COMM_SETPRIV(cdriver, conn);
}

struct comm_driver_t *_select_cdriver(struct context_t *ctxt)
{
	switch (ctxt->dev) {
	case DEV_TYPE_IP:
		LOG_INFO("Returning UDP driver");
		_udp_comm_set_conn(&udp_comm, &ctxt->p.ip);
		udp_comm.enabled = true;
		return &udp_comm;
	case DEV_TYPE_SERIAL:
		LOG_INFO("Returning serial driver");
		serial_comm_set_file_path(ctxt->p.serial.dev);
		serial_comm.enabled = true;
		return &serial_comm;
	default:
		return NULL;
	}
	return NULL;
}

void _print_tools_help()
{
	LOG_RAW("Usage: flash_tool -d <driver> <params>");
	LOG_RAW("\t -d <ip or serial> : Select driver");
	LOG_RAW("\t -p <connection params> : Connection params");
	LOG_RAW("\t\t ip example: flash_tool -d \"ip\" -p \"127.0.0.1:5000\"");
	LOG_RAW("\t\t serial example: flash_tool -d \"serial\" -p \"%s\"",ZEUS_GPMCU_PORT);
	LOG_RAW("\t -f <file to send> : Send binary firmware file");
	LOG_RAW(
		"\t\t serial command example: flash_tool -d \"serial\" -p \"%s\" -f <file>",ZEUS_GPMCU_PORT);
	LOG_RAW("\t -r <file to receive> : Receive binary firmware file");
	LOG_RAW("\t -b <size> : Readback size in bytes from rom content");
	LOG_RAW(
		"\t\t spi-Flash : flash_tool -d \"serial\" -p \"%s\" -r readback.bin -b 223168",ZEUS_GPMCU_PORT);
	LOG_RAW("\t -c <info, boot, swap, powon, powoff, reset, wdog, wipespi> : Send command");
	LOG_RAW(
		"\t\t serial command example: flash_tool -d \"serial\" -p \"%s\" -c \"info\"",ZEUS_GPMCU_PORT);
	LOG_RAW("\t -z : Force debug handler");
	LOG_RAW(
		"\t\t serial command example: flash_tool -d \"serial\" -p \"%s\" -z",ZEUS_GPMCU_PORT);
	LOG_RAW("\t -t : Force crc calculation only");
	LOG_RAW("\t\t crc example: flash_tool -t -f \"flash0.bin\"");
	LOG_RAW("\t -s : i2c over gpmcu spi-Flash command");
	LOG_RAW(
		"\t\t spi-Flash : flash_tool -s 0x02 -f gowin-firmware.bin -p /dev/i2c-5");
	LOG_RAW(
		"\t\t spi-Flash Status Register: flash_tool -s 0x05 -p /dev/i2c-5");
	LOG_RAW(
		"\t\t spi-Flash commands can be in hex \"0x\". or decimal\".\" See SPI Flash data-sheet");
	LOG_RAW(
		"\t\t spi-Flash default port \"%s\" , change with -p parameter", I2C_DEVICE);
	LOG_RAW("\t -h : Print this");
	LOG_RAW("\t\t serial command example: flash_tool -h");
	exit(0);
}

int main(int argc, char **argv)
{
	int err = logger_init();
	int c;
	char driver[MAX_DRIVER_NAME + 1] = { 0 };
	char params[MAX_PARAM_LEN + 1] = { 0 };
	char cmd[MAX_CMD_LEN + 1] = { 0 };
	bool commp = true;
	bool command = false;
	bool dhandler = false;
	bool reset_bootloader = false;
	gp_flash_msg spi_cmd = { SPI_NO_CMD, {0,0,0}};
	int rom_readsize = 0;

	struct context_t ctxt;
	memset(&ctxt, 0, sizeof(ctxt));
	ctxt.file_needed = true;
	ctxt.dev_needed = true;

	serial_comm.enabled = false;
	udp_comm.enabled = false;

	while ((c = getopt(argc, argv, "s:c:d:p:f:r:b:z:h:t")) != -1) {
		switch (c) {
		case 'd':         // driver
			strncpy(driver, optarg, MAX_DRIVER_NAME);
			break;
		case 'p':         // params
			strncpy(params, optarg, MAX_PARAM_LEN);
			break;
		case 'f':         // filename
			strncpy(ctxt.file, optarg, MAX_FILE_NAME);
			break;
		case 'r':
			ctxt.read_back = true;
			strncpy(ctxt.file, optarg, MAX_FILE_NAME);
			break;
		case 'b':
			rom_readsize = atoi(optarg);
			break;
		case 't': // only crc calculation
			ctxt.dev_needed = false;
			break;
		case 'z':
			LOG_INFO("Calling debug handler...");
			commp = false;
			dhandler = true;
			ctxt.file_needed = false;
			break;
		case 'c':         // command
			LOG_INFO("Run command");
			commp = false;
			command = true;
			strncpy(cmd, optarg, MAX_CMD_LEN);
			ctxt.file_needed = false;
			break;
    case 's':         // Gowin SPI Flash actions
			if (optarg[0]=='c' && optarg[1]=='r' && optarg[2]=='c') {
					// create local CRC cmd identifier
					spi_cmd.cmd = ~SPI_IS25_READ;
			}
			else if (optarg[1] != 'x') {
					spi_cmd.cmd = atoi(optarg);
			}
			else {
				spi_cmd.cmd = strtol(optarg, NULL, 16);
			}
			LOG_INFO("Run SPI flash command 0X%02x", spi_cmd.cmd);
			break;
		case 'h':
			_print_tools_help();
		default:
			LOG_ERROR("Invalid parameter");
			_print_tools_help();
		}
	}

	// i2c communication with gpmcu
	if (spi_cmd.cmd != SPI_NO_CMD) {
		int ret;
		if (strlen(params)>5){ // <- /dev/i2c-x
			if (i2c_isp_flash_set_device(params) < 0)
				return 0;
		}
		switch (spi_cmd.cmd)
		{
		case SPI_IS25_BER64:
			ret = i2c_spi_flash_erase64k(atoi(params));
			break;
		case (~SPI_IS25_READ):
			i2c_isp_flash_calculate_crc(ctxt.file);
			exit(0);
			break;
		case SPI_IS25_PP: // program the flash
			ret = i2c_spi_flash_program(ctxt.file);
			break;
		default: // regular commands
			ret = i2c_spi_flash_cmd(spi_cmd);
		}
		exit(ret);
	}

	err = _parse_params(&ctxt, driver, params);
	if (err < 0) {
		LOG_ERROR("Failed to parse params");
	}

	err = _validate_ctxt(&ctxt);
	if (err < 0) {
		LOG_ERROR("Invalid connection context");
		_print_tools_help();
	}


  // CRC calcul only
	if (!ctxt.dev_needed) {
		int fp = open(ctxt.file, O_RDONLY);
		if (fp < 0) {
			LOG_ERROR("Failed to open file");
			return 0;
		}

		struct stat st;
		fstat(fp, &st);

		char *buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fp, 0);
		if (!buf) {
			LOG_ERROR("Failed to mmap file");
			return 0;
		}

		uint32_t crc = calculate_bin_crc(ctxt.file);
		LOG_INFO("CRC: %x", crc);
		return 0;
	}

	struct comm_driver_t *cdriver = _select_cdriver(&ctxt);
	comm_protocol_init();

	if (reset_bootloader) {
		_send_cmd(cdriver, "reset");
	}

	/* read back to file */
	if (ctxt.read_back) {
		int packet_read_size = COMMP_DATA_SIZE;
		int packets = rom_readsize / COMMP_DATA_SIZE;
		int blocknr = 0; // for offset reading <> 0

		int fp = open(ctxt.file, O_RDWR | O_CREAT, (mode_t)0600);
		if (fp < 0) {
			LOG_ERROR("Failed to open file %s", ctxt.file);
			return 0;
		}

		struct stat statbuf;

		if (rom_readsize%COMMP_DATA_SIZE == 0)
			packets--;
		LOG_DEBUG("Going to read a total of %d packets", packets + 1);

		for (int j = 0; j <= packets; j++)
		{
			/* go to the location corresponding to the last byte = append */
			fstat(fp, &statbuf);

			if (statbuf.st_size > 0)
			{
				if (lseek(fp, statbuf.st_size, SEEK_SET) == -1)
				{
					LOG_ERROR("lseek error for file %s", ctxt.file);
					return 0;
				}
			}

			if (j == packets) {
				packet_read_size = rom_readsize % COMMP_DATA_SIZE;
				if (packet_read_size==0) // in case the size matches COMMP_DATA_SIZE
					packet_read_size = COMMP_DATA_SIZE;
			}
			else
				packet_read_size = COMMP_DATA_SIZE;

			if( ftruncate(fp,statbuf.st_size+packet_read_size) == -1 ){
				LOG_ERROR("ftruncate failed");
				return 0;
			}

			// mmap info Check - https://gist.github.com/marcetcheverry/991042
			char *map = mmap(0, statbuf.st_size+packet_read_size, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
			if (! map) {
				LOG_ERROR("Failed to mmap file %s",ctxt.file);
				return 0;
			}

			err = comm_protocol_read_binary(cdriver, (uint8_t *)map+statbuf.st_size, j, packet_read_size, COMMP_ROMID_SPIFLASH0, rom_readsize);
			if ((j%50==0) || (j>(packets-5)))
				LOG_DEBUG("comm_protocol_read_binary returned %d for packet %d, sync %d", err, j, packet_read_size);
			msync(map, statbuf.st_size + err, MS_SYNC);
			munmap(map, statbuf.st_size + err);
		}
		fstat(fp, &statbuf);
		LOG_DEBUG("Final file size is %d ",statbuf.st_size);

		close(fp);

		comm_protocol_close();
		return err;
	}

	/* Retrieve current bootloader context */
	if (cmd[0]!='i' && cmd[1]!='n'){
		_send_cmd(cdriver, "info");
	}

	/* binary transfer */
	if (commp) {
		int fp = open(ctxt.file, O_RDONLY);
		if (fp < 0) {
			LOG_ERROR("Failed to open file");
			return 0;
		}

		struct stat st;
		fstat(fp, &st);

		char *buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fp, 0);
		if (!buf) {
			LOG_ERROR("Failed to mmap file");
			return 0;
		}

		uint32_t crc = calculate_bin_crc(ctxt.file);
		LOG_INFO("CRC: %x (file size: %d)", crc, st.st_size);
		LOG_INFO("Running flash tool");
		err = comm_protocol_transfer_binary(cdriver, (uint8_t *)buf,
						st.st_size, COMMP_ROMID_ROM0, crc); //to do: change rom-nr or crc is always written to part0
	} else {
		if (dhandler) {
			LOG_DEBUG("_send_debug_handler");
			_send_debug_handler(cdriver);
		}
		if (command) {
			LOG_DEBUG("_send_cmd %d", cmd);
			_send_cmd(cdriver, cmd);
		}
	}

	comm_protocol_close();

	return err;
}
