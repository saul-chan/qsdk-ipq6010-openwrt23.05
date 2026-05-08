#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "action.h"
#include "package.h"
#include "ini.h"
#include "cJSON.h"
#include "global_def.h"

int32_t action_burn(stPrimCliCfg* priCfg)
{
    int find_xpk_baseini=1;
    stParseBurnCliCfg parCfg;
    stIniCfg iniCfg;
    stDldUpg dldUpg;
    stAgDlInfo agInfo;
    stUsbBootCtrl uBCtrl;
    // default
    iniCfg.agBaud = 921600;
    iniCfg.dlBaud = 115200;
    iniCfg.detect = 0;
    iniCfg.atBaud = 115200;
    iniCfg.uart_timeout = DEFAULT_UART_TIMEOUT_MS;
    iniCfg.debug = 0;
    iniCfg.skip_addr_align = 0;
    ILOG("Burn action start, type = %s", priCfg->burnFileType);
    memset(&parCfg, 0, sizeof(parCfg));
    // check
    if (!check_burn_params(priCfg, &parCfg))
    {
        return -1;
    }
    // parse ini file
    memset(&iniCfg, 0, sizeof(iniCfg));
    memset(&dldUpg, 0, sizeof(stDldUpg));
    if (ini_parse((const char*)parCfg.cfgFile, parse_ini_handler, &iniCfg) < 0)
    {
        ELOG("Ini file parse fail, path = %s", parCfg.cfgFile);
        return -1;
    }
    set_debug(iniCfg.debug);

    if (ini_parse(XPX_USB_BASEINI_PATH, parse_dldupg_handler, &dldUpg) < 0)
    {
        find_xpk_baseini = 0;
        ILOG("don't find config_xpk_usb_baseini file\r\n");
    }
    ILOG("config_xpk_usb_baseini find is %d, valid %d, connwait %d, ctrlwait %d\r\n", find_xpk_baseini, dldUpg.dld_upg_ctrl_valid, \
        dldUpg.dld_upg_connwait_100ms_cnt, dldUpg.dld_upg_ctrlwait_100ms_cnt);

    // burn agent
    //if (parCfg.skipAgDl == 0)
    if(0)
    {
        // init port
        if (-1 == init_port(parCfg.port, iniCfg.dlBaud, iniCfg.uart_timeout))
        {
            ELOG("Open com port fail, port = %s", parCfg.port);
            free_port();
            return -1;
        }
        ILOG("Open com port success, port = %s", parCfg.port);

        if (iniCfg.detect == 1)
        {
            // init_port(parCfg.port, iniCfg.atBaud, iniCfg.uart_timeout);
            reset_dut(iniCfg.rstAtCmd);
            // free_port();
        }

        // dl sync
        if (-1 == dl_boot_sync())
        {
            ELOG("DL boot sync fail");
            return -1;
        }
        ILOG("DL boot sync success");
        strcpy(agInfo.path, iniCfg.agPath);
        agInfo.baud = iniCfg.agBaud;
        agInfo.type = HTYPE_NOHASH;
        agInfo.pullupQspi = iniCfg.pullupQspi;
        agInfo.ctrlMaigc = iniCfg.agCtrlMagic;
        agInfo.apSize = iniCfg.apSize;
        agInfo.cpSize = iniCfg.cpSIze;
        if (-1 == download_agent(&agInfo))
        {
            ELOG("Download agentboot fail");
            return -1;
        }
        ILOG("Download agentboot success");
        free_port();
    }
    else{
        ILOG("Skip agentboot download");
    }
    if (-1 == init_port(parCfg.port, iniCfg.agBaud, iniCfg.uart_timeout))
    {
        ELOG("Open com port fail, port = %s", parCfg.port);
        free_port();
        return -1;
    }
    ILOG("Open com port success, port = %s", parCfg.port);
    // burn image(bl,ap,cp,ff)
    if(find_xpk_baseini == 0)
    {
        uBCtrl.dld_upg_ctrl_valid = iniCfg.dld_upg_ctrl_valid;
        uBCtrl.dld_upg_connwait_100ms_cnt = iniCfg.dld_upg_connwait_100ms_cnt;
        uBCtrl.dld_upg_ctrlwait_100ms_cnt = iniCfg.dld_upg_ctrlwait_100ms_cnt;
    }
    else
    {
        uBCtrl.dld_upg_ctrl_valid = dldUpg.dld_upg_ctrl_valid;
        uBCtrl.dld_upg_connwait_100ms_cnt = dldUpg.dld_upg_connwait_100ms_cnt;
        uBCtrl.dld_upg_ctrlwait_100ms_cnt = dldUpg.dld_upg_ctrlwait_100ms_cnt;
    }

    if (-1 == download_image(parCfg.bFileType, get_img_path(parCfg.bFileType, &iniCfg), get_img_addr(parCfg.bFileType, &iniCfg), (enStorageType)get_storage_type(parCfg.bFileType, &iniCfg), iniCfg.dribble_dld_en, iniCfg.usb_enable == 1 ? true : false, iniCfg.skip_addr_align == 1 ? true : false, &uBCtrl))
    {
        ELOG("Download image fail");
        return -1;
    }
    ILOG("Burn action success");
    if (parCfg.reset == 1)
    {
        if (-1 == sys_reset())
        {
            ELOG("System reset fail");
            return -1;
        }
        ILOG("System reset success");
    }
    free_port();
    
    return 0;
}

int32_t action_burn_list(stPrimCliCfg* priCfg)
{
    int32_t ret;
    char file_list[10][10];
    uint8_t i,size;
    char pri_reset;

    pri_reset = priCfg->reset;
    memset(file_list, 0, sizeof(file_list));
    ILOG("Burnlist = %s", priCfg->burnFileList);
    if (0 != parse_burnlist_params(priCfg->burnFileList, file_list, &size))
        return -1;
    ILOG("Burn %u files", size);
    for (i=0; i<size; i++)
    {
        memcpy(priCfg->burnFileType, &file_list[i][0], sizeof(priCfg->burnFileType));
        if (i == (size - 1))
            priCfg->reset = pri_reset;
        else
            priCfg->reset = 0;
        ret = action_burn(priCfg);
        if (ret != 0)
        {
            ELOG("Burnlist fail");
            return -1;
        }
        priCfg->skipAgDl = 1;
    }
    ILOG("Burnlist success");
    return 0;
}

