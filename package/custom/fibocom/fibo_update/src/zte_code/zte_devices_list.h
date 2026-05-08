#ifndef __ZTE_DEVICES_LIST_H__
#define __ZTE_DEVICES_LIST_H__

#include "misc_usb.h"

extern int zte_download_main(int argc, char **argv);


static fibo_usbdev_t zte_devices_table[] =
{
    {"FIBOCOM V3T/E", 0x2CB7, 0x0001, 2, zte_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM V3"   , 0x19D2, 0x0579, 2, zte_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM V3T"  , 0x2CA3, 0x4013, 2, zte_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
};

static fibo_usbdev_t zte_dl_devices_table[] =
{
    {"ZTE DL Device", 0x19d2, 0x0256, 0, zte_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1}
};

#endif

