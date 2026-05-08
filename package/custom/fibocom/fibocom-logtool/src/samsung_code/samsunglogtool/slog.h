#ifndef __SLOG_H
#define __SLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif
#include <linux/usbdevice_fs.h>
#include "ftp_client.h"
#include "misc_usb.h"

#define FIBO_BUF_SIZE   512

typedef struct {
    uint8_t fb_flag_1;//0xFF
    uint8_t fb_flag_2;//0xBB
    uint8_t package_type;
    uint8_t package_flag;
    uint32_t payload_lenth;
} FB_TCP_HEAD, *P_FB_TCP_HEAD;


typedef struct arguments
{
    int usbdev;
    int ttyfd;
    int usb_sockets[2];
    fibo_usbdev_t *udev;
} arguments_t;
extern char g_qcom_logdir[];

uint16_t slog_le16 (uint16_t v16);
uint32_t slog_le32 (uint32_t v32);
uint64_t slog_le64 (uint64_t v64);

size_t slog_poll_write(int fd, const void *buf, size_t size);
#endif