int32_t action_erase(stPrimCliCfg* priCfg)
{
    stParseEraseCliCfg parCfg;
    stIniCfg iniCfg;
    // default
    iniCfg.agBaud = 921600;
    iniCfg.dlBaud = 115200;
    iniCfg.detect = 0;
    iniCfg.atBaud = 115200;
    iniCfg.uart_timeout = DEFAULT_UART_TIMEOUT_MS;
    iniCfg.debug = 0;
    ILOG("Erase action start");
    memset(&parCfg, 0, sizeof(parCfg));
    // check
    if (!check_erase_params(priCfg, &parCfg))
    {
        ELOG("Check erase parameters fail");
        return -1;
    }
    // parse ini file
    memset(&iniCfg, 0, sizeof(iniCfg));
    if (ini_parse((const char*)parCfg.cfgFile, parse_ini_handler, &iniCfg) < 0)
    {
        ELOG("Ini file parse fail");
        return -1;
    }
    set_debug(iniCfg.debug);
    // burn agent
    if (parCfg.skipAgDl == 0)
    {
        if (iniCfg.detect == 1)
        {
            init_port(parCfg.port, iniCfg.atBaud, iniCfg.uart_timeout);
            reset_dut(iniCfg.rstAtCmd);
            free_port();
        }
        // init port
        if (-1 == init_port(parCfg.port, iniCfg.dlBaud, iniCfg.uart_timeout))
        {
            ELOG("Open com port fail, port = %s", parCfg.port);
            free_port();
            return -1;
        }
        ILOG("Open com port success, port = %s", parCfg.port);
        // dl sync
        if (-1 == dl_boot_sync())
        {
            ELOG("DL boot sync fail");
            return -1;
        }
        ILOG("DL boot sync success");
        stAgDlInfo agInfo;
        strcpy(agInfo.path, iniCfg.agPath);
        agInfo.baud = iniCfg.agBaud;
        agInfo.type = HTYPE_NOHASH;
        agInfo.pullupQspi = iniCfg.pullupQspi;
        agInfo.ctrlMaigc = iniCfg.agCtrlMagic;
        agInfo.apSize = iniCfg.apSize;
        agInfo.cpSize = iniCfg.cpSIze;
        if (-1 == download_agent(&agInfo))
        {
            ELOG("Download agentboot fail");
            return -1;
        }
        ILOG("Download agentboot success");
        free_port();
    }
    else{
        ILOG("Skip agentboot download");
    }
    if (-1 == init_port(parCfg.port, iniCfg.agBaud, iniCfg.uart_timeout))
    {
        ELOG("Open com port fail, port = %s", parCfg.port);
        free_port();
        return -1;
    }
    ILOG("Open com port success, port = %s", parCfg.port);
    if (-1 == erase_flash((enEraseType)parCfg.eraseType, parCfg.eraseStartAddr, parCfg.eraseLen))
    {
        ELOG("Erase fail");
        return -1;
    }
    free_port();
    ILOG("Erase action success");
    return 0;
}

int32_t action_erase_list(stPrimCliCfg* priCfg)
{
    int32_t ret;
    //uint8_t i,size;
    char all = 0, nvm = 0, cal =0;
    stIniCfg iniCfg;
    // default
    iniCfg.agBaud = 921600;
    iniCfg.dlBaud = 115200;
    iniCfg.detect = 0;
    iniCfg.atBaud = 115200;
    iniCfg.uart_timeout = DEFAULT_UART_TIMEOUT_MS;
    iniCfg.debug = 0;

    ILOG("Eraselist = %s", priCfg->eraseList);
    if (0 != parse_eraselist_params(priCfg->eraseList, &all, &nvm, &cal))
        return -1;
    // parse ini file
    memset(&iniCfg, 0, sizeof(iniCfg));
    if (ini_parse((const char*)priCfg->cfgFile, parse_ini_handler, &iniCfg) < 0)
    {
        ELOG("Ini file parse fail");
        return -1;
    }
    ret = erase_list(priCfg->port, priCfg->cfgFile, iniCfg.formatPath, all, nvm, cal);
    if (ret == 0)
    {
        ILOG("Eraselist success");
    }
    else{
        ELOG("Eraselist fail");
    }
    return ret;
}

int32_t action_read_memory(stPrimCliCfg* priCfg)
{
    stParseReadMemClifCfg parCfg;
    stIniCfg iniCfg;
    // default
    iniCfg.agBaud = 921600;
    iniCfg.dlBaud = 115200;
    iniCfg.detect = 0;
    iniCfg.atBaud = 115200;
    iniCfg.uart_timeout = DEFAULT_UART_TIMEOUT_MS;
    iniCfg.debug = 0;
    ILOG("ReadMem action start");
    memset(&parCfg, 0, sizeof(parCfg));
    // check
    if (!check_readmem_params(priCfg, &parCfg))
    {
        ELOG("Check readmem parameters fail");
        return -1;
    }
    // parse ini file
    memset(&iniCfg, 0, sizeof(iniCfg));
    if (ini_parse((const char*)parCfg.cfgFile, parse_ini_handler, &iniCfg) < 0)
    {
        ELOG("Ini file parse fail");
        return -1;
    }
    set_debug(iniCfg.debug);
    // burn agent
    if (parCfg.skipAgDl == 0)
    {
        if (iniCfg.detect == 1)
        {
            init_port(parCfg.port, iniCfg.atBaud, iniCfg.uart_timeout);
            reset_dut(iniCfg.rstAtCmd);
            free_port();
        }
        // init port
        if (-1 == init_port(parCfg.port, iniCfg.dlBaud, iniCfg.uart_timeout))
        {
            ELOG("Open com port fail, port = %s", parCfg.port);
            free_port();
            return -1;
        }
        ILOG("Open com port success, port = %s", parCfg.port);
        // dl sync
        if (-1 == dl_boot_sync())
        {
            ELOG("DL boot sync fail");
            return -1;
        }
        ILOG("DL boot sync success");
        stAgDlInfo agInfo;
        strcpy(agInfo.path, iniCfg.agPath);
        agInfo.baud = iniCfg.agBaud;
        agInfo.type = HTYPE_NOHASH;
        agInfo.pullupQspi = iniCfg.pullupQspi;
        agInfo.ctrlMaigc = iniCfg.agCtrlMagic;
        agInfo.apSize = iniCfg.apSize;
        agInfo.cpSize = iniCfg.cpSIze;
        if (-1 == download_agent(&agInfo))
        {
            ELOG("Download agentboot fail");
            return -1;
        }
        ILOG("Download agentboot success");
        free_port();
    }
    else{
        ILOG("Skip agentboot download");
    }
    if (-1 == init_port(parCfg.port, iniCfg.agBaud, iniCfg.uart_timeout))
    {
        ELOG("Open com port fail, port = %s", parCfg.port);
        free_port();
        return -1;
    }
    ILOG("Open com port success, port = %s", parCfg.port);
    if (-1 == read_mem(parCfg.readStartAddr, parCfg.readLen, parCfg.filePath))
    {
        ELOG("ReadMem fail");
        return -1;
    }
    free_port();
    ILOG("ReadMem action success");
    return 0;
}

