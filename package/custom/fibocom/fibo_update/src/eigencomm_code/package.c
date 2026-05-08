#include <stdio.h>
#include <string.h>
#include "package.h"
#include "utils.h"
#include "crc.h"
#include "sha256.h"

uint32_t g_data_block_size = 0;
uint32_t g_remain_size = 0;

int32_t init_port(char *port, uint32_t baud, uint32_t timeout)
{
    return open_port(port, baud, timeout);
}

int32_t free_port()
{
    return close_port();
}

int32_t burn_sync(enSynHandshakeType pType, int counter)
{
    int32_t i, j;
    int32_t ret = 0;
    uint8_t writeBuf[20];
    uint8_t readBuf[20];
    memset(writeBuf, 0, sizeof(writeBuf));
    memset(readBuf, 0, sizeof(readBuf));
    uint32_t handshake;
    switch(pType)
    {
        case SYNC_HANDSHAKE_DLBOOT:
            handshake = DLBOOT_HANDSHAKE;
            ILOG("Dl boot handshake");
            break;
        case SYNC_HANDSHAKE_AGBOOT:
            ILOG("Ab handshake");
            handshake = AGBOOT_HANDSHAKE;
            break;
        case SYNC_HANDSHAKE_LPC:
            ILOG("Lpc handshake");
            handshake = LPC_HANDSHAKE;
            break;
        default:
            break;
    }
    memcpy(writeBuf, (uint8_t*)&handshake, sizeof(handshake));
    for (i=0; i<MAX_SYNC_COUNT; i++)
    {
        clear_r_buffer();
        memset(readBuf, 0, sizeof(readBuf));
        for (j=0; j<counter; j++)
        {
            ret = send_data(writeBuf, 4);
            m_sleep(2);
        } 
        if (ret == 0)
        {
            ret = receive_data(readBuf, 4);
            if (ret == 0)
            {
                // while in uart download mode, if dut state is abnormal, may return the 4 right
                // symbol and other unexpected symbols, and this is abnormal,need to continue to
                // sync until only receive 4 symbols. it is necessary while in the first sync in dl boot.
                if (pType == SYNC_HANDSHAKE_DLBOOT)
                {
                    ret = receive_data(readBuf+4, 1);
                    if ((ret == 0) && (readBuf[4] != 0))
                        continue;
                }
                if (uchar_array_equal(readBuf, writeBuf, 4))
                    break;
                else
                    continue;
            }
            else
                continue;
        }
        else
        {
            DLOG("Sync data send fail, handshake type = %u", (uint32_t)pType);
            return -1;
        }           
    }
    clear_r_buffer();

    if (i == MAX_SYNC_COUNT)
    {
        DLOG("Sync %u times and fail", MAX_SYNC_COUNT);
        return -1;
    }
    if (pType == SYNC_HANDSHAKE_DLBOOT)
    {
        ILOG("DlBoot Success.");
    }
    return 0;
}

int32_t burn_agboot(stAgDlInfo *agInfo)
{
    int32_t ret;
    uint32_t imgSize;
    uint8_t *imgBuf = NULL;
    FILE *fp = NULL;
    stCmd *pCmd = NULL;
    uint8_t imgHashV[32];

    DLOG("Burn agent boot start");
    /// 1. base info sync
    ret = package_base_info(BTYPE_HEAD, true);
    if (ret != 0)
    {
        return -1;
    }
    DLOG("burn_agboot base info sync\r\n");
    /// 2. download agent header
    imgSize = get_file_size(agInfo->path);
    if (imgSize == 0)
    {
        return -1;
    }
    imgBuf = (uint8_t*)xmalloc(imgSize);
    fp = fopen(agInfo->path, "rb");
    if (fread(imgBuf, 1, imgSize, fp) != imgSize)
    {
        xfree(imgBuf);
        return -1;
    }
    fclose(fp);
    DLOG("burn_agboot download agent header\r\n");
    sha256_calc(imgBuf, imgSize, imgHashV);
    stImgHeaderInfo imgHeaderInfo;
    imgHeaderInfo.type = BTYPE_AGBOOT;
    memcpy(imgHeaderInfo.hashV, imgHashV, sizeof(imgHashV));
    imgHeaderInfo.imgSize = imgSize;
    imgHeaderInfo.baud = agInfo->baud;
    imgHeaderInfo.addr = 0;
    imgHeaderInfo.hType = HTYPE_NOHASH;
    imgHeaderInfo.bDlBoot = true;
    imgHeaderInfo.pullupQspi = agInfo->pullupQspi;
    imgHeaderInfo.ctrlMaigc = agInfo->ctrlMaigc;
    imgHeaderInfo.apSize = agInfo->apSize;
    imgHeaderInfo.cpSize = agInfo->cpSize;

    ret = package_image_head(&imgHeaderInfo);
    xfree(imgBuf);
    if (ret != 0)
    {
        return -1;
    }
    DLOG("burn_agboot package image head\r\n");
    /// 3. download agentboot
    // dlboot sync
    ret = burn_sync(SYNC_HANDSHAKE_DLBOOT, 2);
    if (ret != 0)
    {
        return -1;
    }
    DLOG("burn_agboot dlboot sync\r\n");
    ret = package_base_info(BTYPE_BOOTLOADER, true);
    if (ret != 0)
    {
        return -1;
    } 
    DLOG("burn_agboot package base info\r\n");
    // download agentboot data
    imgBuf = (uint8_t*)xmalloc(imgSize + FIXED_PROTOCAL_CMD_LEN + CRC32_LEN);
    fp = fopen(agInfo->path, "rb");
    if (fread(imgBuf+FIXED_PROTOCAL_CMD_LEN, 1, imgSize, fp) != imgSize)
    {
        xfree(imgBuf);
        return -1;
    }
    fclose(fp);
    DLOG("burn_agboot download agentboot data\r\n");
    pCmd = (stCmd *)imgBuf;
    pCmd->cmd = CMD_DOWNLOAD_DATA;
    pCmd->index = 0;
    pCmd->order_id = DL_COMMAND_ID;
    pCmd->norder_id = DL_N_COMMAND_ID;
    pCmd->len = imgSize;
    ret = package_data(imgBuf, imgSize, true);
    xfree(imgBuf);
    if (ret != 0)
    {
        return -1;
    }
    DLOG("Burn agent boot success");
    return ret;
}

