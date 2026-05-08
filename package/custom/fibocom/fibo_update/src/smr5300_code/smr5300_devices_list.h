#ifndef __SMR_DEVICES_LIST_H__
#define __SMR_DEVICES_LIST_H__

#include "misc_usb.h"

extern int samsung_download_main(int argc, char **argv);
extern int samsung_download_usb(int argc, char **argv);
extern int smr_download_pcie(int argc, char **argv);
//extern int smr_download_pcie(int argc, char **argv);

static fibo_usbdev_t smr_devices_table[] =
{
    {"SMR5300 AT", 0x04e8, 0x685e, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e01, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e02, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e03, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e04, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e06, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e07, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e08, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 AT", 0x2cb7, 0x0e09, 2,  samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},

};


static fibo_usbdev_t smr_dl_devices_table[] =
{
    {"SMR5300 DL" , 0x18d1, 0x0002, 0, samsung_download_usb, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
	//{"SMR5300 DNW", 0x04e8, 0x1100, 0, samsung_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"SMR5300 DL",  0xFFFF, 0xFF00, 2, smr_download_pcie, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1}
};

#endif