int32_t action_split_pkg(stPrimCliCfg* priCfg)
{
    uint8_t *pBuf = NULL;
    FILE *pkgF;
    //FILE *binF
    uint32_t len, imgLen, i;
    stFieldInfoExt *pFInfo;
    char product_name[16]={0};
    stIniCfg iniCfg;
    uint32_t imagenumber;
    uint16_t vt;
    uint16_t vtsize;
    VARIANTARGEXT* pVar = NULL;
    uint8_t *fieldInfoExtBuf = NULL;
    char imagePath[256];
    char *findPos = NULL;
    // default
    iniCfg.agBaud = 921600;
    iniCfg.dlBaud = 115200;
    iniCfg.detect = 0;
    iniCfg.atBaud = 115200;
    iniCfg.uart_timeout = DEFAULT_UART_TIMEOUT_MS;
    iniCfg.debug = 0;

    // parse ini file
    ILOG("Split pkg file action start");
    memset(&iniCfg, 0, sizeof(iniCfg));
    if (ini_parse((const char*)priCfg->cfgFile, parse_ini_handler, &iniCfg) < 0)
    {
        ELOG("Ini file parse fail");
        return -1;
    }
    pkgF = fopen(iniCfg.pkgPath, "rb");
    if (pkgF == NULL)
    {
        // fclose(pkgF);
        ELOG("Open pkg file fail,path = %s", iniCfg.pkgPath);
        return -1;
    }
    // pkg file info
    len = sizeof(stFileHeaderExt);
    pBuf = (uint8_t*)xmalloc(len);
    if (fread(pBuf, 1, len, pkgF) != len)
    {
        fclose(pkgF);
        xfree(pBuf);
        return -1;
    }
    stFileHeaderExt* pHeader = (stFileHeaderExt*)pBuf;
    imagenumber = pHeader->imagenumber;
    vt = pHeader->vt;
    vtsize = pHeader->vtsize;
    xfree(pBuf);

    if (vt > 0 && vtsize > 0)
    {
        pVar = (VARIANTARGEXT*)xmalloc(vtsize);
        if (fread(pVar, 1, vtsize, pkgF) != vtsize)
        {
            fclose(pkgF);
            xfree(pVar);
            return -1;
        }
        #if 0
        for (i=0; i<pHeader->vt; i++, pVar++) // pVar++ will cause free pVar issue
        {
            if (pVar->vt == VTE_ATTRVAL)
            {
                //argItem[it.attrval.name] = it.attrval.val;
            }
            else if (pVar->vt == VTE_ATTRSTR)
            {
                //argItem[it.attrstr.name] = it.attrstr.str;
            }
            else if (pVar->vt == VTE_MACROTUPLEVAL)
            {
                //argItem[it.tpval.val1.name] = it.tpval.val1.val;
                //argItem[it.tpval.val2.name] = it.tpval.val2.val;
            }
            else if (pVar->vt == VTE_OLDTOOLTAG) //print hardcoding string
            {
                if (pVar->attrval.val == 1)
                {
                    //item["file"] = "ban_old_tool_poison.bin";
                    //item["type"] = "VIRTUALIMG";
                    //item["hash"] = "0";
                    //item["addr"] = 0;
                    //item["flashsize"] = 0;
                    //root["imageinfo"].append(item);
                }
            }
        }
        #endif
        xfree(pVar);
    }

    for(i=0; i<imagenumber; i++)
    {
        len = sizeof(stFieldInfoExt);
        pBuf = (uint8_t*)xmalloc(len);
        if (fread(pBuf, 1, len, pkgF) != len)
        {
            fclose(pkgF);
            xfree(pBuf);
            return -1;
        }
        pFInfo = (stFieldInfoExt*)(pBuf);
        ILOG("image%d = %s, addr = 0x%x, size = %u, offset = %u, type = %s", i, pFInfo->name, pFInfo->addr, pFInfo->flashsize, pFInfo->offset, pFInfo->type);
        imgLen = pFInfo->size;
        if (pFInfo->vt >0 && pFInfo->vtsize > 0)
        {
            // rsvd
            fieldInfoExtBuf = (uint8_t*)xmalloc(pFInfo->vtsize);
            if (fread(fieldInfoExtBuf, 1, pFInfo->vtsize, pkgF) != pFInfo->vtsize)
            {
                fclose(pkgF);
                xfree(pBuf);
                xfree(fieldInfoExtBuf);
                return -1;
            }
        }
        memset(imagePath, 0, sizeof(imagePath));
        findPos = strrchr(iniCfg.pkgPath, '/');
        if (findPos == NULL)
        {
            sprintf(imagePath, "%s.bin", pFInfo->name);
        }
        else
        {
            strncpy(imagePath, iniCfg.pkgPath, (findPos - iniCfg.pkgPath + 1));
            strncat(imagePath, pFInfo->name, strlen((const char*)pFInfo->name));
            strcat(imagePath, ".bin");
        }
        ILOG("Generate  %s file path = %s", pFInfo->name, imagePath);
        if (0 != generate_bin(pkgF, imgLen, imagePath))
        {
            fclose(pkgF);
            xfree(pBuf);
            ELOG("Generate %s file fail", pFInfo->name);
            return -1;
        }
        xfree(pBuf);
    }
    fclose(pkgF);

    pkgF = fopen(iniCfg.pkgPath, "rb");
    fseek(pkgF, PRODUCT_NAME_LOCATION, SEEK_SET);
    fread(product_name, sizeof(char), 16, pkgF);
    fclose(pkgF);

    if(strncmp(product_name, EC718S, 6) == 0)
    {
        snprintf(priCfg->cfgFile, sizeof(priCfg->cfgFile), "%s%s", PATH_PREFIX, INI_718S_PATH);
        ILOG("cfg is cfg_ec718s_usb.ini\r\n");
    }
    else if(strncmp(product_name, EC718P, 6) == 0)
    {
        snprintf(priCfg->cfgFile, sizeof(priCfg->cfgFile), "%s%s", PATH_PREFIX, INI_718P_PATH);
        ILOG("cfg is cfg_ec718p_usb.ini\r\n");
    }
    else if(strncmp(product_name, EC716S, 6) == 0)
    {
        snprintf(priCfg->cfgFile, sizeof(priCfg->cfgFile), "%s%s", PATH_PREFIX, INI_716S_PATH);
        ILOG("cfg is cfg_ec716s_usb.ini\r\n");
    }
    else if(strncmp(product_name, EC716E, 6) == 0)
    {
        snprintf(priCfg->cfgFile, sizeof(priCfg->cfgFile), "%s%s", PATH_PREFIX, INI_716E_PATH);
        ILOG("cfg is cfg_ec716e_usb.ini\r\n");
    }
    else
    {
        ELOG("cfg is error, product_name is %s\r\n", product_name);
        return -1;
    }

    return 0;
#if 0
    // BL
    len = sizeof(stFileHeaderExt)+sizeof(stFieldInfoExt);
    pBuf = (uint8_t*)xmalloc(len);
    if (fread(pBuf, 1, len, pkgF) != len)
    {
        fclose(pkgF);
        xfree(pBuf);
        return -1;
    }
    pFInfo = (stFieldInfoExt*)(pBuf + sizeof(stFileHeaderExt));
    imgLen = pFInfo->size;
    if (0 != generate_bin(pkgF, imgLen, iniCfg.blPath))
    {
        fclose(pkgF);
        xfree(pBuf);
        ELOG("Generate Bootload file fail");
        return -1;
    }
    ILOG("Generate Bootload file success");
    // AP
    len = sizeof(stFieldInfoExt);
    if (fread(pBuf, 1, len, pkgF) != len)
    {
        fclose(pkgF);
        xfree(pBuf);
        return -1;
    }
    pFInfo = (stFieldInfoExt*)(pBuf);
    imgLen = pFInfo->size;
    if (0 != generate_bin(pkgF, imgLen, iniCfg.sysPath))
    {
        fclose(pkgF);
        xfree(pBuf);
        ELOG("Generate system file fail");
        return -1;
    } 
    ILOG("Generate system file success");
    // CP
    if (fread(pBuf, 1, len, pkgF) != len)
    {
        fclose(pkgF);
        xfree(pBuf);
        return -1;
    }
    pFInfo = (stFieldInfoExt*)(pBuf);
    imgLen = pFInfo->size;
    if (0 != generate_bin(pkgF, imgLen, iniCfg.cpSysPath))
    {
        fclose(pkgF);
        xfree(pBuf);
        ELOG("Generate cp_system file fail");
        return -1;
    }    
    ILOG("Generate cp_system file success");
    ILOG("Split pkg file action success");
    return 0;
#endif
}