int32_t burn_image(enBurnImageType imgType, char *path, uint32_t addr, enStorageType storType, uint32_t dribble_dld_en, bool bUsb, bool bSkipAddrAlign, stUsbBootCtrl* uBCtrl)
{
    int32_t ret;
    //uint16_t i;
    uint32_t imgSize, remain, len;
    uint8_t *imgBuf = NULL;
    FILE *fp = NULL;
    uint8_t hashV[32];
    stCmd *pCmd = NULL;
    uint32_t fReadLen = 0;
    bool firstCheckFlag = false;
    /// 1. lpc sync
    DLOG("burn_image lpc sync\r\n");
    ret = burn_sync(SYNC_HANDSHAKE_LPC, 2);
    if (ret != 0)
    {
        return -1;
    }
    /// 2. lpc burn one
    DLOG("burn_image lpc burn one\r\n");
    ret = package_lpc_burn_one(imgType, storType);
    if (ret != 0)
    {
        return -1;
    }
    /// 3. agboot sync
    DLOG("burn_image agboot sync\r\n");
    ret = burn_sync(SYNC_HANDSHAKE_AGBOOT, 2);
    ret |= burn_sync(SYNC_HANDSHAKE_AGBOOT, 2);
    if (0 != ret)
    {
        return -1;
    }
    /// 4. base info sync
    DLOG("burn_image base info sync\r\n");
    ret = package_base_info(BTYPE_HEAD, false);
    if (ret != 0)
    {
        return -1;
    }
    /// 5. image head sync
    DLOG("burn_image image head sync\r\n");
    imgSize = get_file_size(path);
    if (imgSize == 0)
    {
        DLOG("burn_image imgSize 0\r\n");
        return -1;
    }
    DLOG("burn_image sh256\r\n");
    get_img_sh256_value(path, hashV);
    ILOG("imgType = %d, imgSize = %u, addr = 0x%x", (int)imgType, imgSize, addr);
    stImgHeaderInfo imgHeaderInfo;
    imgHeaderInfo.type = imgType;
    memcpy(imgHeaderInfo.hashV, hashV, sizeof(hashV));
    imgHeaderInfo.imgSize = imgSize;
    imgHeaderInfo.baud = 0;
    imgHeaderInfo.addr = addr;
    imgHeaderInfo.hType = HTYPE_NOHASH;
    imgHeaderInfo.bDlBoot = false;
    imgHeaderInfo.pullupQspi = 0;
    imgHeaderInfo.ctrlMaigc = 0;
    imgHeaderInfo.apSize = 0;
    imgHeaderInfo.cpSize = 0;
    imgHeaderInfo.dribble_dld_en = dribble_dld_en;
    imgHeaderInfo.bUsb = bUsb;
    if(imgHeaderInfo.type  == BTYPE_BOOTLOADER)
    {   
        imgHeaderInfo.dld_upg_ctrl_valid = uBCtrl->dld_upg_ctrl_valid;
        imgHeaderInfo.dld_upg_connwait_100ms_cnt = uBCtrl->dld_upg_connwait_100ms_cnt;
        imgHeaderInfo.dld_upg_ctrlwait_100ms_cnt = uBCtrl->dld_upg_ctrlwait_100ms_cnt;
    }
    ret = package_image_head( &imgHeaderInfo);
    if (0 != ret)
    {
        DLOG("burn_image image head error\r\n");
        return -1;
    }
    /// 6. image download
    DLOG("burn_image image download\r\n");
    remain = imgSize;
    imgBuf = (uint8_t*)xmalloc(MAX_DATA_BLOCK_SIZE + FIXED_PROTOCAL_CMD_LEN + CRC32_LEN);
    fp = fopen(path, "rb"); 
    while (remain > 0)
    {
        /// agboot sync
        ret = burn_sync(SYNC_HANDSHAKE_AGBOOT, 2);
        if(ret != 0)
        {
            break;
        }
        /// base info sync
        ret = package_base_info(imgType, false);
        if (ret != 0)
        {
            break;
        }
        if (!firstCheckFlag && (addr % MAX_DATA_BLOCK_SIZE != 0) && remain > MAX_DATA_BLOCK_SIZE && !bUsb && !bSkipAddrAlign) 
        {
            len = MAX_DATA_BLOCK_SIZE - (addr % MAX_DATA_BLOCK_SIZE);
            firstCheckFlag = true;
        }
        else if (remain > MAX_DATA_BLOCK_SIZE)
            len = MAX_DATA_BLOCK_SIZE;
        else
            len = remain;

        fReadLen = fread(imgBuf+FIXED_PROTOCAL_CMD_LEN, 1, len, fp);
        if (fReadLen != len)
        {
            DLOG("fread length != wanted length, freadLen = %u, wantedLen = %u", fReadLen, len);
            // len = fReadLen;
            break;
        }

        pCmd = (stCmd *)imgBuf;
        pCmd->cmd = CMD_DOWNLOAD_DATA;
        pCmd->index = 0;
        pCmd->order_id = DL_COMMAND_ID;
        pCmd->norder_id = DL_N_COMMAND_ID;
        pCmd->len = len;
        ret = package_data(imgBuf, len, false);
        if (ret != 0)
        {
            break;
        } 
        remain -= len;   
        ILOG("Current progress: %d%%", ((imgSize-remain)*100/imgSize));
    }

    fclose(fp);
    xfree(imgBuf);

    if (remain > 0)
    {
        DLOG("Burn image fail, remain = %u", remain);
        return -1;
    }

    ret =  package_lpc_get_burn_status();

    return ret;
}

