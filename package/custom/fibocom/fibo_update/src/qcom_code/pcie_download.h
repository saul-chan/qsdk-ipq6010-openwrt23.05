#ifndef __PCIE_DOWNLOAD__H__
#define __PCIE_DOWNLOAD__H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "misc_usb.h"


#define IOCTL_BHI_GETDEVINFO 0x8BE0 + 1
#define IOCTL_BHI_WRITEIMAGE 0x8BE0 + 2

#define COMMON_MODE 0
#define EDL_MODE 1

typedef unsigned int ULONG;

typedef struct _bhi_info_type
{
   ULONG bhi_ver_minor;
   ULONG bhi_ver_major;
   ULONG bhi_image_address_low;
   ULONG bhi_image_address_high;
   ULONG bhi_image_size;
   ULONG bhi_rsvd1;
   ULONG bhi_imgtxdb;
   ULONG bhi_rsvd2;
   ULONG bhi_msivec;
   ULONG bhi_rsvd3;
   ULONG bhi_ee;
   ULONG bhi_status;
   ULONG bhi_errorcode;
   ULONG bhi_errdbg1;
   ULONG bhi_errdbg2;
   ULONG bhi_errdbg3;
   ULONG bhi_sernum;
   ULONG bhi_sblantirollbackver;
   ULONG bhi_numsegs;
   ULONG bhi_msmhwid[6];
   ULONG bhi_oempkhash[48];
   ULONG bhi_rsvd5;
} BHI_INFO_TYPE, *PBHI_INFO_TYPE;

enum MHI_EE {
   MHI_EE_PBL  = 0x0,            /* Primary Boot Loader */
   MHI_EE_SBL  = 0x1,            /* Secondary Boot Loader   */
   MHI_EE_AMSS = 0x2,            /* AMSS Firmware   */
   MHI_EE_RDDM = 0x3,            /* WIFI Ram Dump Debug Module  */
   MHI_EE_WFW  = 0x4,            /* WIFI (WLAN) Firmware    */
   MHI_EE_PT   = 0x5,            /* PassThrough, Non PCIe BOOT (PCIe is BIOS locked, not used for boot */
   MHI_EE_EDL  = 0x6,            /* PCIe enabled in PBL for emergency download (Non PCIe BOOT)  */
   MHI_EE_FP   = 0x7,            /* FlashProg, Flash Programmer Environment */
   MHI_EE_BHIE = MHI_EE_FP,
   MHI_EE_UEFI = 0x8,            /* UEFI    */

   MHI_EE_DISABLE_TRANSITION = 0x9,
   MHI_EE_MAX
};

fibo_usbdev_t *pcie_open_dl(char *firmware_name, char *pcie_portname);

#endif