// check whether file could access
bool check_file(const char *path)
{
    FILE *f;
    if ((f = fopen(path, "r")) != NULL)
    {
        fclose(f);
        return true;
    }
    else
    {
        ELOG("File could not access or not exist, path = %s", path);
        return false;
    }
}

// check port string format
bool chekc_port(char *port)
{
    uppercase(port);
    if (strstr((const char*)port, "DEV/TTY") == NULL)
    {
        ELOG("invalid port: %s", port);
        return false;
    }
    return true;
}

bool check_burn_params(stPrimCliCfg* priCfg, stParseBurnCliCfg* parCfg)
{
    stPrimCliCfg cliCfg;
    int32_t type_len, i;
    memcpy(&cliCfg, priCfg, sizeof(cliCfg));
    if (!check_file(cliCfg.cfgFile))
    {
        return false;
    }
    else
    {
        memcpy(parCfg->cfgFile, priCfg->cfgFile, sizeof(priCfg->cfgFile));
    }

    if (!chekc_port(cliCfg.port))
    {
        return false;
    }
    else
    {
        memcpy(parCfg->port, priCfg->port, sizeof(priCfg->port));
    }
    parCfg->skipAgDl = priCfg->skipAgDl;
    parCfg->reset = priCfg->reset;

    type_len = sizeof(gImage_type_info)/sizeof(gImage_type_info[0]);
    for (i=0; i<type_len; i++)
    {
        uppercase(cliCfg.burnFileType);
        if (strcmp((const char*)cliCfg.burnFileType, gImage_type_info[i].typeName) == 0)
        {
            parCfg->bFileType = gImage_type_info[i].type;
            break;
        }
    }
    if (type_len == i)
    {
        ELOG("burn file type is invalid, type = %s", cliCfg.burnFileType);
        return false;
    }

    return true;
}