int32_t erase_flash(enEraseType type, uint32_t addr, uint32_t size)
{
    uint32_t remain;
    uint32_t len;
    uint32_t tmpAddr;

    /// 1. lpc sync
    if (burn_sync(SYNC_HANDSHAKE_LPC, 2) != 0)
    {
        return -1;
    }

    if (type == ETYPE_CHIP)
    {
        return package_lpc_erase(type, addr, size);
    }

    remain = size;
    tmpAddr = addr;
    while (remain > 0)
    {
        if (remain > ERASE_BLOCK_SIZE)
            len = ERASE_BLOCK_SIZE;
        else
            len = remain;
        if (0 != package_lpc_erase(type, tmpAddr, len))
        {
            DLOG("Erase fail, addr = %u, size = %u", tmpAddr, len);
            return -1;
        }
        remain -= len;
        tmpAddr += len;
    }
    if (remain != 0)
    {
        return -1;
    }

    return 0;   
}

int32_t read_mem(uint32_t addr, uint32_t size, char* path)
{
    int32_t remain;
    uint32_t len;
    uint32_t tmpAddr;
    uint32_t readLen;
    FILE *fp;

    uint8_t buf[MAX_READ_MEM_SIZE * 2];
    /// 1. lpc sync
    if (burn_sync(SYNC_HANDSHAKE_LPC, 2) != 0)
    {
        return -1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL)
    {
        // fclose(binF);
        ELOG("File could not access or has no permission to handle, path = %s", path);
        return -1;
    }
    remain = size;
    while(remain >= 0)
    {
        memset(buf, 0, sizeof(buf));
        readLen = 0;
        len =  (remain > MAX_READ_MEM_SIZE) ? (MAX_READ_MEM_SIZE) : (remain);
        tmpAddr = addr + (size - remain);
        ILOG("tmpAddr = %u, len = %u", tmpAddr, len);
        if ((0 == package_lpc_readmem(tmpAddr, len, &readLen, &buf[0])) && (readLen == len))
        {
            ILOG("LOG1");
            if (len != fwrite(buf, 1, len, fp))
            {
                ELOG("Write file fail, file_path = %s", path);
                if (fp != NULL)
                    fclose(fp);
                return -1;
            }
        }
        else
        {
            ILOG("LOG2");
            ELOG("ReadMem fail, addr = %u, size = %u, readLen = %u", tmpAddr, len, readLen);
            if (fp != NULL)
                fclose(fp);
            return -1;
        }
        remain -= MAX_READ_MEM_SIZE; 
        ILOG("remain = %d", remain);
    }
    fflush(fp);
    fclose(fp);

    return 0; 
}

int32_t sys_reset()
{
    int32_t ret;

    /// 1. lpc sync
    ret = burn_sync(SYNC_HANDSHAKE_LPC, 2);
    if (ret != 0)
    {
        return -1;
    }
    return package_lpc_sys_reset();
}

int32_t at_command(char *cmd)
{
    int32_t ret;
    uint8_t tmpCmd[50];
    uint8_t recvBuf[100];
    uint8_t len;
    uint8_t retryCount;
    
    memset(tmpCmd, 0, sizeof(tmpCmd));
    len = strlen((const char*)cmd);
    memcpy(tmpCmd, cmd, len);
    if (!endswith((const char*)tmpCmd, (const char*)cmd))
    {
        tmpCmd[len] = 0xd;
        tmpCmd[len+1] = 0xa;
        len += 2;
    }

    retryCount = MAX_RESET_TRYCOUNT;
    do
    {
        memset(recvBuf, 0, sizeof(recvBuf));
        ret = send_data(tmpCmd, len);
        if (ret == 0)
        {
            ret = receive_data(recvBuf, sizeof(recvBuf));
            clear_r_buffer();
            if (strstr((const char*)recvBuf, "OK") != NULL)
            {
                return 0;
            }
        }
    } while (retryCount-- > 0);
    
    return -1;
}

