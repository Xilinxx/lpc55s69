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

#include "version.h" // git version
#include "bootloader.h"
#include "bootloader_spi.h"
#include "comm_driver.h"
#include "crc_linux.h"
#include "comm_protocol.h"
#include "comm_i2c_gpmcu.h"
#include "socket.h"
#include "logger.h"

#define MAX_DRIVER_NAME    32
#define MAX_FILE_NAME      128
#define MAX_DEV_NAME       64
#define MAX_PARAM_LEN      128
#define MAX_CMD_LEN        128
#define ZEUS_GPMCU_PORT    "/dev/ttyPS1:230400"
#define DEFAULT_DRIVER     "serial"
#define I2C_DEVICE         "/dev/i2c-5"
#define I2C_SETGP2BOOT     "i2cset -y 5 0x60 0x30 0x01"
#define I2C_APP_REBOOT     "i2cset -y 5 0x60 0x31 0x01"
#define GOWIN_BITFILE_SIZE 221368

extern struct comm_driver_t serial_comm;
extern struct comm_driver_t udp_comm;
extern int serial_comm_set_file_path(char * path);

struct bootloader_ctxt_t _bcontext;
struct spi_ctxt_t _spicontext;

/* interfaces/commp_cmd_helpers.c implementation */
int _comm_send_generic_cmd(struct comm_driver_t * cdriver,
                           comm_proto_cmd_t cmd);
int _comm_send_application_cmd(struct comm_driver_t * cdriver,
                               comm_proto_cmd_t cmd,
                               uint8_t offset);

bool _in_boot_code();

void _bootloader_print_versioninfo(const char * buffer) {
    char * copy = strdup(buffer);
    char * version = strtok(copy, "*");

    LOG_RAW("Bootloader version\t[%s]", version);
    version = strtok(NULL, COMMP_CMD_VERSION_SEPERATOR); // Version seperator is '*'
    LOG_RAW("Bootloader git ver\t[%s]", version);
}

void _bootloader_print_info(struct bootloader_ctxt_t * bctxt) {
    LOG_RAW("Bootloader info (struct size is %d bytes)", sizeof(struct bootloader_ctxt_t));
    LOG_RAW("Nr of Partitions: %d", BTLR_NR_APPS);
    LOG_RAW("Current partition: 0x%x", (uint8_t)bctxt->part);
    for (int i = 0; i < BTLR_NR_APPS; i++) {
        LOG_RAW("Partition #%d (%s)", i,
                bctxt->apps[i].crc == BOOTLOADER_MAGIC_CRC ? "unused" : "used");
        LOG_RAW("\t start: 0x%x", bctxt->apps[i].start_addr);
        LOG_RAW("\t size:  0x%x", bctxt->apps[i].partition_size);
        LOG_RAW("\t crc:   0x%x", bctxt->apps[i].crc);
    }
}

void _bootloader_spi_print_info(struct spi_ctxt_t * bctxt) {
    LOG_RAW("SPI info (struct size is %d bytes)", sizeof(struct spi_ctxt_t));
    LOG_RAW("Nr of Partitions: %d", BTLR_NR_APPS);
    LOG_RAW("Current partition: 0x%x (%d bytes)", bctxt->part & 0x000000ff,
            sizeof(bctxt->part));
    LOG_RAW("Own CRC32: 0x%x", bctxt->crc);
    for (int i = 0; i < BTLR_SPI_NR_BIN; i++) {
        LOG_RAW("Partition #%d (%s)", i,
                bctxt->gowin[i].crc == SPI_MAGIC_CRC ? "unused" : "used");
        LOG_RAW("\t start    @ 0x%x", bctxt->gowin[i].start_addr);
        LOG_RAW("\t app size:  0x%x", bctxt->gowin[i].image_size);
        LOG_RAW("\t part size: 0x%x", bctxt->gowin[i].partition_size);
        LOG_RAW("\t crc:       0x%x", bctxt->gowin[i].crc);
        LOG_RAW("\t file size: 0x%x - %d", bctxt->gowin[i].bitfile_size,
                bctxt->gowin[i].bitfile_size);
        LOG_RAW("\t file crc:  0x%x", bctxt->gowin[i].bitfile_crc);
    }
}

