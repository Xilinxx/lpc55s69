#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#include "comm_driver.h"
#include "logger.h"

#define SERIAL_PORT_MAX_LEN 64

struct serial_comm_ctxt_t {
	int	fd;
	char	port_name[SERIAL_PORT_MAX_LEN + 1];
};

static int _set_interface_attribs(int fd, int speed, int parity)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) != 0) {
		LOG_ERROR("error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~(IGNBRK | INLCR | ICRNL);       // disable break/NL/CR processing
	tty.c_lflag = 0;                                // no signaling chars, no echo,
	                                                // no canonical processing
	tty.c_oflag = 0;                                // no remapping, no delays
	tty.c_cc[VMIN] = 0;                             // read doesn't block
	tty.c_cc[VTIME] = 5;                            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);                // ignore modem controls,
	                                                // enable reading
	tty.c_cflag &= ~(PARENB | PARODD);              // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		LOG_ERROR("error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}

static int _comm_serial_init(void *drv)
{
	struct comm_driver_t *driver = (struct comm_driver_t *)drv;

	if (!driver) {
		LOG_ERROR("No driver present");
		return -1;
	}
	struct serial_comm_ctxt_t *ctxt =
		(struct serial_comm_ctxt_t *)driver->priv_data;

	if (!ctxt) {
		LOG_ERROR("No context found!");
		return -1;
	}

	ctxt->fd = open(ctxt->port_name, O_RDWR | O_NOCTTY | O_SYNC);
	if (ctxt->fd < 0) {
		LOG_ERROR("Failed to open port %s", ctxt->port_name);
		return -1;
	}

	int error = _set_interface_attribs(ctxt->fd, B230400, 0);

	if (error) {
		LOG_ERROR("Failed to set attributes");
		close(ctxt->fd);
		return -1;
	}
	LOG_OK("Serial driver initialized");

	return 0;
}

static int _comm_serial_write(void *drv, uint8_t *buffer, size_t len)
{
	struct comm_driver_t *driver = (struct comm_driver_t *)drv;
	if (!driver) {
		LOG_ERROR("No driver present");
		return -1;
	}

	struct serial_comm_ctxt_t *ctxt =
		(struct serial_comm_ctxt_t *)driver->priv_data;
	if (!ctxt) {
		LOG_ERROR("No context found!");
		return -1;
	}

	int written = write(ctxt->fd, buffer, len);

	if (written < 0) {
		LOG_ERROR("Failed to write to serial");
		return -1;
	}

	return written;
}

static int _comm_serial_read(void *drv, uint8_t *buffer, size_t *len)
{
	struct comm_driver_t *driver = (struct comm_driver_t *)drv;
	if (!driver) {
		LOG_ERROR("No driver present");
		return -1;
	}

	struct serial_comm_ctxt_t *ctxt =
		(struct serial_comm_ctxt_t *)driver->priv_data;
	if (!ctxt) {
		LOG_ERROR("No context found!");
		return -1;
	}

	size_t tmp = 0;
#define READ_TIMEOUT 5
	int timeout = READ_TIMEOUT;
#define MIN_READ_SIZE 1
	while (tmp < MIN_READ_SIZE) {
		int readn = read(ctxt->fd, &buffer[tmp], *len);
		if (readn < 0) {
			LOG_ERROR("Failed to read from serial");
			return -1;
		}
		if (readn == 0)
		{
			timeout--;
			if(timeout == 0) {
				LOG_ERROR("Failed to read - timeout");
				return -1;
			}
		}

		tmp += readn;
	}
	*len = tmp;

	return *len;
}

static void _comm_serial_close(void *drv)
{
	struct comm_driver_t *driver = (struct comm_driver_t *)drv;

	if (!driver) {
		LOG_ERROR("No driver present");
		return;
	}
	struct serial_comm_ctxt_t *ctxt =
		(struct serial_comm_ctxt_t *)driver->priv_data;

	if (!ctxt) {
		LOG_ERROR("No context found!");
		return;
	}
	close(ctxt->fd);
}

static struct serial_comm_ctxt_t _ctxt = {
	.fd	= -1,
};

static const struct comm_ops_t _serial_ops = {
	.init	= _comm_serial_init,
	.write	= _comm_serial_write,
	.read	= _comm_serial_read,
	.close	= _comm_serial_close,
};

struct comm_driver_t serial_comm = {
	.enabled	= true,
	.name		= "serial",
	.ops		= &_serial_ops,
	.priv_data	= (void *)&_ctxt,
};

int serial_comm_set_file_path(char *path)
{
	struct serial_comm_ctxt_t *ctxt =
		(struct serial_comm_ctxt_t *)serial_comm.priv_data;

	if (!ctxt) {
		LOG_ERROR("No context found!");
		return -1;
	}
	memset(ctxt->port_name, 0, SERIAL_PORT_MAX_LEN + 1);
	strncpy(ctxt->port_name, path, SERIAL_PORT_MAX_LEN);

	return 0;
}