int32_t package_image_head(stImgHeaderInfo *imgHeaderInfo)
{
    int32_t ret;
    stImgHead imgHd;
    uint8_t *cmdBuf = NULL;
    stCmd *pCmd = NULL;

    init_image_header(&imgHd);
    imgHd.imgbody.id = get_imageid(imgHeaderInfo->type);
    imgHd.imgbody.size = imgHeaderInfo->imgSize;
    imgHd.imgbody.burnaddr = imgHeaderInfo->addr;
    if (imgHeaderInfo->dld_upg_ctrl_valid == 0) {
        stAgDlUpgCtrl *agDlCtrl = (stAgDlUpgCtrl*)(imgHd.imgbody.agDlCtrl.data);
        agDlCtrl->ctrlMagic = (imgHeaderInfo->ctrlMaigc & 0xF);
        agDlCtrl->cpFlashSizeMB = (imgHeaderInfo->cpSize & 0xF);
        agDlCtrl->apFlashSizeMB = (imgHeaderInfo->apSize & 0xFF);
        agDlCtrl->rsvd = 0;
        agDlCtrl->chkSum = (agDlCtrl->ctrlMagic + agDlCtrl->cpFlashSizeMB + agDlCtrl->apFlashSizeMB + agDlCtrl->rsvd) & 0xFF;
    }
    else
    {
        stDldUpgradeCtrl *agDlCtrl = (stDldUpgradeCtrl*)(imgHd.imgbody.agDlCtrl.data);
        agDlCtrl->ctrlMagic = DLD_CTRLMAGIC;
        agDlCtrl->tailBaud = DLD_TAILBAUD;
        agDlCtrl->connWait100MsCnt = (imgHeaderInfo->dld_upg_connwait_100ms_cnt);
        agDlCtrl->hostCtrlWait100MsCnt = (imgHeaderInfo->dld_upg_ctrlwait_100ms_cnt);
        agDlCtrl->chkSum = (agDlCtrl->ctrlMagic + agDlCtrl->tailBaud + agDlCtrl->connWait100MsCnt + agDlCtrl->hostCtrlWait100MsCnt) & 0xFF;

    }

    memcpy(imgHd.imgbody.hashv, imgHeaderInfo->hashV, 32);
    if (imgHeaderInfo->baud != 0)
    {
        imgHd.ctlinfo.baudratectrl = ((imgHeaderInfo->baud/100) & 0x7FFF) + 0x8000;
    }
    else{
        imgHd.ctlinfo.baudratectrl = 0;
    }
    
    imgHd.ctlinfo.hashtype = get_hashtype(imgHeaderInfo->hType);
    // imgHd.rsvd0 = imgHeaderInfo->pullupQspi;
    memset(&imgHd.rsvd0, 0, sizeof(imgHd.rsvd0));
    imgHd.rsvd0.byte0_pullup_qspi = imgHeaderInfo->pullupQspi & 0x1;
    if (imgHeaderInfo->addr == 0 || imgHeaderInfo->bUsb)
    {
        ;
    }
    else
    {
        if (imgHeaderInfo->dribble_dld_en == 0)
        {
            imgHd.rsvd0.byte0_dribble_dld_en = 0;
            imgHd.rsvd0.byte0_dribble_cur_valid = 1;
        }
        else
        {
            imgHd.rsvd0.byte0_dribble_dld_en = 1;
            imgHd.rsvd0.byte0_dribble_cur_valid = (imgHeaderInfo->addr & 0xffff) == 0 ? 0 : 1;
        }
    }

    sha256_calc((uint8_t*)&imgHd, sizeof(imgHd), imgHd.hashih);
    
    cmdBuf = (uint8_t*)xmalloc(FIXED_PROTOCAL_CMD_LEN + sizeof(imgHd) + CRC32_LEN);
    pCmd = (stCmd*)cmdBuf;
    pCmd->cmd = (uint8_t)CMD_DOWNLOAD_DATA;
    pCmd->index = 0;
    pCmd->order_id = DL_COMMAND_ID;
    pCmd->norder_id = DL_N_COMMAND_ID;
    pCmd->len = sizeof(imgHd);
    memcpy(pCmd->data, (uint8_t*)&imgHd, sizeof(imgHd));
    ret = package_data(cmdBuf, sizeof(imgHd), imgHeaderInfo->bDlBoot);
    xfree(cmdBuf);

    if(ret == 0)
    {
        DLOG("Image header send success");
    }
    else
    {
        DLOG("Image header send fail");
    }
    return ret;
}

int32_t send_data(uint8_t* data, uint64_t size)
{
    return write_data(data, size);
}

int32_t receive_data(uint8_t* buffer, uint64_t size)
{
    return read_data(buffer, size);
}

int32_t clear_r_buffer()
{
    return clear_read_buffer();
}

