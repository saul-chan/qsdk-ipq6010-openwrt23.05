#ifndef __USB2TCP_H__
#define __USB2TCP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <dirent.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "misc_usb.h"

#define FIBO_USB2TPC_MASK 0x12345678

typedef struct {
    int type;
    int length;
    int data[];
} tcp_tlv;

typedef struct {
    int type;
    int length;
    int idVendor;
    int idProduct;
    char ModuleName[64];
} tcp_tlv_modules;

int tcp_connect_module_host(const char *ip_portname, int *p_idVendor, int *p_idProduct);

fibo_usbdev_t *fibo_usb2tcp_open(void);
int fibo_usb2tcp_close(fibo_usbdev_t *pdev);

int usb2tcp_server_main(fibo_usbdev_t *pdev);

#endif