/* helper function for _bootloader_print_app_info
 * return: crc
 */
uint32_t _bootloader_verify_uart_reply(int n,
                                       int required,
                                       uint8_t * data) {
    // we expect 9 return bytes
    if (n < required) {
        LOG_ERROR("Did not get enough bytes for crc calculation");
        exit(0);
    }
    // crc starts at 3th byte and is 4 byte long
    return  ((uint32_t)((data[n - 6] << 24) | (data[n - 5] << 16) | (data[n - 4] << 8) | data[n -
                                                                                              3]));
}

/* Getting the crc from both rom partitions from the application code
 * Required for upgrade script which compares crc and we can't reboot to bootloader or USB data gets lost
 */
void _bootloader_print_app_info(struct comm_driver_t * drv,
                                uint8_t * data) {
    if (_in_boot_code()) {
        LOG_WARN("Not possible in boot_code");
        exit(0);
    }

    const int EXPECT_REPLY_SIZE = 9;  // 4byte data + 5 overhead
    size_t size = COMMP_PACKET_SIZE;

    _comm_send_application_cmd(drv, COMMP_CMD_INFO_APP, 0x3);  // CRC0
    int r = comm_protocol_read_data(drv, data, &size);
    uint32_t crc = _bootloader_verify_uart_reply(r, EXPECT_REPLY_SIZE, data);
    LOG_RAW("approm0 crc = %.8x", crc);

    _comm_send_application_cmd(drv, COMMP_CMD_INFO_APP, 0x7); // CRC1
    r = comm_protocol_read_data(drv, data, &size);
    crc = _bootloader_verify_uart_reply(r, EXPECT_REPLY_SIZE, data);
    LOG_RAW("approm1 crc = %.8x", crc);

    // 0x20 + offset in {start_addr = 0x0, image_size = 0x3f000, partition_size = 0x40000, crc = 0xa5c3c132, bitfile_size = 0x360b8, bitfile_crc = 0x68227395}
    _comm_send_application_cmd(drv, COMMP_CMD_INFO_APP, 0x25); // CRC Gowin Part0
    r = comm_protocol_read_data(drv, data, &size);
    crc = _bootloader_verify_uart_reply(r, EXPECT_REPLY_SIZE, data);
    LOG_RAW("gowin0 bitfile crc = %.8x", crc);

    _comm_send_application_cmd(drv, COMMP_CMD_INFO_APP, 0x2b); // CRC Golden Gowin
    r = comm_protocol_read_data(drv, data, &size);
    crc = _bootloader_verify_uart_reply(r, EXPECT_REPLY_SIZE, data);
    LOG_RAW("gowin1 bitfile crc = %.8x", crc);

    exit(0);
}

enum dev_type_t {
    DEV_TYPE_IP,
    DEV_TYPE_SERIAL,
};

struct ip_params_t {
    char ip[INET_ADDRSTRLEN + 1];
    int port;
};

struct serial_params_t {
    char dev[MAX_DEV_NAME + 1];
    long baudrate;
};

struct context_t {
    enum dev_type_t dev;
    union {
        struct ip_params_t ip;
        struct serial_params_t serial;
    } p;
    char file[MAX_FILE_NAME + 1];
    char rxfile[MAX_FILE_NAME + 1];
    bool file_needed;
    bool dev_needed;
    bool read_back;
};