int32_t package_data(uint8_t* data, uint32_t pureDataSize, bool bDlBoot)
{
    int32_t ret;
    uint32_t remainSize;
    uint32_t tbSize;
    uint8_t *tbBuf = NULL;
    uint16_t counter;
    stCmd *pCmd = NULL;

    remainSize = pureDataSize;
    counter = 0;
    while (remainSize > 0)
    {
        if (package_data_head(remainSize, &tbSize, bDlBoot) != 0)
        {
            DLOG("package_data -> package_data_head fail");
            return -1;
        }
        if (tbSize >= pureDataSize)
        {
            ret = package_data_single(data, bDlBoot);
            break;
        }
        tbBuf = (uint8_t*)xmalloc(tbSize + FIXED_PROTOCAL_CMD_LEN + CRC32_LEN);
        pCmd = (stCmd *)data;
        pCmd->index = counter;
        pCmd->len = tbSize;
        memcpy(tbBuf, data, FIXED_PROTOCAL_CMD_LEN);
        memcpy(tbBuf+FIXED_PROTOCAL_CMD_LEN, data+FIXED_PROTOCAL_CMD_LEN+(pureDataSize-remainSize), tbSize);
        ret = package_data_single(tbBuf, bDlBoot);
        if (ret != 0)
        {
            DLOG("package_data -> package_data_single fail");
            xfree(tbBuf);
            return -1;
        }
        remainSize -= tbSize;
        counter += 1;
        xfree(tbBuf);
    }
    if (ret != 0)
    {
        DLOG("package_data fail");
        return -1;
    }
    return package_done(bDlBoot);
}

int32_t package_data_single(uint8_t* data, bool bDlBoot)
{
    int32_t ret;
    int8_t retryCount;
    uint32_t len;
    stCmd *pCmd  = NULL;
    pCmd = (stCmd*)data;
    len = pCmd->len;
    retryCount = MAX_RESEND_COUNT;
    do
    {
        pCmd->len = len;
        ret = send_recv_Cmd(data, NULL, NULL, bDlBoot);
        clear_r_buffer();
        if (ret == 0)
        {
            break;
        }
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Package_data_single %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }

    return 0;
}

int32_t package_data_head(uint32_t len, uint32_t *tbSize, bool bDlBoot)
{
    int32_t ret;
    stCmd pWCmd;
    uint32_t blockSize;
    int8_t retryCount;

    pWCmd.cmd = (uint8_t)CMD_DATA_HEAD;
    pWCmd.index = 0;
    pWCmd.order_id = DL_COMMAND_ID;
    pWCmd.norder_id = DL_N_COMMAND_ID;
    pWCmd.len = 4;
    memcpy(pWCmd.data, (uint8_t*)&len, 4);

    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = 4;
        ret = send_recv_Cmd((uint8_t*)&pWCmd, (uint8_t*)&blockSize, NULL, bDlBoot);
        clear_r_buffer();
        if (ret == 0)
        {
            g_data_block_size = blockSize;
            *tbSize = blockSize;
            break;
        }
    } while (--retryCount > 0);

    if (retryCount == 0)
    {
        DLOG("Package_data_head %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }

    return 0;
}

int32_t package_base_info(enBurnImageType type, bool bDlBoot)
{
    if (0 != package_get_version(bDlBoot))
    {
        return -1;
    }
    if (0 != package_sel_image(type, bDlBoot))
    {
        return -1;
    }
    if (0 != package_verify_image(bDlBoot))
    {
        return -1;
    }
    return 0;
}

int32_t package_get_version(bool bDlBoot)
{
    stCmd pWCmd;
    int8_t retryCount;

    pWCmd.cmd = (uint8_t)CMD_GET_VERSION;
    pWCmd.index = 0;
    pWCmd.order_id = DL_COMMAND_ID;
    pWCmd.norder_id = DL_N_COMMAND_ID;
    pWCmd.len = 0;

    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = 0;
        if (send_recv_Cmd((uint8_t*)&pWCmd, NULL, NULL, bDlBoot) == 0)
            break;
    } while (--retryCount > 0);    

    if (retryCount == 0)
    {
        DLOG("Get version %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }
    DLOG("Get version success");
        
    return 0;
}

int32_t package_sel_image(enBurnImageType type, bool bDlBoot)
{
    int32_t ret;
    stCmd pWCmd;
    uint32_t identifier;
    int8_t retryCount;

    pWCmd.cmd = (uint8_t)CMD_SEL_IMAGE;
    pWCmd.index = 0;
    pWCmd.order_id = DL_COMMAND_ID;
    pWCmd.norder_id = DL_N_COMMAND_ID;
    pWCmd.len = 0;

    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = 0;
        ret = send_recv_Cmd((uint8_t*)&pWCmd, (uint8_t*)&identifier, NULL, bDlBoot);
        clear_r_buffer();
        if ((ret == 0) && identifier == get_imageid(type))
            break;

    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Sel image %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }
    DLOG("Sel image success");

    return 0;
}

int32_t package_verify_image(bool bDlBoot)
{
    int32_t ret;
    stCmd pWCmd;
    int8_t retryCount;

    pWCmd.cmd = (uint8_t)CMD_VERIFY_IMAGE;
    pWCmd.index = 0;
    pWCmd.order_id = DL_COMMAND_ID;
    pWCmd.norder_id = DL_N_COMMAND_ID;
    pWCmd.len = 0;

    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = 0;
        ret = send_recv_Cmd((uint8_t*)&pWCmd, NULL, NULL, bDlBoot);
        clear_r_buffer();
        if (ret == 0)
            break;
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Verify image %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }
    DLOG("Verify image success");

    return 0;
}

