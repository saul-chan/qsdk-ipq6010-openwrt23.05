#pragma once

#include "global_def.h"


#define AP_IMAGE_ADDR_718S 0x872000
#define AP_IMAGE_ADDR_718P 0x87e000
#define PRODUCT_NAME_LOCATION 0x190
#define EC718S "EC718S"
#define EC718P "EC718P"
#define EC716S "EC716S"
#define EC716E "EC716E"
#define XPX_USB_BASEINI_PATH "./config/config_xpk_usb_baseini.bin"




int32_t action_burn(stPrimCliCfg* priCfg);
int32_t action_burn_list(stPrimCliCfg* priCfg);
int32_t action_erase(stPrimCliCfg* priCfg);
int32_t action_erase_list(stPrimCliCfg* priCfg);
int32_t action_read_memory(stPrimCliCfg* priCfg);
int32_t action_split_pkg(stPrimCliCfg* priCfg);

int32_t reset_dut(char *cmd);
int32_t dl_boot_sync();
int32_t agent_boot_sync();
int32_t download_agent(stAgDlInfo *agInfo);
int32_t download_image(enBurnImageType imgType, char *path, uint32_t addr, enStorageType storType, uint32_t dribble_dld_en, bool bUsb, bool bSkipAddrAlign, stUsbBootCtrl* uBCtrl);
bool check_file(const char *path);
bool chekc_port(char *port);
bool check_burn_params(stPrimCliCfg* priCfg, stParseBurnCliCfg* parCfg);
bool check_erase_params(stPrimCliCfg* priCfg, stParseEraseCliCfg* parCfg);
bool check_readmem_params(stPrimCliCfg* priCfg, stParseReadMemClifCfg* parCfg);
int32_t parse_ini_handler(void *user, const char *group, const char* key, const char *value);
int32_t parse_dldupg_handler(void *user, const char *group, const char* key, const char *value);
int32_t generate_bin(FILE* fp, uint32_t size, char *binPath);
char* get_img_path(enBurnImageType imgType, stIniCfg* iniCfg);
uint32_t get_img_addr(enBurnImageType imgType, stIniCfg* iniCfg);
uint32_t get_storage_type(enBurnImageType imgType, stIniCfg* iniCfg);
uint32_t getStoreType(char* type);
int32_t parse_erase_params(char *params, uint32_t *addr, uint32_t *size, uint8_t *type);
int32_t parse_burnlist_params(char *params, char (*fList)[10], uint8_t *size);
int32_t parse_eraselist_params(char *params, char *all, char *nvm, char *cal);
int32_t parse_readmem_params(char *params, uint32_t *addr, uint32_t *size, char *path);
int32_t erase_list(char *port, char *cfgPath, char *jsonPath, char all, char nvm, char cal);
int32_t erase_one(char *port, char *cfgPath, uint32_t addr, uint32_t size, char skipAgDl, uint8_t type);
bool checkMemAddr(uint32_t addr, uint32_t len);
