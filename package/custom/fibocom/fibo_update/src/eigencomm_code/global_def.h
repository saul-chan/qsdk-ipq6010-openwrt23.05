#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PATH_PREFIX "./config"
#define AP_IMAGE_NAME "ap_fibo_soft"
#define INI_718S_PATH "/cfg_ec718s_usb.ini"
#define INI_718P_PATH "/cfg_ec718p_usb.ini"
#define INI_716S_PATH "/cfg_ec716s_usb.ini"
#define INI_716E_PATH "/cfg_ec716e_usb.ini"


// define
#define DLBOOT_HANDSHAKE          (0x2b02d300)
#define AGBOOT_HANDSHAKE          (0x2b02d3aa)
#define LPC_HANDSHAKE             (0x2b02d3cd)

#define IMGH_IDENTIFIER           (0x54494d48)
#define AGBT_IDENTIFIER           (0x4F424D49)
#define AIMG_IDENTIFIER           (0x444B4249)
#define CIMG_IDENTIFIER           (0x43504249)
#define FLEX_IDENTIFIER           (0x464c5849)
#define DL_COMMAND_ID             (0xcd)
#define DL_N_COMMAND_ID           (0x32)

#define CP_FLASH_IND             (0xe101)

#define DLD_CTRLMAGIC            (0x5)
#define DLD_TAILBAUD             (0xf)

// LPC
#define LPC_COMMAND_ID           (0x4c)
#define LPC_N_COMMAND_ID         (0xb3)  
#define MAX_LPC_CMD_DATA_LEN     (0x1000)
#define MAX_LPC_RSP_DATA_LEN     (0x100)
#define FIXED_LPC_RSP_LEN        (6)
#define FIXED_LPC_CMD_LEN        (8)

#define RSP_STATE_AK             (0)
#define RSP_STATE_NAK            (1)

#define FIXED_PROTOCAL_RSP_LEN   (6)
#define FIXED_PROTOCAL_CMD_LEN   (8)
#define CRC32_LEN                (4)

#define MAX_SYNC_COUNT           (50)
#define MAX_RESET_TRYCOUNT       (3)
#define MAX_RESEND_COUNT         (10)
#define MAX_DATA_BLOCK_SIZE      (0x10000)
#define MAX_PROTOCAL_CMD_SIZE    ((MAX_DATA_BLOCK_SIZE) + (FIXED_PROTOCAL_CMD_LEN) + (CRC32_LEN))
#define MAX_IMAGE_FILE_NAME_LENGTH  (64)
#define MAX_SPLIT_PKG_BUFFER_SIZE   (1024*100)
#define DEFAULT_UART_TIMEOUT_MS  (100)

#define ERASE_BLOCK_SIZE         (0x10000)

#define MAX_READ_MEM_SIZE        (240)

// enumerate
typedef enum {
    CMD_GET_VERSION = 0x20,
    CMD_SEL_IMAGE = 0x21,
    CMD_VERIFY_IMAGE = 0x22,
    CMD_DATA_HEAD = 0x31,
    CMD_DOWNLOAD_DATA = 0x32,
    CMD_DONE = 0x3a,
    CMD_DISCONNECT = 0x40,

    CMD_INVALID = 0xff,
}enCmdType;

typedef enum {
    BTYPE_BOOTLOADER = 0,
    BTYPE_AP,
    BTYPE_CP,
    BTYPE_FLEXFILE,
    BTYPE_OTHER1,
    BTYPE_OTHER2,
    BTYPE_OTHER3,
    BTYPE_OTHER4,
    BTYPE_OTHER5,

    BTYPE_HEAD,
    BTYPE_AGBOOT,
    BTYTE_DLBOOT,
    BTYPE_INVALID
}enBurnImageType;

typedef enum {
    ERASETYPE_ALL = 0,
    ERASETYPE_NVM,
    ERASETYPE_CAL,
}enEraseSectionType;

