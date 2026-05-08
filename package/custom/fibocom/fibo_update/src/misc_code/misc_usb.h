#ifndef __MISC_USB_H__
#define __MISC_USB_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <dirent.h>
#include <pthread.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif
#include <linux/usbdevice_fs.h>
#include "misc.h"


#define MAX_USBFS_BULK_OUT_SIZE     (16 * 1024)
#define MAX_USBFS_BULK_IN_SIZE      (16 * 1024)
#define EXTEND    32
#define USB_DIR_BASE    "/sys/bus/usb/devices"

typedef struct
{
    /* do not add new variable here */
    char *ModuleName;
    int idVendor;
    int idProduct;
    int used_ifnum;    //diag_interface_num
    int (* main_function)(int argc, char **argv);
    int usb_need_zero_package;
    /* do not add new variable before */

    char portname[MAX_PATH_LEN];    // diag_port
    char at_port[MAX_PATH_LEN];     // at_port
    char syspath[MAX_PATH_LEN*2];
    char busname[MAX_PATH_LEN];

    int bulk_ep_in;
    int bulk_ep_out;
    int wMaxPacketSize;
    int erase_all_before_download;

    int ttyfd;
    int usbdev;

    int tcp_client_fd;
    int (* write)(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec);
    int (* read)(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec);
    int pcie_fd;
    int at_interface_num;
} fibo_usbdev_t;


fibo_usbdev_t *fibo_get_fibocom_device(fibo_usbdev_t *(* find_devices_in_table)(int idvendor, int idproduct, int port_check_mode), char *portname, char *syspath, int port_check_mode);
int fibo_usb_open(fibo_usbdev_t *pdev, int usbfs_only);
int fibo_usb_close(fibo_usbdev_t *pdev);
int fibo_get_ttyport_by_syspath(fibo_usbdev_t *pdev);
int fibo_common_send_atcmd(fibo_usbdev_t *pdev, char * at_cmd);

#endif

