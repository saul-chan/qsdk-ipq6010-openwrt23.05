#ifndef _STD_C_INCLUDE__
#define _STD_C_INCLUDE__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <endian.h>
#include "misc_usb.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))


#if __BYTE_ORDER == __LITTLE_ENDIAN
#ifndef le32toh
static __inline uint16_t __bswap16(uint16_t __x)
{
    return __x<<8 | __x>>8;
}
#define le16toh(x) (uint16_t)(x)
#define le32toh(x) (uint32_t)(x)
#define be16toh(x) __bswap16(x)
#endif
#endif


int unisoc_dloader_main(fibo_usbdev_t *pdev, char *package_name);

#endif