int32_t package_done(bool bDlBoot)
{
    stCmd pWCmd;
    int8_t retryCount;

    pWCmd.cmd = (uint8_t)CMD_DONE;
    pWCmd.index = 0;
    pWCmd.order_id = DL_COMMAND_ID;
    pWCmd.norder_id = DL_N_COMMAND_ID;
    pWCmd.len = 0;

    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = 0;
        if (send_recv_Cmd((uint8_t*)&pWCmd, NULL, NULL, bDlBoot) == 0)
        {
            clear_r_buffer();
            break;
        }
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Package done %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }
    DLOG("Package done success", MAX_RESEND_COUNT);

    return 0;
}

int32_t package_lpc_burn_one(enBurnImageType type, enStorageType storType)
{
    int32_t ret;
    stLpcCmd pWCmd;
    int8_t retryCount;
    uint32_t imgId;
    uint16_t extern_data;
    uint32_t tempLen;

    imgId = get_imageid(type);
    pWCmd.cmd = (uint8_t)LPC_BURN_ONE;
    pWCmd.index = 0;
    pWCmd.order_id = LPC_COMMAND_ID;
    pWCmd.norder_id = LPC_N_COMMAND_ID;
    if (storType == STYPE_CP_FLASH)
    {
        pWCmd.len = 6;
        tempLen = 6;
        memcpy(pWCmd.data, (uint8_t*)&imgId, sizeof(imgId));
        extern_data = CP_FLASH_IND;
        memcpy(pWCmd.data+sizeof(imgId), &extern_data, sizeof(extern_data));
    }
    else
    {
        pWCmd.len = 4;
        tempLen= 4;
        memcpy(pWCmd.data, (uint8_t*)&imgId, sizeof(imgId));
    }
    
    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = tempLen;
        ret = send_recv_lpcCmd((uint8_t*)&pWCmd, NULL, NULL);
        clear_r_buffer();
        if (ret == 0)
            break;
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Lpc burn one %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }

    DLOG("Lpc burn one success");
    return 0;
}

int32_t package_lpc_erase(enEraseType type, uint32_t addr, uint32_t len)
{
    stLpcCmd pWCmd;
    int8_t retryCount;

    pWCmd.cmd = (uint8_t)LPC_FLASH_ERASE;
    pWCmd.index = 0;
    pWCmd.order_id = LPC_COMMAND_ID;
    pWCmd.norder_id = LPC_N_COMMAND_ID;
    pWCmd.len = 8;
    
    memcpy(pWCmd.data, &len, 4);
    memcpy(pWCmd.data + 4, &addr, 4);

    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = 8;
        if (send_recv_lpcCmd((uint8_t*)&pWCmd, NULL, NULL) == 0)
        {
            clear_r_buffer();
            break;
        }
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Lpc erase %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }

    return 0;
}

int32_t package_lpc_get_burn_status()
{
    int32_t ret;
    stLpcCmd pWCmd;
    int8_t retryCount;
    uint32_t status = 0xFFFF;

    pWCmd.cmd = (uint8_t)LPC_GET_BURN_STATUS;
    pWCmd.index = 0;
    pWCmd.order_id = LPC_COMMAND_ID;
    pWCmd.norder_id = LPC_N_COMMAND_ID;
    pWCmd.len = 0;
    
    retryCount = MAX_RESEND_COUNT;
    do
    {
        ret = send_recv_lpcCmd((uint8_t*)&pWCmd, (uint8_t*)&status, NULL);
        clear_r_buffer();
        if (ret == 0)
            break;
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Lpc get burn status %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }

    if (status == 0)
        return 0;
    else
        return -1;
}

int32_t package_lpc_sys_reset()
{
    int32_t ret;
    stLpcCmd pWCmd;
    int8_t retryCount;
    uint8_t recvBuf[10];

    pWCmd.cmd = (uint8_t)LPC_SYS_RST;
    pWCmd.index = 0;
    pWCmd.order_id = LPC_COMMAND_ID;
    pWCmd.norder_id = LPC_N_COMMAND_ID;
    pWCmd.len = 0;
    
    retryCount = MAX_RESEND_COUNT;
    do
    {
        memset(recvBuf, 0, sizeof(recvBuf));
        ret = send_recv_lpcCmd((uint8_t*)&pWCmd, recvBuf, NULL);
        clear_r_buffer();
        if (ret == 0)
            break;
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Lpc get burn status %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }

    if (strcmp((const char*)recvBuf, "ZzZzZzZz") == 0)
        return 0;
    else
        return -1;
}