typedef enum {
    PTYPE_DLBOOT_HANDSHAKE = 0x0,
    PTYPE_AGBOOT_HANDSHAKE,

    PTYPE_GET_VERSION,
    PTYPE_SEL_IMAGE,
    PTYPE_VERIFY_IMAGE,

    PTYPE_DL_AGENT,
    PTYPE_DL_IMAGE,

    PTYPE_INVALID
}enPackageCmdType;

typedef enum {
    LPC_FLASH_ERASE = 0x10,
    LPC_READ_MEM = 0x21,
    LPC_BURN_ONE = 0x42,
    LPC_GET_BURN_STATUS = 0x44,
    LPC_SYS_RST = 0xaa,
}enLpcCmdType;

typedef enum {
    SYNC_HANDSHAKE_DLBOOT = 0x0,
    SYNC_HANDSHAKE_AGBOOT,
    SYNC_HANDSHAKE_LPC
}enSynHandshakeType;

typedef enum {
    HTYPE_NOHASH = 0x0,
    HTYPE_SWHASH,
    HTYPE_HWHASH
}enHashType;

typedef enum {
    CHKTYPE_CRC8 = 0x0,
    CHKTYPE_CRC32,
    CHKTYPE_SELDEF1
}enCheckType;

typedef enum {
    ETYPE_BLOCK = 0x0,
    ETYPE_CHIP = 0x1
}enEraseType;

typedef enum {
    STYPE_AP_FLASH = 0x0,
    STYPE_CP_FLASH,

    STYPE_INVALID = 0xFF
}enStorageType;
// struct
typedef struct
{
    char port[30];
    char cfgFile[256];
    char action[128]; // burn/burnlist/erase/eraselist/readmem
    char reset; //  0: reset disable; 1: reset enable;
    char skipAgDl; // 0: skip disable; 1: skip enable;
    char burnFileType[40]; // BL/AP/CP/FF
    char burnFileList[40];
    char eraseSection[32];
    char eraseList[30];
    char readSection[280];
}stPrimCliCfg;

typedef struct {
    char port[30];
    char cfgFile[256];
    enBurnImageType bFileType;
    char skipAgDl;
    char reset;
}stParseBurnCliCfg;

typedef struct {
    char port[30];
    char cfgFile[256];
    char skipAgDl;
    uint32_t eraseStartAddr;
    uint32_t eraseLen;
    uint8_t eraseType;
}stParseEraseCliCfg;

typedef struct {
    char port[30];
    char cfgFile[256];
    char skipAgDl;
    char filePath[256];
    uint32_t readStartAddr;
    uint32_t readLen;
}stParseReadMemClifCfg;

typedef struct {
    uint32_t agBaud;
    uint32_t dlBaud;
    uint32_t detect;
    char rstAtCmd[30];
    uint32_t atBaud;
    uint32_t pullupQspi;
    uint32_t dribble_dld_en;
    uint32_t debug;
    uint32_t uart_timeout;
    uint32_t usb_enable;
    uint32_t skip_addr_align;
    char formatPath[256];
    char pkgPath[256];
    char agPath[256];
    char blPath[256];
    uint32_t blBurnAddr;
    char sysPath[256];
    uint32_t sysnBurnAddr;
    char cpSysPath[256];
    uint32_t cpSysBurnAddr;
    char cpSysStorType[20];
    char flexF0Path[256];
    uint32_t flexF0BurnAddr;
    char flexF0StorType[20];
    char otherF1Path[256];
    uint32_t otherF1BurnAddr;
    char otherF1StorType[20];
    char otherF2Path[256];
    uint32_t otherF2BurnAddr;
    char otherF2StorType[20];
    char otherF3Path[256];
    uint32_t otherF3BurnAddr;
    char otherF3StorType[20];
    char otherF4Path[256];
    uint32_t otherF4BurnAddr;
    char otherF4StorType[20];
    char otherF5Path[256];
    uint32_t otherF5BurnAddr;
    char otherF5StorType[20];

    uint32_t agCtrlMagic;
    uint32_t apSize;
    uint32_t cpSIze;
    uint8_t dld_upg_ctrl_valid;
    uint32_t dld_upg_connwait_100ms_cnt;
    uint32_t dld_upg_ctrlwait_100ms_cnt;

}stIniCfg;

