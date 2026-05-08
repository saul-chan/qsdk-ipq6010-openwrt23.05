#ifndef _CTX_H__
#define _CTX_H__

#include "misc_usb.h"

#define FRAMESZ_DATA_SMR        12*1024  // frame size for others

typedef struct {
//<!--  File-ID: Can not be changed,it is used by tools -->
    char FileID[32];
    char partition_name[64];
    char file_path[256];
} Scheme_t;

typedef struct {
    char FileID[32];
    char file_path[128];
} Edl_t;

typedef enum {
    NONE,
    SMR5300
} cputype_enum;

typedef struct {
    fibo_usbdev_t *pdev;
    cputype_enum platform;
    FILE *fp;
    size_t cur_bin_offset;
    uint8_t file_req_data[FRAMESZ_DATA_SMR+32];
    uint8_t file_rsp_data[FRAMESZ_DATA_SMR];
} file_ctx_t;


#endif