bool check_erase_params(stPrimCliCfg* priCfg, stParseEraseCliCfg* parCfg)
{
    stPrimCliCfg cliCfg;
    memcpy(&cliCfg, priCfg, sizeof(cliCfg));
    if (!check_file(cliCfg.cfgFile))
    {
        return false;
    }
    else
    {
        memcpy(parCfg->cfgFile, priCfg->cfgFile, sizeof(priCfg->cfgFile));
    }

    if (!chekc_port(cliCfg.port))
    {
        return false;
    }
    else
    {
        memcpy(parCfg->port, priCfg->port, sizeof(priCfg->port));
    }
    parCfg->skipAgDl = priCfg->skipAgDl;

    if (0 != parse_erase_params(priCfg->eraseSection, &parCfg->eraseStartAddr, &parCfg->eraseLen, &parCfg->eraseType))
    {
        return false;
    }

    return true;
}

bool check_readmem_params(stPrimCliCfg* priCfg, stParseReadMemClifCfg* parCfg)
{
    stPrimCliCfg cliCfg;

    memcpy(&cliCfg, priCfg, sizeof(cliCfg));
    if (!check_file(cliCfg.cfgFile))
    {
        return false;
    }
    else
    {
        memcpy(parCfg->cfgFile, priCfg->cfgFile, sizeof(priCfg->cfgFile));
    }

    if (!chekc_port(cliCfg.port))
    {
        return false;
    }
    else
    {
        memcpy(parCfg->port, priCfg->port, sizeof(priCfg->port));
    }
    parCfg->skipAgDl = priCfg->skipAgDl;

    if (0 != parse_readmem_params(priCfg->readSection, &parCfg->readStartAddr, &parCfg->readLen, parCfg->filePath))
    {
        return false;
    }

    return true;
}

int32_t reset_dut(char *cmd)
{
    return at_command(cmd);
}

int32_t dl_boot_sync()
{
    return burn_sync(SYNC_HANDSHAKE_DLBOOT,2);
}

int32_t agent_boot_sync()
{
    return burn_sync(SYNC_HANDSHAKE_AGBOOT,2);
}

int32_t download_agent(stAgDlInfo *agInfo)
{
    return burn_agboot(agInfo);
}

int32_t download_image(enBurnImageType imgType,char *path, uint32_t addr, enStorageType storType, uint32_t dribble_dld_en, bool bUsb, bool bSkipAddrAlign, stUsbBootCtrl* uBCtrl)
{
    return burn_image(imgType, path, addr, storType, dribble_dld_en, bUsb, bSkipAddrAlign, uBCtrl);
}

int32_t parse_ini_handler(void *user, const char *group, const char* key, const char *value)
{
    stIniCfg *iniCfg = (stIniCfg*)user;
    #define MATCH(g,k) (strcmp(group, g) == 0 && strcmp(key, k) == 0)

    if (MATCH("config", "agbaud"))
    {
        iniCfg->agBaud = atoi(value);
    }
    if (MATCH("config", "dlbaud"))
    {
        iniCfg->dlBaud = atoi(value);
    }
    if (MATCH("control", "detect"))
    {
        iniCfg->detect = atoi(value);
    }
    if (MATCH("control", "atreset"))
    {
        strcpy(iniCfg->rstAtCmd, value);    
    }
    if (MATCH("control", "atbaud"))
    {
        iniCfg->atBaud = atoi(value);
    }
    if (MATCH("control", "pullup_qspi"))
    {
        iniCfg->pullupQspi = atoi(value);
    }
    if (MATCH("control", "debug"))
    {
        iniCfg->debug = atoi(value);
    }
    if (MATCH("control", "uart_timeout"))
    {
        iniCfg->uart_timeout = atoi(value);
    }
    if (MATCH("control", "dribble_dld_en"))
    {
        iniCfg->dribble_dld_en = atoi(value);
    }
    if (MATCH("control", "usb_enable"))
    {
        iniCfg->usb_enable = atoi(value);
    }
    if (MATCH("control", "skip_addr_align"))
    {
        iniCfg->skip_addr_align = atoi(value);
    }
    if (MATCH("control", "dld_upg_ctrl_valid"))
    {
        iniCfg->dld_upg_ctrl_valid = atoi(value);
    }
    if (MATCH("control", "dld_upg_connwait_100ms_cnt"))
    {
        iniCfg->dld_upg_connwait_100ms_cnt = atoi(value);
    }
    if (MATCH("control", "dld_upg_ctrlwait_100ms_cnt"))
    {
        iniCfg->dld_upg_ctrlwait_100ms_cnt = atoi(value);
    }
    if (MATCH("storage_cfg", "format_path"))
    {
        strcpy(iniCfg->formatPath, value);
    }
    if (MATCH("package_info", "pkgpath"))
    {
        strcpy(iniCfg->pkgPath, value);
    }
    if (MATCH("agentboot", "agpath"))
    {
        strcpy(iniCfg->agPath, value);
    }
    if (MATCH("agentboot", "ctrl"))
    {
        iniCfg->agCtrlMagic = atoi(value);
    }
    if (MATCH("agentboot", "ap_size"))
    {
        iniCfg->apSize = atoi(value);
    }
    if (MATCH("agentboot", "cp_size"))
    {
        iniCfg->cpSIze = atoi(value);
    }
    if (MATCH("bootloader", "blpath"))
    {
        strcpy(iniCfg->blPath, value);
    }
    if (MATCH("bootloader", "burnaddr"))
    {
        iniCfg->blBurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("system", "syspath"))
    {
        strcpy(iniCfg->sysPath, value);
    }
    if (MATCH("system", "burnaddr"))
    {
        iniCfg->sysnBurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("cp_system", "cp_syspath"))
    {
        strcpy(iniCfg->cpSysPath, value);
    }
    if (MATCH("cp_system", "burnaddr"))
    {
        iniCfg->cpSysBurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("cp_system", "storage_type"))
    {
        strcpy(iniCfg->cpSysStorType, value);
    }
    if (MATCH("flexfile0", "filepath"))
    {
        strcpy(iniCfg->flexF0Path, value);
    }
    if (MATCH("flexfile0", "burnaddr"))
    {
        iniCfg->flexF0BurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("flexfile0", "storage_type"))
    {
        strcpy(iniCfg->flexF0StorType, value);
    }
    if (MATCH("otherfile1", "filepath"))
    {
        strcpy(iniCfg->otherF1Path, value);
    }
    if (MATCH("otherfile1", "burnaddr"))
    {
        iniCfg->otherF1BurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("otherfile1", "storage_type"))
    {
        strcpy(iniCfg->otherF1StorType, value);
    }
    if (MATCH("otherfile2", "filepath"))
    {
        strcpy(iniCfg->otherF2Path, value);
    }
    if (MATCH("otherfile2", "burnaddr"))
    {
        iniCfg->otherF2BurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("otherfile2", "storage_type"))
    {
        strcpy(iniCfg->otherF2StorType, value);
    }
    if (MATCH("otherfile3", "filepath"))
    {
        strcpy(iniCfg->otherF3Path, value);
    }
    if (MATCH("otherfile3", "burnaddr"))
    {
        iniCfg->otherF3BurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("otherfile3", "storage_type"))
    {
        strcpy(iniCfg->otherF3StorType, value);
    }
    if (MATCH("otherfile4", "filepath"))
    {
        strcpy(iniCfg->otherF4Path, value);
    }
    if (MATCH("otherfile4", "burnaddr"))
    {
        iniCfg->otherF4BurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("otherfile4", "storage_type"))
    {
        strcpy(iniCfg->otherF4StorType, value);
    }
    if (MATCH("otherfile5", "filepath"))
    {
        strcpy(iniCfg->otherF5Path, value);
    }
    if (MATCH("otherfile5", "burnaddr"))
    {
        iniCfg->otherF5BurnAddr = (uint32_t)strtoul(value, NULL, 16);
    }
    if (MATCH("otherfile5", "storage_type"))
    {
        strcpy(iniCfg->otherF5StorType, value);
    }
    return 1;
}