typedef struct {
    uint8_t dld_upg_ctrl_valid;
    uint32_t dld_upg_connwait_100ms_cnt;
    uint32_t dld_upg_ctrlwait_100ms_cnt;
}stDldUpg;


typedef struct {
    enCmdType type;
    uint8_t cmd_id;
}stCmdInfo;

typedef struct {
    const char     *typeName;
    enBurnImageType type;
}stImageTypeInfo;

typedef struct {
    const char     *typeName;
    enEraseSectionType type;
}stEraseTypeInfo;

typedef struct {
    uint8_t cmd;
    uint8_t index;
    uint8_t order_id;
    uint8_t norder_id;
    uint32_t  len;
    uint8_t data[4];
    uint32_t  fcs;
}stCmd;

typedef struct
{
    uint8_t cmd;
    uint8_t index;
    uint8_t order_id;
    uint8_t norder_id;
    uint8_t state;
    uint8_t  len;
    uint8_t data[12];   
    uint32_t  fcs;
}stRsp;

typedef struct {
    uint8_t cmd;
    uint8_t index;
    uint8_t order_id;
    uint8_t norder_id;
    uint32_t  len;
    uint8_t data[MAX_LPC_CMD_DATA_LEN];
    uint32_t  fcs;
}stLpcCmd;

typedef struct
{
    uint8_t cmd;
    uint8_t index;
    uint8_t order_id;
    uint8_t norder_id;
    uint8_t state;
    uint8_t  len;
    uint8_t data[MAX_LPC_RSP_DATA_LEN];   
    uint32_t  fcs;
}stLpcRsp;

typedef struct
{
    uint32_t vVal;     
    uint32_t id; 
    uint32_t dtm; 
    uint32_t rsvd;
}stVersionInfo; // 16bytes

typedef struct{
    uint8_t hashtype;  
    uint8_t loadtype;
    uint16_t baudratectrl;  
}stCtlInfo; // 4bytes

typedef struct{
    uint8_t ctrlMagic : 4;
    uint8_t cpFlashSizeMB: 4;
    uint8_t apFlashSizeMB;
    uint8_t rsvd;
    uint8_t chkSum;
}stAgDlUpgCtrl;

typedef struct {
    uint8_t ctrlMagic : 4;  // 0x5 valid, other not valid
    uint8_t tailBaud : 4;  //
    uint8_t connWait100MsCnt; //     1->100ms , 255->25500 25s
    uint8_t hostCtrlWait100MsCnt; //     1->100ms , 255->25500 25s
    uint8_t chkSum;
}stDldUpgradeCtrl;

typedef union
{
    stAgDlUpgCtrl ctrl1;
    stDldUpgradeCtrl ctrl2;
    uint8_t data[4];
}unAgDlUpgCtrl;

typedef struct
{
    uint32_t id;
    uint32_t burnaddr;
    uint32_t ldloc;
    uint32_t size;
    unAgDlUpgCtrl agDlCtrl;
    uint8_t  reserve[12];
    uint8_t  hashv[32];
    uint8_t  ecdsasign[64];
    uint8_t  pubkey[64];
}stImgBody; // 192bytes

typedef struct
{
    uint8_t dld_upg_ctrl_valid;
    uint32_t dld_upg_connwait_100ms_cnt;
    uint32_t dld_upg_ctrlwait_100ms_cnt;
}stUsbBootCtrl;

typedef struct
{
    uint32_t rsvdAreaId;
    uint32_t rsvdAreaSize;
    uint8_t  rsvd[8];
}stReservedArea; // 16bytes

