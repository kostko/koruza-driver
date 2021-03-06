/*
 * koruza-driver - KORUZA driver
 *
 * Copyright (C) 2016 Jernej Kos <jernej@kos.mx>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "serial.h"
#include "configuration.h"

#include <libubox/uloop.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

struct serial_device {
  uint8_t ready;
  // Device.
  char *device;
  // Serial device uloop file descriptor wrapper.
  struct uloop_fd ufd;
  // Frame parser.
  parser_t parser;
};

static struct serial_device device_motors;
static struct serial_device device_accelerometer;

int serial_start_device(struct serial_device *cfg);
int serial_init_device(struct serial_device *cfg, int quiet);
int serial_reinit_device(struct serial_device *cfg);
struct serial_device *serial_get_device(serial_device_t device);
struct serial_device *serial_get_device_fd(int fd);
void serial_fd_handler(struct uloop_fd *ufd, unsigned int events);

int serial_init(struct uci_context *uci)
{
  int result = 0;

  // Motors MCU.
  device_motors.ready = 0;
  device_motors.device = uci_get_string(uci, "koruza.@mcu[0].device");
  if (!device_motors.device) {
    device_motors.device = "/dev/ttyS1";
  }
  result = serial_start_device(&device_motors);

  if (result != 0) {
    return result;
  }

  // Accelerometer MCU (can be disconnected).
  device_accelerometer.ready = 0;
  device_accelerometer.device = uci_get_string(uci, "koruza.@accelerometer[0].device");
  if (!device_accelerometer.device) {
    device_accelerometer.device = "/dev/ttyUSB0";
  }
  (void) serial_start_device(&device_accelerometer);

  return 0;
}

struct serial_device *serial_get_device(serial_device_t device)
{
  switch (device) {
    case DEVICE_MOTORS: return &device_motors;
    case DEVICE_ACCELEROMETER: return &device_accelerometer;
    default: return NULL;
  }
}

struct serial_device *serial_get_device_fd(int fd)
{
  if (fd == device_motors.ufd.fd) {
    return &device_motors;
  } else if (fd == device_accelerometer.ufd.fd) {
    return &device_accelerometer;
  } else {
    return NULL;
  }
}

void serial_set_message_handler(serial_device_t device, frame_message_handler handler)
{
  struct serial_device *cfg = serial_get_device(device);
  if (!cfg) {
    syslog(LOG_ERR, "Failed to set message handler for serial device %d", device);
    return;
  }

  cfg->parser.handler = handler;
}

int serial_start_device(struct serial_device *cfg)
{
  frame_parser_init(&cfg->parser);
  return serial_init_device(cfg, 0);
}

int serial_init_device(struct serial_device *cfg, int quiet)
{
  if (cfg->ready != 0 || !cfg->device) {
    return -1;
  }

  cfg->ufd.fd = open(cfg->device, O_RDWR | O_CLOEXEC);
  if (cfg->ufd.fd < 0) {
    if (!quiet) {
      syslog(LOG_ERR, "Failed to open serial device '%s'.", cfg->device);
    }
    return -1;
  }

  struct termios serial_tio;
  if (tcgetattr(cfg->ufd.fd, &serial_tio) < 0) {
    if (!quiet) {
      syslog(LOG_ERR, "Failed to get configuration for serial device '%s': %s (%d)",
        cfg->device, strerror(errno), errno);
    }
    close(cfg->ufd.fd);
    return -1;
  }

  cfmakeraw(&serial_tio);
  cfsetispeed(&serial_tio, B115200);
  cfsetospeed(&serial_tio, B115200);

  if (tcsetattr(cfg->ufd.fd, TCSAFLUSH, &serial_tio) < 0) {
    if (!quiet) {
      syslog(LOG_ERR, "Failed to configure serial device '%s': %s (%d)",
        cfg->device, strerror(errno), errno);
    }
    close(cfg->ufd.fd);
    return -1;
  }

  cfg->ready = 1;
  cfg->ufd.cb = serial_fd_handler;

  uloop_fd_add(&cfg->ufd, ULOOP_READ);

  syslog(LOG_INFO, "Initialized serial device '%s'.", cfg->device);

  return 0;
}

int serial_reinit_device(struct serial_device *cfg)
{
  cfg->ready = 0;
  uloop_fd_delete(&cfg->ufd);
  close(cfg->ufd.fd);

  return serial_init_device(cfg, 1);
}

void serial_fd_handler(struct uloop_fd *ufd, unsigned int events)
{
  struct serial_device *cfg = serial_get_device_fd(ufd->fd);
  if (!cfg || !cfg->ready) {
    return;
  }

  uint8_t buffer[1024];
  ssize_t size = read(cfg->ufd.fd, buffer, sizeof(buffer));
  if (size < 0) {
    syslog(LOG_ERR, "Failed to read from serial device.");
    serial_reinit_device(cfg);
    return;
  }

  frame_parser_push_buffer(&cfg->parser, buffer, size);
}

int serial_send_message(serial_device_t device, const message_t *message)
{
  struct serial_device *cfg = serial_get_device(device);
  if (!cfg || !cfg->ready) {
    serial_reinit_device(cfg);
    return -1;
  }

  if (cfg->ufd.fd < 0) {
    serial_reinit_device(cfg);
    return -1;
  }

  static uint8_t buffer[FRAME_MAX_LENGTH];
  ssize_t size = frame_message(buffer, sizeof(buffer), message);
  if (size < 0) {
    return -1;
  }

  size_t offset = 0;
  while (offset < size) {
    ssize_t written = write(cfg->ufd.fd, &buffer[offset], size - offset);
    if (written < 0) {
      syslog(LOG_ERR, "Failed to write frame (%ld bytes) to serial device: %s (%d)",
        (long int) size, strerror(errno), errno);
      serial_reinit_device(cfg);
      return -1;
    }

    offset += written;
  }

  return 0;
}