int32_t parse_dldupg_handler(void *user, const char *group, const char* key, const char *value)
{
    stDldUpg *dldUpg = (stDldUpg*)user;
    #define MATCH(g,k) (strcmp(group, g) == 0 && strcmp(key, k) == 0)

    if (MATCH("control", "dld_upg_ctrl_valid"))
    {
        dldUpg->dld_upg_ctrl_valid = atoi(value);
    }
    if (MATCH("control", "dld_upg_connwait_100ms_cnt"))
    {
        dldUpg->dld_upg_connwait_100ms_cnt = atoi(value);
    }
    if (MATCH("control", "dld_upg_ctrlwait_100ms_cnt"))
    {
        dldUpg->dld_upg_ctrlwait_100ms_cnt = atoi(value);
    }

    return 1;
}


int32_t generate_bin(FILE* fp, uint32_t size, char *binPath)
{
    FILE *binF = NULL;
    uint8_t *buf = NULL;
    uint32_t remain, len;

    if (fp == NULL)
        return -1;
    binF = fopen(binPath, "wb");
    if (binF == NULL)
    {
        // fclose(binF);
        ELOG("File could not access or has no permission to handle, path = %s", binPath);
        return -1;
    }
    buf = (uint8_t*)xmalloc(MAX_SPLIT_PKG_BUFFER_SIZE);
    remain = size;
    while (remain > 0)
    {
        if (remain > MAX_SPLIT_PKG_BUFFER_SIZE)
            len = MAX_SPLIT_PKG_BUFFER_SIZE;
        else
            len = remain;
        if (len != fread(buf, 1, len, fp))
            break;
        if (len != fwrite(buf, 1, len, binF))
            break;
        remain -= len;
    }
    fflush(binF);
    fclose(binF);
    xfree(buf);

    if (remain > 0)
    {
        ELOG("Generate bin file fail, remain = %u", remain);
        return -1;
    }
        
    
    return 0;
}

char* get_img_path(enBurnImageType imgType, stIniCfg* iniCfg)
{
    switch(imgType)
    {
        case BTYPE_BOOTLOADER:
            return iniCfg->blPath;
        case BTYPE_AP:
            return iniCfg->sysPath;
        case BTYPE_CP:
            return iniCfg->cpSysPath;
        case BTYPE_FLEXFILE:
            return iniCfg->flexF0Path;
        case BTYPE_OTHER1:
            return iniCfg->otherF1Path;
        case BTYPE_OTHER2:
            return iniCfg->otherF2Path;
        case BTYPE_OTHER3:
            return iniCfg->otherF3Path;
        case BTYPE_OTHER4:
            return iniCfg->otherF4Path;
        case BTYPE_OTHER5:
            return iniCfg->otherF5Path;
        default:
            return NULL;
    }
}

uint32_t get_img_addr(enBurnImageType imgType, stIniCfg* iniCfg)
{
    switch(imgType)
    {
        case BTYPE_BOOTLOADER:
            return iniCfg->blBurnAddr;
        case BTYPE_AP:
            return iniCfg->sysnBurnAddr;
        case BTYPE_CP:
            return iniCfg->cpSysBurnAddr;
        case BTYPE_FLEXFILE:
            return iniCfg->flexF0BurnAddr;
        case BTYPE_OTHER1:
            return iniCfg->otherF1BurnAddr;
        case BTYPE_OTHER2:
            return iniCfg->otherF2BurnAddr;
        case BTYPE_OTHER3:
            return iniCfg->otherF3BurnAddr;
        case BTYPE_OTHER4:
            return iniCfg->otherF4BurnAddr;
        case BTYPE_OTHER5:
            return iniCfg->otherF5BurnAddr;
        default:
            return 0;
    }
}

uint32_t get_storage_type(enBurnImageType imgType, stIniCfg* iniCfg)
{
    char* type = NULL;
    switch (imgType)
    {
        case BTYPE_BOOTLOADER:
        case BTYPE_AP:
            return (uint32_t)STYPE_AP_FLASH;
        case BTYPE_CP:
            type = iniCfg->cpSysStorType;
            if (getStoreType(type) == STYPE_INVALID)
                return (uint32_t)STYPE_CP_FLASH; 
            else
                return getStoreType(type);
        case BTYPE_FLEXFILE:
            type = iniCfg->flexF0StorType;
            return getStoreType(type);
        case BTYPE_OTHER1:
            type = iniCfg->otherF1StorType;
            return getStoreType(type);
        case BTYPE_OTHER2:
            type = iniCfg->otherF2StorType;
            return getStoreType(type);
        case BTYPE_OTHER3:
            type = iniCfg->otherF3StorType;
            return getStoreType(type);
        case BTYPE_OTHER4:
            type = iniCfg->otherF4StorType;
            return getStoreType(type);
        case BTYPE_OTHER5:
            type = iniCfg->otherF5StorType;
            return getStoreType(type);
        default:
            return (uint32_t)STYPE_INVALID; 
    }
}

