#ifndef __QCOM_DEVICES_LIST_H__
#define __QCOM_DEVICES_LIST_H__

#include "misc_usb.h"

extern int qcom_download_main(int argc, char **argv);


static fibo_usbdev_t qcom_devices_table[] =
{
    /* ModuleName, VID, PID, usb interface number, main_function */
    //mdm9x07
    {"FIBOCOM MDM9X07", 0x1508, 0x1001, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM MDM9X07", 0x2ca3, 0x4009, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM MDM9X07", 0x2CB7, 0x0103, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x0104, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x0105, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x0106, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x010B, 3, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x010C, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x010D, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x010E, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x0110, 3, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x0111, 3, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM SDX"    , 0x2CB7, 0x01A0, 2, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},

    //mdm9205
    {"FIBOCOM MDM9205", 0x2CB7, 0x0106, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM MDM9205", 0x2CB7, 0x01A5, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},

    //QCOM
    {"FIBOCOM MDM9X07", 0x05C6, 0x9025, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM MDM9X07", 0x05C6, 0x90B2, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM MDM9X07", 0x05C6, 0x90B6, 3, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},
    {"FIBOCOM MDM9X07", 0x05C6, 0x90DB, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},

    //SDX35
    {"FIBOCOM SDX35", 0x2CB7, 0x0112, 1, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,2},
    {"FIBOCOM SDX35", 0x2CB7, 0x0113, 2, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,3},
    {"FIBOCOM SDX35", 0x2CB7, 0x0114, 2, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,3},
    {"FIBOCOM SDX35", 0x2CB7, 0x0115, 3, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,4},
    {"FIBOCOM SDX35", 0x2CB7, 0x0117, 1, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,2},
    {"FIBOCOM SDX35", 0x2CB7, 0x0118, 2, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,3},
    {"FIBOCOM SDX35", 0x2CB7, 0x0119, 3, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,4},

    //AVM
    {"FIBOCOM SDX12", 0x2CB7, 0x0116, 3, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0,-1},

};

static fibo_usbdev_t dl_qcom_devices_table[] =
{
    {"QCOM DL", 0x05C6, 0x9008, 0, qcom_download_main, 0, {0}, {0}, {0}, {0},0,0,0,0,0,0,0,NULL,NULL,0},
};

#endif