typedef struct
{
    uint8_t byte0_pullup_qspi:1;
    uint8_t byte0_dribble_dld_en:1;
    uint8_t byte0_dribble_cur_valid:1; //when dribble dld en is 1, the cur valid used to check the burnaddr
    uint8_t byte0_rsvd:5;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
}stImgHdRsvd0;

typedef struct
{
    stVersionInfo  verinfo; // 16
    uint32_t       imgnum;  // 4
    stCtlInfo      ctlinfo; // 4
    stImgHdRsvd0   rsvd0;   // 4
    uint32_t       rsvd1;   // 4
    uint8_t        hashih[32]; // 32
    stImgBody      imgbody; // 192
    stReservedArea rsvdarea; // 16
}stImgHead; // 272bytes

typedef struct
{
    char        ver[8];
    uint32_t    magicnumber;
    uint32_t    date;
    uint32_t    imagenumber;
    char        crypt[8];
    uint16_t    vt;
    uint16_t    vtsize;
    uint32_t    rsvd;
    uint32_t    rsvd2;
    uint32_t    rsvd3;
    uint32_t    rsvd4;
    // char        *pData;
    uint32_t    pData;
}stFileHeaderExt; // 52bytes

typedef struct
{
    char        name[MAX_IMAGE_FILE_NAME_LENGTH];
    uint32_t    addr;
    uint32_t    flashsize;
    uint32_t    offset;
    uint32_t    size;   // image size
    char        hash[256];
    char        type[16];
    uint16_t    vt;
    uint16_t    vtsize;
    uint32_t    rsvd;
    // char        *pData;
    uint32_t    pData;
}stFieldInfoExt; // 364bytes

typedef struct
{
    char  path[256];
    uint32_t baud;
    enHashType type;
    uint32_t pullupQspi;
    uint32_t ctrlMaigc;
    uint32_t apSize;
    uint32_t cpSize;
}stAgDlInfo;

typedef struct 
{
    enBurnImageType type;
    uint8_t hashV[32];
    uint32_t imgSize;
    uint32_t baud;
    uint32_t addr;
    enHashType hType;
    bool bDlBoot;
    uint32_t pullupQspi;
    uint32_t ctrlMaigc;
    uint32_t apSize;
    uint32_t cpSize;
    uint32_t dribble_dld_en;
    bool bUsb;
    uint8_t dld_upg_ctrl_valid;
    uint32_t dld_upg_connwait_100ms_cnt;
    uint32_t dld_upg_ctrlwait_100ms_cnt;

}stImgHeaderInfo;

typedef enum 
{
    VTE_NULL = 0,
    VTE_ATTRVAL = 1,
    VTE_ATTRSTR = 2,
    VTE_MACROTUPLEVAL = 3,
    VTE_OLDTOOLTAG = 4,
    VTE_END
}VTENUM;

typedef struct 
{
    char        name[64];
    uint32_t    val;
}AttributeVal;

typedef struct 
{
    char    name[64];
    char    str[64];
}AttributeString;

typedef struct 
{
    AttributeVal val1;
    AttributeVal val2;
}MacroTupleVal;

typedef struct 
{
    uint16_t vt;
    union 
    {
        AttributeVal attrval;
        AttributeString attrstr;
        MacroTupleVal tpval;
    };
}VARIANTARGEXT;

// const data
static const stImageTypeInfo gImage_type_info[] = {
    { "AP",     BTYPE_AP },
    { "CP",     BTYPE_CP },
    { "BL", 	BTYPE_BOOTLOADER },
    { "FF",     BTYPE_FLEXFILE },
    { "OTHER1",   BTYPE_OTHER1 },
    { "OTHER2",   BTYPE_OTHER2 },
    { "OTHER3",   BTYPE_OTHER3 },
    { "OTHER4",   BTYPE_OTHER4 },
    { "OTHER5",   BTYPE_OTHER5 }
};

static const stEraseTypeInfo gErase_type_info[] = {
    { "ALL",    ERASETYPE_ALL },
    { "NVM",    ERASETYPE_NVM },
    { "CAL",    ERASETYPE_CAL }
};
