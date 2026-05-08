#ifndef __ZTE_DOWNLOAD_H__
#define __ZTE_DOWNLOAD_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#define MAXFILENUM          50
#define MAXFILEPATH         512
#define DOWNLOAD_PACKSIZE   4096 //256

#define BSWAP_32(X)     (unsigned int)((((X)&0xFF000000) >> 24) | (((X)&0x00FF0000) >> 8) | (((X)&0x0000FF00) << 8) | (((X)&0x000000FF) << 24))

//shijiaxing 2024-0926 modify for MTC0356-1582
#if __BYTE_ORDER != __LITTLE_ENDIAN
#define ExchangeEndian(X)   (X)
#else
#define ExchangeEndian(X)   BSWAP_32(X)
#endif

#define TRUE    1
#define FALSE   0

#define nand    (0x01)
#define fs      (0x02)
#define zftl    (0x03)
#define ddr     (0x04)
#define raw     (0x05)


//cmd define
#define CMD_SYNC_TBOOT              0x5A
#define CMD_SYNC_ACK_TBOOT          0xA7
#define CMD_SYNC_BYTE_ACK_BOOTROM   0xA5

#define CMD_SEND_DATA_BOOTROM       0x7A
#define CMD_SEND_DATA_ACK_BOOTROM   0xA7

#define CMD_STARTUP_BOOTROM         0x8A
#define CMD_STARTUP_ACK_BOOTROM     0xA8

#define CMD_SEND_ADDRESS_LEN_ACK_BOOTROM    0xA1


#define PACKET_SIZE_BOOT    131072
#define PACKET_SIZE_FLASH   4096
#define PACKET_SIZE_IMAGE   131072

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned int DWORD;
typedef unsigned long ULONG;
typedef unsigned short WORD;

typedef enum {
    DOWNLOAD_TYPE_AMT,
    DOWNLOAD_TYPE_AMT_MODEM,
    DOWNLOAD_TYPE_AMT_AP,
    DOWNLOAD_TYPE_IMAGE,
    DOWNLOAD_TYPE_NV_IMAGE,
    DOWNLOAD_TYPE_NV,
    DOWNLOAD_TYPE_SN,
    DOWNLOAD_TYPE_IMEI,
    DOWNLOAD_TYPE_TESTINFO,
    DOWNLOAD_TYPE_SN_BACKUP,
    DOWNLOAD_TYPE_IMEI_BACKUP,
    DOWNLOAD_TYPE_TESTINFO_BACKUP,
    DOWNLOAD_TYPE_IN_SOFTVERSION,
    DOWNLOAD_TYPE_MAC_WIFI,
    DOWNLOAD_TYPE_SSID_FLAG,
    DOWNLOAD_TYPE_SSID,
    DOWNLOAD_TYPE_FS_FLAG,
    DOWNLOAD_TYPE_OUT_SOFTVERSION,
    DOWNLOAD_TYPE_HARDWARE_SN,
    DOWNLOAD_TYPE_TESTINFO_DL_PV,
    DOWNLOAD_TYPE_TESTINFO_DL_RELEASE,
    DOWNLOAD_TYPE_TESTINFO_DL_AMT,
    DOWNLOAD_TYPE_MAC_RJ45,
} enDownloadType;

typedef enum
{
    METHOD_NORMAL,
    METHOD_WORK_BACKUP,
    METHOD_WRITE_TO_FILE,
} enDownloadNVBackupMethod;

typedef struct
{
    BOOL bDownload;
    char *strFileName;
    BOOL bUseRelativePath;
    BOOL bReadFromFile;
    BOOL bAutoDeleteImageCache;
    BYTE *pImageCache;
    DWORD dwOffset;
    DWORD dwLength;
    int nType;
    char PartitionName[16];
    char FileName[16];
    char PartitionType[16];
    DWORD nPartitionOffset;
    ULONG ulCRCValue;
} stFileItem;


typedef struct
{
    BYTE FileId[16];
    BYTE chVersionIN[32];
    BYTE chVersionOUT[32];
    DWORD nTotalFileNum;
    DWORD nFlashType;
    DWORD nNVCoalition;
    DWORD iFileSize;
    DWORD iCkeckSum;
    DWORD iImageStructOffset;
    DWORD tloaderOffset;
    DWORD tloaderLength;
    DWORD tBootOffset;
    DWORD tBootLength;
    DWORD PartitionOffset;
    DWORD PartitionLength;
    BYTE  UnUsed[80];
} stBinMasterInfo;

typedef struct
{
    BYTE FileName[64];
    BYTE PartitionName[16];
    BYTE PartitionType[16];
    unsigned int iFileLength;
    unsigned int iPartitionOffset;
    unsigned int iFileOffset;
    BYTE UnUsed[20];
} stBinImageHead;

typedef enum
{
    STAT_SYNC,               // 0
    STAT_START_BYTE,         // 1
    STAT_NO_MODIFY_REG,      // 2
    STAT_ADDRESS,            // 3
    STAT_DATA_LEN,           // 4
    STAT_DATAPACKET,         // 5
    STAT_LINK_EST,           // 6
    STAT_DEVICEINFO,         // 7
    STAT_CONFIGTDP,          // 8
    STAT_PROG_DEV_ST_ADDR,   // 9
    STAT_PROG_DEV_END_ADDR,  // 10
    STAT_SET_PACKET_SIZE,    // 11
    STAT_CHANGE_CONFIG,      // 12
    STAT_ERASE_SEG_ST_ADDR,  // 13
    STAT_ERASE_SEG_END_ADDR, // 14
    STAT_DEV_BASE_ADDR,      // 15
    STAT_EXECUTE_CODE,       // 16
    STAT_TERMINATE,          // 17
    STAT_RVRS_DWNLD,         // 18
    STAT_UPLD_DATA_SIZE,     // 19
    STAT_SUCCESS,            // 20
    STAT_CRC_ON,
    STAT_CRC_OFF,
    // New stat add code here
} enSTAT;

typedef enum
{
    // STAT_TBOOT_OK,
    STAT_SYNC_PARTIITON,
    STAT_SET_PARTITION_CMD,
    STAT_SEND_PARTIITON,
    STAT_EXIT,
} enParpSTAT;

typedef struct
{
    enSTAT mNextTRUE;
    enSTAT mNextFALSE;
} stSTAT;

typedef struct DownloadConfig
{
    enSTAT mStartState;
    BOOL mbExecuteCode;
    stFileItem mCurrentFile;
    DWORD mPacketSize;
} stDownloadConfig;

void DumpHex(char *buf, int size);

BOOL PrimaryDoDownload(char *softwarepath);
BOOL DoDownloadBootForDL(char *softwarepath);
BOOL SendData(BYTE *pbyWriteBuffer, DWORD dwWriteCount, DWORD dwSleepAfterAction, DWORD dwTimeoutCount);
BOOL ReadData(BYTE *pbyWriteBuffer, DWORD dwWriteCount, DWORD dwSleepAfterAction, DWORD dwTimeoutCount);

void DownLoad_LogInfo(const char *filename, const char *func_name, const int line, const char *fmt, ...);

#endif