int32_t package_lpc_readmem(uint32_t addr, uint32_t len, uint32_t *readLen, uint8_t *buf)
{
    stLpcCmd pWCmd;
    int8_t retryCount;
    uint8_t rLen;

    pWCmd.cmd = (uint8_t)LPC_READ_MEM;
    pWCmd.index = 0;
    pWCmd.order_id = LPC_COMMAND_ID;
    pWCmd.norder_id = LPC_N_COMMAND_ID;
    pWCmd.len = 8;
    
    memcpy(pWCmd.data, &len, 4);
    memcpy(pWCmd.data + 4, &addr, 4);

    retryCount = MAX_RESEND_COUNT;
    do
    {
        pWCmd.len = 8;
        if ((send_recv_lpcCmd((uint8_t*)&pWCmd, buf, &rLen) == 0))
        {
            clear_r_buffer();
            *readLen = rLen;
            break;
        }
    } while (--retryCount > 0);
    
    if (retryCount == 0)
    {
        DLOG("Lpc readmem %u times and fail", MAX_RESEND_COUNT);
        return -1;
    }

    return 0;
}

uint32_t get_imageid(enBurnImageType type)
{
    switch(type)
    {
        case BTYPE_BOOTLOADER:
        case BTYPE_AGBOOT:
            return AGBT_IDENTIFIER;
        case BTYPE_AP:
            return AIMG_IDENTIFIER;
        case BTYPE_CP:
            return CIMG_IDENTIFIER;
        case BTYPE_FLEXFILE:
        case BTYPE_OTHER1:
        case BTYPE_OTHER2:
        case BTYPE_OTHER3:
        case BTYPE_OTHER4:
        case BTYPE_OTHER5:
            return FLEX_IDENTIFIER;
        case BTYPE_HEAD:
        case BTYTE_DLBOOT:
            return IMGH_IDENTIFIER;
        default:
            return 0xFFFFFFFF;
    }
}

uint8_t get_hashtype(enHashType type)
{
    switch(type)
    {
        case HTYPE_NOHASH:
            return 0xee;
        case HTYPE_SWHASH:
            return 0xaa;
        case HTYPE_HWHASH:
            return 0xdd;
        default:
            return 0xee;
    }
}

int32_t send_recv_Cmd(uint8_t *s_data, uint8_t *r_data, uint8_t *r_len, bool bDlBoot)
{
    int32_t ret;
    uint8_t readBuf[40];
    uint32_t ckVal;
    uint32_t wLen;
    uint32_t tempLen;
    uint8_t  currBufLen;
    stRsp *pRsp = NULL;
    stCmd *pCmd  = NULL;

    memset(readBuf, 0, sizeof(readBuf));
    pCmd = (stCmd*)s_data;
    wLen = FIXED_PROTOCAL_CMD_LEN + pCmd->len;
    if (!bDlBoot)
    {
        ckVal = crc32(s_data, wLen);
        memcpy(s_data+wLen, (uint8_t*)&ckVal, 4);
        // length check value
        if (pCmd->len > 0)
        {
            tempLen = (pCmd->len & 0xffffff);
            pCmd->len = (crc8((uint8_t*)&tempLen, 3)<<24) + (pCmd->len & 0xffffff);

        }
        wLen += 4;
    }
    else if (pCmd->cmd == (uint8_t)CMD_DOWNLOAD_DATA)
    {
        ckVal = self_def_check1(s_data);
        memcpy(s_data+wLen, (uint8_t*)&ckVal, 4);
        wLen += 4;
    }   
    else;
    ret = send_data(s_data, wLen);
    if (ret == 0)
    {
        currBufLen = 0;
        ret = receive_data(readBuf, FIXED_PROTOCAL_RSP_LEN);
        currBufLen += FIXED_PROTOCAL_RSP_LEN;
        pRsp = (stRsp*)readBuf;
        if ((ret != 0) || !check_cmd_bwt_send_recv(s_data, readBuf) || (pRsp->state != RSP_STATE_AK))
        {
            DLOG("Protocal rsp head fail");
            return -1;
        }
        
        if (pRsp->len > 0)
        {
            ret = receive_data(readBuf+currBufLen, pRsp->len);
            if ((ret != 0))
            {
                DLOG("Receive data fail");
                return -1;
            }
            if (r_data != NULL)
                memcpy(r_data, readBuf+currBufLen, pRsp->len);
            if (r_len != NULL)
                *r_len = pRsp->len;

            currBufLen += pRsp->len;
        }
        if (!bDlBoot)
        {
            ret = receive_data(readBuf+currBufLen, CRC32_LEN);
            if((ret != 0))
            {
                DLOG("Receive crc32 data fail");
                return -1;
            }
            ckVal = crc32(readBuf, FIXED_PROTOCAL_RSP_LEN + pRsp->len);
            //if (ckVal != *(uint32_t*)(readBuf+currBufLen))
            if (0 != memcmp((uint8_t*)&ckVal, (readBuf+currBufLen), 4))
            {
                DLOG("Crc32 check fail, recv crc = %u, calc crc = %u", *(uint32_t*)(readBuf+currBufLen), ckVal);
                return -1;
            }
        }
    }

    return ret;    
}

