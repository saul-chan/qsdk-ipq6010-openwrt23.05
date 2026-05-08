#ifndef __QCOM_DEVICES_LIST_H__
#define __QCOM_DEVICES_LIST_H__

#include "misc_usb.h"

extern int unisoc_download_main(int argc, char **argv);


static fibo_usbdev_t unisoc_devices_table[] =
{
    //UDX710
    {"UDX710 AT", 0x2CB7, 0x0A04, 2, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x2CB7, 0x0A05, 2, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x2CB7, 0x0A06, 2, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x2CB7, 0x0A07, 2, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x3C93, 0xFFFF, 3, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x3C93, 0xFFFE, 3, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},

    /*Modify for MBB0098-408 20230824 zhangboxing begin*/
    //SIJI 93 95 81 83
    {"UDX710 AT", 0x3763, 0x3C93, 1, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x2CB7, 0x0105, 2, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x3C93, 0x00FF, 3, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 AT", 0x585F, 0x0551, 3, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    /*Modify for MBB0098-408 20230824 zhangboxing end*/

    //UIC8910
    {"UIC8910 AT", 0x1782, 0x4D10, 0, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UIC8910 AT", 0x1782, 0x4D11, 7, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},

    //UIC8850
    {"UIC8850 AT", 0x2CB7, 0x0A0A, 3, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UIC8850 AT", 0x2CB7, 0x0A0B, 5, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UIC8850 AT", 0x2CB7, 0x0A0C, 5, unisoc_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
};

static fibo_usbdev_t unisoc_dl_devices_table[] =
{
    {"UDX710 DL" , 0x2CB7, 0x0A09, 0, unisoc_download_main, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UDX710 DL" , 0x1782, 0x4D00, 0, unisoc_download_main, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UIC8910 DL", 0x0525, 0xA4A7, 1, unisoc_download_main, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"UIC8850 DL", 0x1782, 0x4D16, 1, unisoc_download_main, 1, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
};

#endif