uint32_t getStoreType(char* type)
{
    if (strstr((const char*)type, "ap_flash") != NULL)
        return (uint32_t)STYPE_AP_FLASH;
    else if (strstr((const char*)type, "cp_flash") != NULL)
        return (uint32_t)STYPE_CP_FLASH;
    else
        return (uint32_t)STYPE_INVALID;
}

int32_t parse_erase_params(char *params, uint32_t *addr, uint32_t *size, uint8_t *type)
{
    char tempBuf[32];
    char s[32];
    char *beg = NULL;
    char *end = NULL;
    uint8_t len;

    strcpy(s, params);
    memset(tempBuf, 0, sizeof(tempBuf));

    beg = s;
    end = strchr(s, ' ');
    if (end == NULL)
    {
        DLOG("Erase params is incomplete, only has address param");
        return -1;
    }
    len = end - beg;
    strncpy(tempBuf, beg, len);
    *addr = (uint32_t)strtoul(tempBuf, NULL, 16);

    strcpy(s, end+1);
    beg = s;
    end = strchr(s, ' ');
    if (end == NULL)
    {
        DLOG("Erase params is incomplete, only has address and length params. default erase type = 0");
        *size = atoi(s);
        *type = 0;
        return 0;
    }
    len = end - beg;
    memset(tempBuf, 0, sizeof(tempBuf));
    strncpy(tempBuf, beg, len);
    *size = (uint32_t)strtoul(tempBuf, NULL, 16);

    strcpy(s, end+1);
    *type = atoi(s);

    return 0;
}

int32_t parse_burnlist_params(char *params, char (*fList)[10], uint8_t *size)
{
    uint8_t type_len;
    uint8_t i,j;

    type_len = sizeof(gImage_type_info)/sizeof(gImage_type_info[0]);
    uppercase(params);
    j = 0;
    for (i=0; i<type_len; i++)
    {    
        if (strstr((const char*)params, gImage_type_info[i].typeName) != NULL)
        {
            strcpy(fList[j], gImage_type_info[i].typeName);
            j++;
        }
    }
    *size = j;
    if (j == 0)
    {
        ELOG("burnlist params is unsupport, params = %s", params);
        return -1;
    }
    return 0;
}

int32_t parse_eraselist_params(char *params, char *all, char *nvm, char *cal)
{
//    uint8_t type_len;
    uint8_t j;

//    type_len = sizeof(gErase_type_info)/sizeof(gErase_type_info[0]);
    uppercase(params);
    j = 0;
    if (strstr((const char*)params, gErase_type_info[0].typeName) != NULL)
    {
        *all = 1;
        j++;
    }
    if (strstr((const char*)params, gErase_type_info[1].typeName) != NULL)
    {
        *nvm = 1;
        j++;
    }
    if (strstr((const char*)params, gErase_type_info[2].typeName) != NULL)
    {
        *cal = 1;
        j++;
    }
    if (j == 0)
    {
        ELOG("eraselist params is unsupport, params = %s", params);
        return -1;
    }
    return 0;
}

int32_t parse_readmem_params(char *params, uint32_t *addr, uint32_t *size, char *path)
{
    char tempBuf[280];
    char s[280];
    char *beg = NULL;
    char *end = NULL;
    uint8_t len;

    strcpy(s, params);
    memset(tempBuf, 0, sizeof(tempBuf));

    // addr
    beg = s;
    end = strchr(s, ' ');
    if (end == NULL)
    {
        DLOG("ReadMem params is incomplete, only has address param");
        return -1;
    }
    len = end - beg;
    strncpy(tempBuf, beg, len);
    *addr = (uint32_t)strtoul(tempBuf, NULL, 16);

    //size
    strcpy(s, end+1);
    beg = s;
    end = strchr(s, ' ');
    if (end == NULL)
    {
        DLOG("ReadMem params is incomplete, only has address and length params, no save file path");
        *size = atoi(s);
        return 0;
    }
    len = end - beg;
    memset(tempBuf, 0, sizeof(tempBuf));
    strncpy(tempBuf, beg, len);
    *size = (uint32_t)strtoul(tempBuf, NULL, 16);

    // path
    strcpy(path, end+1);

    return 0;
}

