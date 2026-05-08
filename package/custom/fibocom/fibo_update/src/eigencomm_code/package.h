#pragma once

#include "global_def.h"
#include "linux_comm.h"



int32_t burn_sync(enSynHandshakeType pType, int counter);
int32_t burn_agboot(stAgDlInfo *agInfo);
int32_t burn_image(enBurnImageType imgType, char *path, uint32_t addr, enStorageType storType, uint32_t dribble_dld_en, bool bUsb, bool bSkipAddrAlign, stUsbBootCtrl* uBCtrl);
int32_t erase_flash(enEraseType type, uint32_t addr, uint32_t size);
int32_t read_mem(uint32_t addr, uint32_t size, char* path);
int32_t sys_reset();
int32_t at_command(char *cmd);
int32_t init_port(char *port, uint32_t baud, uint32_t timeout);
int32_t free_port();


int32_t send_data(uint8_t* data, uint64_t size);
int32_t receive_data(uint8_t* buffer, uint64_t size);
int32_t clear_r_buffer();
int32_t package_data(uint8_t* data, uint32_t pureDataSize, bool bDlBoot);
int32_t package_data_single(uint8_t* data, bool bDlBoot);
int32_t package_data_head(uint32_t len, uint32_t *tbSize, bool bDlBoot);
int32_t package_image_head(stImgHeaderInfo *imgHeaderInfo);
int32_t package_base_info(enBurnImageType type, bool bDlBoot);
int32_t package_get_version(bool bDlBoot);
int32_t package_sel_image(enBurnImageType type, bool bDlBoot);
int32_t package_verify_image(bool bDlBoot);
int32_t package_done(bool bDlBoot);
int32_t package_lpc_burn_one(enBurnImageType type, enStorageType storType);
int32_t package_lpc_erase(enEraseType type, uint32_t addr, uint32_t len);
int32_t package_lpc_get_burn_status();
int32_t package_lpc_sys_reset();
int32_t package_lpc_readmem(uint32_t addr, uint32_t len, uint32_t *readLen, uint8_t *buf);
int32_t send_recv_Cmd(uint8_t *s_data, uint8_t *r_data, uint8_t *r_len, bool bDlBoot);
int32_t send_recv_lpcCmd(uint8_t *s_data, uint8_t *r_data, uint8_t *r_len);
uint8_t get_hashtype(enHashType type);
uint32_t get_imageid(enBurnImageType type);
uint32_t self_def_check1(uint8_t *in);
bool check_cmd_bwt_send_recv(uint8_t *send, uint8_t* recv);
void init_image_header(stImgHead *p);
int32_t get_img_sh256_value(char *path, uint8_t *hash);