int _tty_find_params(struct serial_params_t * p,
                     char * params) {
    int len = strlen(params);

    if (len > MAX_PARAM_LEN) {
        return -1;
    }

    /* Order should be device:baud */
    char * dev = strtok(params, ":");
    if (!dev) {
        return -1;
    }
    strncpy(p->dev, dev, MAX_DEV_NAME);

    char * baudrate = strtok(NULL, ":");
    if (!baudrate) {
        return -1;
    }
    p->baudrate = atoi(baudrate);

    LOG_DEBUG("Serial Params: %s:%d", p->dev, p->baudrate);

    return 0;
}

int _ip_find_params(struct ip_params_t * p,
                    char * params) {
    int len = strlen(params);

    if (len > MAX_PARAM_LEN) {
        return -1;
    }

    /* Order should be ip:port */
    char * ip = strtok(params, ":");
    if (!ip) {
        return -1;
    }
    strncpy(p->ip, ip, INET_ADDRSTRLEN);

    char * port = strtok(NULL, ":");
    if (!port) {
        return -1;
    }
    p->port = atoi(port);

    LOG_DEBUG("IP Params: %s:%d", p->ip, p->port);

    return 0;
}

int _parse_params(struct context_t * ctxt,
                  char * drv,
                  char * paramstr) {
    if (ctxt->dev_needed) {
        // Add default parameters
        if (drv[0] == '\0')
            strcpy(drv, DEFAULT_DRIVER);
        if (paramstr[0] == '\0')
            strcpy(paramstr, ZEUS_GPMCU_PORT);
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
int _is_valid_ip(char * ipAddress) {
    struct sockaddr_in sa;

    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
}

int _is_valid_path(char * path) {
    return !access(path, F_OK);
}

bool _in_boot_code() {
    FILE * fp;
    char path[256];

    /* Open the command for reading. */
    fp = popen("i2cdetect -y -r 5 | grep 60", "r");
    if (fp == NULL) {
        LOG_RAW("Failed to run command");
        exit(1);
    }

    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path), fp) != NULL) {
        // LOG_RAW("%s", path);
    }
    pclose(fp);

    if (path[4] == '-')   // 60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
        return true;
    else
        return false;
}

int _validate_ctxt(struct context_t * ctxt) {
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
            LOG_ERROR("File is required, got [%s]", ctxt->file);
            return -1;
        }
    }
    return 0;
}

void _send_debug_handler(struct comm_driver_t * cdriver) {
    uint8_t buffer[4] = { 0 };

    buffer[0] = 0x00;
    buffer[1] = COMMP_DEBUG;
    comm_protocol_write_data(cdriver, buffer, 4);
}