int32_t erase_list(char *port, char *cfgPath, char *jsonPath, char all, char nvm, char cal)
{
    FILE  *fp = NULL;
    cJSON *json = NULL;
    uint32_t fSize;
    uint8_t  i = 0;
    char  *jsBuf = NULL;
    cJSON *arrayItem = NULL;
    cJSON *obj = NULL;
    cJSON *item = NULL;
    uint32_t ap_flash_addr = 0;
    uint32_t cp_flash_addr = 0;
    uint32_t ap_flash_size = 0;
    uint32_t cp_flash_size = 0;
    uint32_t erase_addr = 0x0;
    uint32_t erase_size = 0x0;
    char skipAgDl = 0; 
    uint32_t ofst_flag = 0;

    fSize = get_file_size(jsonPath);
    if (fSize == 0)
    {
        ELOG("Json file is empty or not exist, filepath = %s", jsonPath);
        return -1;
    }
    jsBuf = (char*)xmalloc(fSize+1);
    fp = fopen(jsonPath, "r");
    fread(jsBuf, 1, fSize, fp);
    cJSON_Minify(jsBuf);  //2024-11-14, Shijiaxing, add rubost support for allow comments in json.
    json = cJSON_Parse(jsBuf);
    if (json == NULL)
    {
        ELOG("cJSON_Parse error: %s,  path = %s", cJSON_GetErrorPtr(), jsonPath);
        xfree(jsBuf);
        return -1;
    }
    if ((item = cJSON_GetObjectItem(json, "ap_flash_addr")))
    {
        ap_flash_addr = (uint32_t)strtoul(item->valuestring, NULL, 16);
    }
    if ((item = cJSON_GetObjectItem(json, "cp_flash_addr")))
    {
        cp_flash_addr = (uint32_t)strtoul(item->valuestring, NULL, 16);
    }
    if ((item = cJSON_GetObjectItem(json, "ap_flash_length")))
    {
        ap_flash_size = (uint32_t)strtoul(item->valuestring, NULL, 16);
    }
    if ((item = cJSON_GetObjectItem(json, "cp_flash_length")))
    {
        cp_flash_size = (uint32_t)strtoul(item->valuestring, NULL, 16);
    }
    // erase all, include cal nv
    if ((all != 0) && (nvm != 0) && (cal != 0))
    {
        if (ap_flash_size > 0)
        {
            if (0 != erase_one(port, cfgPath, ap_flash_addr, ap_flash_size, skipAgDl, 0))
            {
                xfree(jsBuf);
                return -1;
            }
            skipAgDl = 1;
        }
        if (cp_flash_size > 0)
        {
            if (0 != erase_one(port, cfgPath, cp_flash_addr, cp_flash_size, skipAgDl, 0))
            {
                xfree(jsBuf);
                return -1;
            }
            skipAgDl = 1;
        }
        return 0;
    }
    if (all != 0)
    {
        arrayItem = cJSON_GetObjectItem(json, "erallum");
        while ((obj=cJSON_GetArrayItem(arrayItem, i)))
        {
            item = cJSON_GetObjectItem(obj, "begin");
            erase_addr = (uint32_t)strtoul(item->valuestring, NULL, 16);
            item = cJSON_GetObjectItem(obj, "length");
            erase_size = (uint32_t)strtoul(item->valuestring, NULL, 16);
            item = cJSON_GetObjectItem(obj, "offst_flag");
            ofst_flag = (uint32_t)strtoul(item->valuestring, NULL, 10);
            item = cJSON_GetObjectItem(obj, "stor_type");
            if (strstr(item->valuestring, "ap_flash") != NULL)
            {
                if (ofst_flag == 1)
                    erase_addr += ap_flash_addr;
            }
            else if (strstr(item->valuestring, "cp_flash") != NULL)
            {
                if (ofst_flag == 1)
                    erase_addr += cp_flash_addr;
            }
            else;
            if (0 != erase_one(port, cfgPath, erase_addr, erase_size, skipAgDl, 0))
            {
                xfree(jsBuf);
                return -1;
            }
            i++;
            skipAgDl = 1;
        }
    }
    if (nvm != 0)
    {
        obj = cJSON_GetObjectItem(json, "fs");
        item = cJSON_GetObjectItem(obj, "begin");
        erase_addr = (uint32_t)strtoul(item->valuestring, NULL, 16);
        item = cJSON_GetObjectItem(obj, "length");
        erase_size = (uint32_t)strtoul(item->valuestring, NULL, 16);
        if (0 != erase_one(port, cfgPath, erase_addr, erase_size, skipAgDl, 0))
        {
            xfree(jsBuf);
            return -1;
        }
        skipAgDl = 1;
        obj = cJSON_GetObjectItem(json, "fraw");
        item = cJSON_GetObjectItem(obj, "begin");
        erase_addr = (uint32_t)strtoul(item->valuestring, NULL, 16);
        item = cJSON_GetObjectItem(obj, "length");
        erase_size = (uint32_t)strtoul(item->valuestring, NULL, 16);
        if (0 != erase_one(port, cfgPath, erase_addr, erase_size, skipAgDl, 0))
        {
            xfree(jsBuf);
            return -1;
        }
    }
    if (cal != 0)
    {
        arrayItem = cJSON_GetObjectItem(json, "cal");
        while ((obj=cJSON_GetArrayItem(arrayItem, i)))
        {
            item = cJSON_GetObjectItem(obj, "begin");
            erase_addr = (uint32_t)strtoul(item->valuestring, NULL, 16);
            item = cJSON_GetObjectItem(obj, "length");
            erase_size = (uint32_t)strtoul(item->valuestring, NULL, 16);
            item = cJSON_GetObjectItem(obj, "offst_flag");
            ofst_flag = (uint32_t)strtoul(item->valuestring, NULL, 10);
            item = cJSON_GetObjectItem(obj, "stor_type");
            if (strstr(item->valuestring, "ap_flash") != NULL)
            {
                if (ofst_flag == 1)
                    erase_addr += ap_flash_addr;
            }
            else if (strstr(item->valuestring, "cp_flash") != NULL)
            {
                if (ofst_flag == 1)
                    erase_addr += cp_flash_addr;
            }
            else;
            if (0 != erase_one(port, cfgPath, erase_addr, erase_size, skipAgDl, 0))
            {
                xfree(jsBuf);
                return -1;
            }
            i++;
            skipAgDl = 1;
        }
    }
    xfree(jsBuf);

    return 0;
}

int erase_one(char *port, char *cfgPath, uint32_t addr, uint32_t size, char skipAgDl, uint8_t type)
{
    int ret = -1;
    stPrimCliCfg cfg;
    char tmpBuf[32];

    cfg.skipAgDl = skipAgDl;
    strcpy(cfg.port, port);
    strcpy(cfg.cfgFile, cfgPath);
    memset(tmpBuf, 0, sizeof(tmpBuf));
    sprintf(tmpBuf, "%08x", addr);
    tmpBuf[8] = ' ';
    sprintf(tmpBuf+9, "%08x", size);
    tmpBuf[17] = ' ';
    tmpBuf[18] = (char)type;
    memcpy(cfg.eraseSection, tmpBuf, sizeof(tmpBuf));

    ret = action_erase(&cfg);

    return ret;
}

bool checkMemAddr(uint32_t addr, uint32_t len)
{
    // for ecmemhoellist may be not the same for each chip plate,
    // and i do not get information about this, so here we ignore
    // the check of memory's validity temporarily
    return true;
}
