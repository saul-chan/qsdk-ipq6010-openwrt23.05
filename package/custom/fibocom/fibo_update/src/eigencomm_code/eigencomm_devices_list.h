#ifndef __EIGENCOMM_DEVICES_LIST_H__
#define __EIGENCOMM_DEVICES_LIST_H__

#include "misc_usb.h"

extern int eigencomm_download_main(int argc,char *argv[]);



static fibo_usbdev_t eigencomm_devices_table[] =
{
    {"FIBOCOM LE", 0x2CB7, 0x0d01, 4, eigencomm_download_main, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,2},
    {"FIBOCOM LE", 0x2CB7, 0x0111, 4, eigencomm_download_main, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,2},
};

static fibo_usbdev_t eigencomm_dl_devices_table[] =
{
    {"FIBOCOM LE", 0x17d1, 0x0001, 0, eigencomm_download_main, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
};


#endif