void _send_cmd(struct comm_driver_t * cdriver,
               char * cmd) {
    uint8_t in_buffer[COMMP_PACKET_SIZE];

    if (!cdriver || !cmd) {
        return;
    }

    if (!strcmp(cmd, "info")) {
        LOG_INFO("Requesting bootinfo");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_BOOTINFO);
        size_t size = COMMP_PACKET_SIZE;
        int r = comm_protocol_read_data(cdriver, in_buffer, &size);
        if (r < 0) {
            LOG_ERROR("Failed to read bootinfo");
            return;
        }
        memcpy(&_bcontext, &in_buffer[COMMP_CMD_DATA_OFFSET], sizeof(struct bootloader_ctxt_t));
        _bootloader_print_info(&_bcontext);
    } else if (!strcmp(cmd, "version")) {
        LOG_INFO("Requesting version info");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_VERINFO);
        size_t size = COMMP_PACKET_SIZE;
        int r = comm_protocol_read_data(cdriver, in_buffer, &size);
        if (r < 0) {
            LOG_ERROR("Failed to read bootinfo");
            return;
        }
        _bootloader_print_versioninfo(&in_buffer[COMMP_CMD_DATA_OFFSET]);
    } else if (!strcmp(cmd, "boot")) {
        LOG_INFO("Forcing boot");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_BOOT);
    } else if (!strcmp(cmd, "reset")) {
        LOG_INFO("Forcing reset");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_RESET);
    } else if (!strcmp(cmd, "swap")) {
        LOG_INFO("Forcing swap");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_SWAP);
    } else if (!strcmp(cmd, "powon")) {
        LOG_INFO("Forcing power on");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_PWR_ON);
    } else if (!strcmp(cmd, "powoff")) {
        LOG_INFO("Forcing power off");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_PWR_OFF);
    } else if (!strcmp(cmd, "reconfigure")) {
        LOG_INFO("Reconfigure Gowin FPGA");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_RECONFIG_GOWIN);
    } else if (!strcmp(cmd, "wdog")) {
        LOG_INFO("Watchdog reset");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_TRIGGER_WDOG);
    } else if (!strcmp(cmd, "gp2boot")) {
        LOG_INFO("Send application to bootcode over i2c");
        // set gp to boot partition
        system(I2C_SETGP2BOOT);
        // reset request - via wdog
        system(I2C_APP_REBOOT);
    } else if (!strcmp(cmd, "wipespi0")) {
        LOG_INFO("SPI Flash erase");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_ERASE_SPI0);
    } else if (!strcmp(cmd, "wipespi1")) {
        LOG_INFO("SPI Flash erase");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_ERASE_SPI1);
    } else if (!strcmp(cmd, "setspi0")) {
        LOG_INFO("Set SPI0 partition");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_SET_SPI0);
    } else if (!strcmp(cmd, "setspi1")) {
        LOG_INFO("Set SPI1 partition");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_SET_SPI1);
    } else if (!strcmp(cmd, "setrom0")) {
        LOG_INFO("Set APPROM0 partition");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_SET_ROM0);
    } else if (!strcmp(cmd, "setrom1")) {
        LOG_INFO("Set APPROM1 partition");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_SET_ROM1);
    } else if (!strcmp(cmd, "infospi")) {
        LOG_INFO("Request SPI info");
        _comm_send_generic_cmd(cdriver, COMMP_CMD_INFO_SPI);
        size_t size = COMMP_PACKET_SIZE;
        int r = comm_protocol_read_data(cdriver, in_buffer, &size);
        if (r < 0) {
            LOG_ERROR("Failed to read spiinfo");
            return;
        }
        memcpy(&_spicontext, &in_buffer[COMMP_CMD_DATA_OFFSET], sizeof(struct spi_ctxt_t));
        _bootloader_spi_print_info(&_spicontext);
    } else if (!strcmp(cmd, "infoapp")) {
        _bootloader_print_app_info(cdriver, in_buffer);
    } else {
        LOG_ERROR("Invalid command");
        return;
    }
    return;
}

void _udp_comm_set_conn(struct comm_driver_t * cdriver,
                        struct ip_params_t * ip) {
    struct conn_t * conn = socket_init_connection(ROLE_CLIENT, IPPROTO_UDP,
                                                  ip->ip, ip->port);

    COMM_SETPRIV(cdriver, conn);
}

