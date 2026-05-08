#ifndef __FIREHOSE_DOWNLOAD_H__
#define __FIREHOSE_DOWNLOAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include "misc_usb.h"

#define error_return()  do {LogInfo("[%s] failed\n", __FILE__); return __LINE__; } while(0)
#define error_dbg()  do {LogInfo("[%s] failed\n", __FILE__); } while(0)

#define XML_MAX_LINE 128

struct fh_configure_cmd
{
    const char *type;
    const char *MemoryName;
    uint32_t Verbose;
    uint32_t AlwaysValidate;
    uint32_t MaxDigestTableSizeInBytes;
    uint32_t MaxPayloadSizeToTargetInBytes;
    uint32_t MaxPayloadSizeFromTargetInBytes ;          //2048
    uint32_t MaxPayloadSizeToTargetInByteSupported;     //16k
    uint32_t ZlpAwareHost;
    uint32_t SkipStorageInit;
};

struct fh_erase_cmd
{
    const char *type;
    uint32_t PAGES_PER_BLOCK;
    uint32_t SECTOR_SIZE_IN_BYTES;
    char label[MAX_PATH_LEN];
    uint32_t last_sector;
    uint32_t num_partition_sectors;
    uint32_t physical_partition_number;
    uint32_t start_sector;
};

struct fh_program_cmd
{
    const char *type;
    uint32_t erase_first;
    char filename[MAX_PATH_LEN];
    uint32_t filesz;
    uint32_t PAGES_PER_BLOCK;
    uint32_t SECTOR_SIZE_IN_BYTES;
    char label[MAX_PATH_LEN];
    uint32_t last_sector;
    uint32_t num_partition_sectors;
    uint32_t physical_partition_number;
    uint32_t start_sector;
    uint32_t erase_num_partition_sectors;
};

struct fh_response_cmd
{
    const char *type;
    const char *value;
    uint32_t rawmode;
    uint32_t MaxPayloadSizeToTargetInBytes;
};

struct fh_log_cmd
{
    const char *type;
};

struct fh_cmd_header
{
    const char *type;
    uint32_t erase_first;
};

struct fh_vendor_defines
{
    const char *type;
    char buffer[256];
};

struct fh_cmd
{
    union {
        struct fh_cmd_header cmd;
        struct fh_configure_cmd cfg;
        struct fh_erase_cmd erase;
        struct fh_program_cmd program;
        struct fh_response_cmd response;
        struct fh_log_cmd log;
        struct fh_vendor_defines vdef;
    };
    char xml_original_data[MAX_PATH_LEN];
};

struct fh_data
{
    fibo_usbdev_t *pdev;
    const char *image_dir;
    uint32_t MaxPayloadSizeToTargetInBytes;
    uint32_t fh_cmd_count;
    struct fh_cmd fh_cmd_table[XML_MAX_LINE];
    uint32_t xml_size;
    char xml_buf[1024];
    uint32_t ZlpAwareHost;
    int efs_download_flag;
};

int sahara_download_main(const char *image_dir, void *usb_handle, int edl_mode_05c69008);
int firehose_download_main(const char *image_dir, void *usb_handle, int efs_download_flag, const char *xml_dir);

#endif