int32_t send_recv_lpcCmd(uint8_t *s_data, uint8_t *r_data, uint8_t *r_len)
{
    int32_t ret;
    uint8_t readBuf[MAX_READ_MEM_SIZE * 2 + 6];
    uint32_t ckVal;
    uint32_t wLen;
    uint32_t tempLen;
    uint8_t  currBufLen;
    stLpcCmd *pCmd = NULL;
    stLpcRsp *pRsp = NULL;

    memset(readBuf, 0, sizeof(readBuf));
    pCmd = (stLpcCmd*)s_data;
    wLen = FIXED_LPC_CMD_LEN + pCmd->len;

    if (1)
    {
        ckVal = crc32(s_data, wLen);
        memcpy(s_data+wLen, (uint8_t*)&ckVal, 4);
        // length check value
        if (pCmd->len > 0)
        {
            tempLen = (pCmd->len & 0xffffff);
            pCmd->len = (crc8((uint8_t*)&tempLen, 3)<<24) + (pCmd->len & 0xffffff);

        }
        wLen += 4;
    }
    ret = send_data(s_data, wLen);
    if (ret == 0)
    {
        currBufLen = 0;
        ret = receive_data(readBuf, FIXED_LPC_RSP_LEN);
        currBufLen += FIXED_LPC_RSP_LEN;
        pRsp = (stLpcRsp*)readBuf;
        if ((ret != 0) || !check_cmd_bwt_send_recv(s_data, readBuf) || (pRsp->state != RSP_STATE_AK))
        {
            DLOG("Protocal rsp head fail");
            return -1;
        }
        
        if (pRsp->len > 0)
        {
            ret = receive_data(readBuf+currBufLen, pRsp->len);
            if ((ret != 0))
            {
                DLOG("Receive data fail");
                return -1;
            }
            if (r_data != NULL)
                memcpy(r_data, readBuf+currBufLen, pRsp->len);
            if (r_len != NULL)
                *r_len = pRsp->len;

            currBufLen += pRsp->len;
        }
        if (1)
        {
            ret = receive_data(readBuf+currBufLen, CRC32_LEN);
            if((ret != 0))
            {
                DLOG("Receive crc32 data fail");
                return -1;
            }
            ckVal = crc32(readBuf, FIXED_LPC_RSP_LEN + pRsp->len);
            //if (ckVal != *(uint32_t*)(readBuf+currBufLen))
            if (0 != memcmp((uint8_t*)&ckVal, (readBuf+currBufLen), 4))
            {
                DLOG("Crc32 check fail, recv crc = %u, calc crc = %u", *(uint32_t*)(readBuf+currBufLen), ckVal);
                return -1;
            }
        }
    }

    return ret;   
}

void init_image_header(stImgHead *p)
{
    if (p == NULL)
        return;
    memset(p, 0, sizeof(stImgHead));
    p->verinfo.vVal = 0x10000001;
    p->verinfo.id = IMGH_IDENTIFIER;
    p->verinfo.dtm = 0x20180507;
    p->imgnum = 1;
    p->ctlinfo.hashtype = 0xee;
    p->imgbody.id = AGBT_IDENTIFIER;
    p->imgbody.ldloc = 0x04010000;
}

uint32_t self_def_check1(uint8_t *in)
{
    uint32_t ckVal;
    uint32_t i;
    stCmd *p = (stCmd *)in;

    ckVal = p->cmd + p->index + p->order_id + p->norder_id + (p->len&0xff) + ((p->len>>8)&0xff) + ((p->len>>16)&0xff) + ((p->len>>24)&0xff);
    for (i=0; i<(p->len); i++)
    {
        ckVal += (*((uint8_t*)p+FIXED_PROTOCAL_CMD_LEN+i));
    }
    ckVal &= 0xffffffff;

    return ckVal;
}

bool check_cmd_bwt_send_recv(uint8_t *send, uint8_t* recv)
{
    stCmd *pCmd = NULL;
    stRsp *pRsp = NULL;

    if ((send == NULL) || (recv == NULL))
        return false;
    pCmd = (stCmd *)send;
    pRsp = (stRsp *)recv;
    if (pCmd->cmd == CMD_DOWNLOAD_DATA)
    {
        if ((pCmd->cmd == pRsp->cmd) && ((pCmd->index+1) == pRsp->index) && (pCmd->order_id == pRsp->order_id) && (pCmd->norder_id == pRsp->norder_id))
            return true;
    }
    else
    {
        if ((pCmd->cmd == pRsp->cmd) && (pCmd->index == pRsp->index) && (pCmd->order_id == pRsp->order_id) && (pCmd->norder_id == pRsp->norder_id))
            return true;
    }
    return false;
}

int32_t get_img_sh256_value(char *path, uint8_t *hash)
{
    FILE *fp;
    uint32_t size;
    uint8_t *buf;

    size = get_file_size(path);
    fp = fopen(path, "rb");
    if (fp == NULL)
        return -1;
    buf = (uint8_t*)xmalloc(size);
    if (fread(buf, 1, size, fp) != size)
    {
        fclose(fp);
        xfree(buf);
        return -1;
    }
    sha256_calc(buf, size, hash);
    fclose(fp);
    xfree(buf);

    return 0;
}