struct comm_driver_t * _select_cdriver(struct context_t * ctxt) {
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

int _read_back_rom(struct context_t ctxt,
                   struct comm_driver_t * cdriver,
                   int rom_readsize,
                   int gowin_partition) {
    int err = 0;
    int packet_read_size = COMMP_DATA_SIZE;
    int packets = rom_readsize / COMMP_DATA_SIZE;
    int blocknr = 0;     // for offset reading <> 0

    int fp = open(ctxt.file, O_RDWR | O_CREAT, (mode_t)0600);

    if (fp < 0) {
        LOG_ERROR("Failed to open file %s", ctxt.file);
        return 0;
    }

    struct stat statbuf;

    if (rom_readsize % COMMP_DATA_SIZE == 0)
        packets--;
    LOG_DEBUG("Going to read a total of %d packets", packets + 1);

    for (int j = 0; j <= packets; j++) {
        /* go to the location corresponding to the last byte = append */
        fstat(fp, &statbuf);

        if (statbuf.st_size > 0) {
            if (lseek(fp, statbuf.st_size, SEEK_SET) == -1) {
                LOG_ERROR("lseek error for file %s", ctxt.file);
                return 0;
            }
        }

        if (j == packets) {
            packet_read_size = rom_readsize % COMMP_DATA_SIZE;
            if (packet_read_size == 0) {                    // in case the size matches COMMP_DATA_SIZE
                packet_read_size = COMMP_DATA_SIZE;
            }
        } else {
            packet_read_size = COMMP_DATA_SIZE;
        }

        if ( ftruncate(fp, statbuf.st_size + packet_read_size) == -1 ) {
            LOG_ERROR("ftruncate failed");
            return 0;
        }

        // mmap info Check - https://gist.github.com/marcetcheverry/991042
        char * map = mmap(0, statbuf.st_size + packet_read_size, PROT_READ |
                          PROT_WRITE, MAP_SHARED, fp, 0);
        if (!map) {
            LOG_ERROR("Failed to mmap file %s", ctxt.file);
            return 0;
        }

        uint8_t _read_part = COMMP_ROMID_SPIFLASH0;
        if (gowin_partition == 1)
            _read_part = COMMP_ROMID_SPIFLASH1;

        err = comm_protocol_read_binary(cdriver, (uint8_t *)map + statbuf.st_size, j,
                                        packet_read_size, _read_part, rom_readsize);
        if ((j % 50 == 0) || (j > (packets - 5))) {
            LOG_DEBUG("comm_protocol_read_binary returned %d for packet %d, sync %d",
                      err, j, packet_read_size);
        }
        msync(map, statbuf.st_size + err, MS_SYNC);
        munmap(map, statbuf.st_size + err);
    }
    fstat(fp, &statbuf);
    LOG_DEBUG("Final file size is %d ", statbuf.st_size);

    close(fp);

    comm_protocol_close();
    return err;
}

void _print_tools_help() {
    LOG_RAW("Usage: flash_tool -d <driver> <params>");
    LOG_RAW("       Commands only functional when GPmcu runs in bootloader-code!");
    LOG_RAW("\t -v : version info");
    LOG_RAW("\t -d <ip or serial> : Select driver (default: %s)", DEFAULT_DRIVER);
    LOG_RAW("\t -p <connection params> : Connection params (default: %s)", ZEUS_GPMCU_PORT);
    LOG_RAW("\t\t ip example: flash_tool -d \"ip\" -p \"127.0.0.1:5000\"");
    LOG_RAW("\t\t serial example: flash_tool -d \"serial\" -p \"%s\"", ZEUS_GPMCU_PORT);
    LOG_RAW("\t -f <file to send> : Send binary firmware file");
    LOG_RAW(
        "\t\t serial command example: flash_tool -d \"serial\" -p \"%s\" -f <file>",
        ZEUS_GPMCU_PORT);
    LOG_RAW("\t -g <0,1>: Indicates binary firmware file is for Gowin(SPI flash) partition nr");
    LOG_RAW("\t -r <file to receive> : Receive binary firmware file");
    LOG_RAW("\t -b <size> : Readback size in bytes from rom content");
    LOG_RAW(
        "\t\t spi-Flash : flash_tool -d \"serial\" -p \"%s\" -r gw-readb.bin -b %d",
        ZEUS_GPMCU_PORT, GOWIN_BITFILE_SIZE);
    LOG_RAW(
        "\t -c <info, version, boot, swap, powon, powoff, reset, wdog, setrom<0,1>> : Send command");
    LOG_RAW("\t -c <setspi<0,1>, wipespi<0,1>, infospi, > : Send spi command");
    LOG_RAW("\t -c reconfigure : gowin reconfig command");
    LOG_RAW("\t -c gp2boot : Application (i2c)-command to get in bootloader");
    LOG_RAW("\t -c infoapp : Application (uart)command to get partition crc");
    LOG_RAW(
        "\t\t serial command example: flash_tool -d \"serial\" -p \"%s\" -c \"info\"",
        ZEUS_GPMCU_PORT);
    LOG_RAW("\t -z : Force debug handler");
    LOG_RAW(
        "\t\t serial command example: flash_tool -d \"serial\" -p \"%s\" -z", ZEUS_GPMCU_PORT);
    LOG_RAW("\t -t : Force crc32 calculation of a file");
    LOG_RAW("\t\t crc example: flash_tool -t -f \"flash.bin\"");
    LOG_RAW("\t -s : i2c over gpmcu spi-Flash command (Application-code only!)");
    // Still functional but obselete since spi is flashed via bootloader now
    // LOG_RAW(
    //     "\t\t spi-Flash : flash_tool -s 0x02 -f gw-greenpower.bin -p %s",I2C_DEVICE);
    LOG_RAW(
        "\t\t spi-Flash Status Register: flash_tool -s 0x05 -p %s", I2C_DEVICE);
    LOG_RAW(
        "\t\t spi-Flash commands can be in hex \"0x\". or decimal\".\" See SPI Flash data-sheet");
    LOG_RAW(
        "\t\t spi-Flash default port \"%s\" , change with -p parameter", I2C_DEVICE);
    LOG_RAW("\t -h : Print this");
    LOG_RAW("\t\t serial command example: flash_tool -h");
    exit(0);
}

int main(int argc,
         char ** argv) {
    int err = logger_init();
    int c;
    char driver[MAX_DRIVER_NAME + 1] = { 0 };
    char params[MAX_PARAM_LEN + 1] = { 0 };
    char cmd[MAX_CMD_LEN + 1] = { 0 };
    bool commp = true;
    bool command = false;
    bool dhandler = false;
    bool reset_bootloader = false;
    int gowin_partition = -1;
    gp_flash_msg spi_cmd = { SPI_NO_CMD, { 0, 0, 0 } };
    int rom_readsize = 0;

    struct context_t ctxt;

    memset(&ctxt, 0, sizeof(ctxt));
    ctxt.file_needed = true;
    ctxt.dev_needed = true;

    serial_comm.enabled = false;
    udp_comm.enabled = false;

    while ((c = getopt(argc, argv, "s:c:d:p:f:r:b:z:h:vtg:")) != -1) {
        switch (c) {
            case 'd':             // driver
                strncpy(driver, optarg, MAX_DRIVER_NAME);
                break;
            case 'p':             // params
                strncpy(params, optarg, MAX_PARAM_LEN);
                break;
            case 'f':             // filename
                strncpy(ctxt.file, optarg, MAX_FILE_NAME);
                break;
            case 'r':
                ctxt.read_back = true;
                strncpy(ctxt.file, optarg, MAX_FILE_NAME);
                break;
            case 'b':
                rom_readsize = atoi(optarg);
                break;
            case 't':     // only crc calculation of input file
                ctxt.dev_needed = false;
                break;
            case 'z':
                LOG_INFO("Calling debug handler...");
                commp = false;
                dhandler = true;
                ctxt.file_needed = false;
                break;
            case 'c': // command
                LOG_INFO("Run command");
                commp = false;
                command = true;
                strncpy(cmd, optarg, MAX_CMD_LEN);
                ctxt.file_needed = false;
                break;
            case 's': // Gowin SPI Flash actions
                if (optarg[0] == 'c' && optarg[1] == 'r' && optarg[2] == 'c') {
                    // create local CRC cmd identifier
                    spi_cmd.cmd = ~SPI_IS25_READ;
                } else if (optarg[1] != 'x') {
                    spi_cmd.cmd = atoi(optarg);
                } else {
                    spi_cmd.cmd = strtol(optarg, NULL, 16);
                }
                LOG_INFO("Run SPI flash command 0X%02x", spi_cmd.cmd);
                break;
            case 'g':
                gowin_partition = atoi(optarg);
                LOG_INFO("Gowin partition = %d", gowin_partition);
                break;
            case 'v':
                LOG_RAW("%s", FLASH_TOOL_VERSION_GIT);
                exit(0);
                break;
            case 'h':
                _print_tools_help();
            default:
                LOG_ERROR("Invalid parameter");
                _print_tools_help();
        }
    }

    bool in_app = false;
    if (_in_boot_code()) {
        LOG_INFO("GPmcu in boot-code");
    } else {
        LOG_WARN("GPmcu in application-code, most commands only work in boot-code");
        in_app = true;
    }

    // i2c communication with gpmcu - application code
    if (spi_cmd.cmd != SPI_NO_CMD) {
        if (in_app == false) {
            return 0;
        }
        int ret;
        // overrule the default I2C_DEVICE
        if (strlen(params) > 5) {      // <- /dev/i2c-x
            if (i2c_isp_flash_set_device(params) < 0)
                return 0;
        }
        switch (spi_cmd.cmd) {
            case SPI_IS25_BER64:
                ret = i2c_spi_flash_erase64k(atoi(params));
                break;
            case (~SPI_IS25_READ):
                i2c_isp_flash_calculate_crc(ctxt.file);
                exit(0);
                break;
            case SPI_IS25_PP:     // program the flash
                ret = i2c_spi_flash_program(ctxt.file);
                break;
            default:     // regular commands
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

        char * buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fp, 0);
        if (!buf) {
            LOG_ERROR("Failed to mmap file");
            return 0;
        }

        uint32_t crc = calculate_bin_crc(ctxt.file);
        LOG_INFO("CRC: %x", crc);
        return 0;
    }

    struct comm_driver_t * cdriver = _select_cdriver(&ctxt);
    comm_protocol_init();

    if (reset_bootloader) {
        _send_cmd(cdriver, "reset");
    }

    /* read back to file - SPI Flash */
    if (ctxt.read_back && !in_app) {
        return _read_back_rom(ctxt, cdriver, rom_readsize, gowin_partition);
    }

    /* Retrieve current bootloader context */
    if (cmd[0] != 'i' && cmd[1] != 'n' && gowin_partition < 0 && !in_app) {
        _send_cmd(cdriver, "info");
    }

    /* binary transfer */
    if (commp && !in_app) {
        int fp = open(ctxt.file, O_RDONLY);
        if (fp < 0) {
            LOG_ERROR("Failed to open file");
            return 0;
        }

        struct stat st;
        fstat(fp, &st);

        char * buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fp, 0);
        if (!buf) {
            LOG_ERROR("Failed to mmap file");
            return 0;
        }

        uint32_t crc = calculate_bin_crc(ctxt.file);
        LOG_INFO("CRC: %x (file size: %d)", crc, st.st_size);
        LOG_INFO("Running flash tool");
        if (gowin_partition >= 0) {
            if (gowin_partition == 0)
                err = comm_protocol_transfer_binary(cdriver, (uint8_t *)buf,
                                                    st.st_size, COMMP_ROMID_SPIFLASH0, crc);
            else if (gowin_partition == 1)
                err = comm_protocol_transfer_binary(cdriver, (uint8_t *)buf,
                                                    st.st_size, COMMP_ROMID_SPIFLASH1, crc);
            else
                LOG_ERROR("unknown spi gowin partition");
        } else {
            err = comm_protocol_transfer_binary(cdriver, (uint8_t *)buf,
                                                st.st_size, COMMP_ROMID_ROM0, crc); // to do: change rom-nr or crc is always written to part0
            // COMMP_ROMID_ROM0/1, COMMP_ROMID_SPIFLASH0/1
        }
    } else {
        if (dhandler) {
            LOG_DEBUG("_send_debug_handler");
            _send_debug_handler(cdriver);
        }
        if (command) {
            LOG_DEBUG("_send_cmd [%s]", cmd);
            _send_cmd(cdriver, cmd);
        }
    }

    comm_protocol_close();

    return err;
}
