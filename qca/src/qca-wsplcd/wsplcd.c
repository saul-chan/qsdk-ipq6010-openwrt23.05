/* @File: wsplcd.c
 * @Notes:  IEEE1905 AP Auto-Configuration Daemon
 *          AP Enrollee gets wifi configuration from AP Registrar via
 *          authenticated IEEE1905 Interfaces
 *
 * Copyright (c) 2012, 2015-2018 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * 2012, 2015-2016 Qualcomm Atheros, Inc.
 *
 * Qualcomm Atheros Confidential and Proprietary.
 * All rights reserved.
 *
 */

/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "wsplcd.h"
#include "eloop.h"
#include "ucpk_hyfi20.h"
#include "apac_priv.h"
#include "apac_hyfi20_mib.h"
#include "apac_hyfi20_wps.h"
#include <sys/time.h>
#include <sys/stat.h>
#if MAP_ENABLED
#include "apac_map.h"
#endif
#include <signal.h>
#if CALLTRACE_SUPPORT
#include <libunwind.h>
#endif

#ifdef SON_MEMORY_DEBUG
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <wlan_son_ioctl.h>


#ifdef QCA_EXT_EM_CONTROLLER
#include "emex.h"
#ifdef BUILD_RDKB
#include "syscfg/syscfg.h"
#endif
#endif

char g_log_file_path[APAC_CONF_FILE_NAME_MAX_LEN];

char g_cfg_file[APAC_CONF_FILE_NAME_MAX_LEN];
#if MAP_ENABLED
char g_map_cfg_file[APAC_CONF_FILE_NAME_MAX_LEN];
char g_map_cfg_bsta_file[APAC_CONF_FILE_NAME_MAX_LEN];
apacMapEProfileMatcherType_e g_map_cfg_file_format;
#endif

int debug_level = MSG_INFO;
apacLogFileMode_e logFileMode = APAC_LOG_FILE_INVALID;

FILE *pLogFile = NULL;

#ifdef QCA_EXT_EM_CONTROLLER
ieee1905_call_bks_t cbs;
static int emex_active;
#endif

extern int wlanIfConfigInit(u32);
void wlanIfConfigExit(void);

int dprintf(int level, const char *fmt, ...)
{
    va_list ap = {0};
    struct timeval curTime;
    struct stat st;
    char * EnvS = getenv(WSPLCD_DBG_FILE_SIZE);
    long int filesize_limit=0;

    if(EnvS && *EnvS) {
        filesize_limit = atol(EnvS) * 1024; // KiloByte to Byte conversion
    }

    if (stat(g_log_file_path,&st) == -1) {
        pLogFile = fopen(g_log_file_path, "a+");
    }

    va_start(ap, fmt);
    if (level >= debug_level) {
        if (pLogFile) {
            gettimeofday(&curTime, NULL);
            if(filesize_limit && st.st_size > filesize_limit) {
                ftruncate(fileno(pLogFile), (off_t)0); //truncating the file
            }
            fprintf(pLogFile, "[%lu.%lu] ", curTime.tv_sec, curTime.tv_usec);
            vfprintf(pLogFile, fmt, ap);
            fflush(pLogFile);
        } else {
            vprintf(fmt, ap);
        }
    }
    va_end(ap);
    return 0;
}

void shutdown_fatal(void)
{
    if (pLogFile) {
        fclose(pLogFile);
    }
    exit(1);
}

void apacHyfi20CheckArgList(int *argc, char **argv) {
    char tempArgvList[*argc][APAC_CONF_FILE_NAME_MAX_LEN];
    u_int8_t i, j = 0;

	memset(tempArgvList, 0, sizeof(tempArgvList));

    for (i = 0; i < *argc; i++) {
        if (strlen(argv[i]) > 0) {
            memcpy(tempArgvList[j++], argv[i], strlen(argv[i])+1);
        }
    }

    for (i = 0; i < j; i++) {
        memcpy(argv[i], tempArgvList[i], strlen(tempArgvList[i])+1);
		//Updating the Next argument address w.r.t Non zero Temp Buffer
		if ( (i+1) < j ) {
			argv[i+1] = argv[i] + strlen(tempArgvList[i]) + 1;
		}
    }

    *argc = i;
}

#if CALLTRACE_SUPPORT
/**
 * @brief storing the calltrace info into a separate file
 */

FILE *fpLog;
int wsplcd_print(const char *fmt, ...)
{
    va_list ap = {0};
    struct timeval curTime;

    va_start(ap, fmt);
    if (fpLog) {
        gettimeofday(&curTime, NULL);
        fprintf(fpLog, "[%lu.%lu] ", curTime.tv_sec, curTime.tv_usec);
        vfprintf(fpLog, fmt, ap);
        fflush(fpLog);
    } else {
        vprintf(fmt, ap);
    }
    va_end(ap);
    return 0;
}

/**
 * @brief React to a signal to shut down the daemon by marking the event
 *        event loop as terminated.
 */

static void wsplcdShutdownSignalHandler(int signal) {
    fpLog  = fopen("/tmp/wsplcdCrash_log.txt", "a");
    unw_cursor_t cursor;
    unw_context_t context;
    char sym[256];

    switch(signal)
    {
      case SIGSEGV:
          wsplcd_print("SIGSEGV : Segmentation Fault\n");
      break;
      case SIGTERM:
          wsplcd_print("SIGTERM : Terminate\n");
      break;
      case SIGQUIT:
          wsplcd_print("SIGQUIT : Quit\n");
      break;
      case SIGILL:
          wsplcd_print("SIGILL : Illegal Instruction\n");
      break;
      case SIGTRAP:
          wsplcd_print("SIGTRAP : Trap\n");
      break;
      case SIGABRT:
          wsplcd_print("SIGABRT : Abort\n");
      break;
      case SIGFPE:
          wsplcd_print("SIGFPE : Floating-point Exception\n");
      break;
      default:
          wsplcd_print("Default Case Detected ! \n");
      break;
    }
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    wsplcd_print("Process id : %d\n", getpid());

    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_SP, &pc);
        if (pc == 0)
            break;
        wsplcd_print("0x%x    ", pc);
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            wsplcd_print("Symbol : (%s+0x%x)\n", sym, offset);
        } else {
            wsplcd_print(" -- error: unable to obtain symbol name for this frame\n");
        }
    }
    wsplcd_print ("\n\n\n\n");
    fclose(fpLog);
    evloopAbort();
    exit(0);
 }
#endif

#ifdef SON_MEMORY_DEBUG

/*
 * @brief display memory usage summary information for every periodic interval
 */
static void apacSonMemDebugPeriodicTimeoutHandler(void *eloop_ctx, void *timeout_ctx)
{
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20Config_t *pConfig = &(pData->config);
    son_mem_dbg_display_list();
    eloop_cancel_timeout(apacSonMemDebugPeriodicTimeoutHandler, pData, NULL);
    eloop_register_timeout(pConfig->report_interval, 0, apacSonMemDebugPeriodicTimeoutHandler, pData, NULL);
}

#endif

static void wsplcdMeshSignalHandler(int sig, void *eloop_ctx, void *signal_ctx) {

    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)signal_ctx;
    apacHyfi20Config_t *pConfig = &(pData->config);

    switch(sig)
    {
      case SIGTERM:
          dprintf(MSG_INFO, "SIGTERM : Terminate\n");
      break;
      case SIGQUIT:
          dprintf(MSG_INFO, "SIGQUIT : Quit\n");
      break;
      case SIGINT:
          dprintf(MSG_INFO, "SIGINT : Terminate\n");
      break;
      default:
          dprintf(MSG_INFO, "Default Case Detected ! Signal:%d \n",sig);
      break;
    }

    // on enrollee, we need to cleanup any active session
    if (pConfig->role == APAC_ENROLLEE && pData->sess_list) {
        dprintf(MSG_INFO, "Deleting active session on enrollee\n");
        apac_wps_del_session(pData->sess_list);
    }

    exit(0);
}

void wsplcdMaskSignals() {
    sigset_t mask;
    dprintf(MSG_INFO, "%s: Masking signals\n", __func__);
    sigemptyset(&mask);
    /* add required signals to the mask */
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_SETMASK, &mask, NULL);
}

void wsplcdUnmaskSignals() {
    sigset_t mask;
    dprintf(MSG_INFO, "%s: Resetting signal mask\n", __func__);
    sigemptyset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);
}

s32 wsplcdUciGetInt(const char *config) {
    s32 val = -1;
    FILE *fp = NULL;
    char buff[128] = {0};

    if (!config) {
        dprintf(MSG_ERROR, "%s: invalid input\n", __func__);
        return val;
    }

    snprintf(buff, sizeof(buff), "uci get %s", config);
    dprintf(MSG_INFO, "%s: running cmd: %s\n", __func__, buff);
    fp = popen(buff, "r");
    if (fp && fgets(buff, sizeof(buff), fp)) {
        dprintf(MSG_MSGDUMP, "%s: buff: %s\n", __func__, buff);
        val = atoi(buff);
        dprintf(MSG_INFO, "%s: val %d\n", __func__, val);
    }
    else {
        dprintf(MSG_ERROR, "%s: unable to read stream from pipe\n", __func__);
    }

    if (fp) {
        pclose(fp);
    }

    return val;
}

/**
 * @brief Get Version Compatibility check
 *
 * This function will identify for first available WLAN Interface name
 * and then invoke the cfg API to perform the check
 *
 * @param [in] void
 *
 * @return void
 */
void wsplcdGetVersionCompatibilityCheck(apacHyfi20Data_t *ptrData, char *ver_str) {

    int ret = -1;
    apacHyfi20IF_t *ptrIF = NULL;

    dprintf(MSG_ERROR, "Application version string [%s]\n", ver_str);

    if ((ptrIF = apacHyfi20FindFirstValidWLANInterface(ptrData)) != NULL ) {
        ret = apacHyfi20GetVersionCompatibility(ptrIF, ver_str, strlen(ver_str));
    }

    if (ret == 0) {
        dprintf(MSG_ERROR, "Version Compatibility check PASSED !!!\n");
    } else if (ret == 1) {
        dprintf(MSG_ERROR, "Version Compatibility check FAILED !!!\n");
    } else {
        dprintf(MSG_ERROR, "Unknown return status !!!\n");
    }
}

#ifdef QCA_EXT_EM_CONTROLLER
static inline void extract_uint8(uint8_t **packet, uint8_t *val)
{
    *val       = **packet;
    (*packet) += 1;
}

/* Callback from IEEE 1905 module to retrieve  EM+ configs. */
static int emex_get_config_cb(char *config, char *path, char *val)
{
    int ret = 0;
    if (emex_active) {
#ifdef BUILD_RDKB
        ret = emex_platform_get(config, path, val);
#else
        ret = emex_platform_get_uci(config, path, val);
#endif
        dprintf(MSG_INFO,"EMEX config gathering was %s\n", ret == 0 ?  "failed" : "succeeded" );
    }
    return ret;
}

/* Callback function to understand EM+ vendor tlv */
static int emex_vendor_message_cb(unsigned char **tlvs,unsigned int *len)
{
    int ret = -1;
    if (emex_active) {
        uint8_t oui;

	/* check oui */
	extract_uint8(tlvs, &oui);
	if (oui!=EMEX_VENDOR_OUI_1) {
		return ret;
	}

	extract_uint8(tlvs, &oui);
	if (oui!=EMEX_VENDOR_OUI_2) {
		return ret;
	}

	extract_uint8(tlvs, &oui);
	if (oui!=EMEX_VENDOR_OUI_3) {
		return ret;
	}

	*len -= 3;
	dprintf(MSG_INFO,"emex_vendor_message_cb\n");
	ret = 0;
    }
    return ret;
}

#ifdef BUILD_RDKB
int emex_platform_get(char *module, char* path, char* value){

    char optv[256];
    int ret = 0;
    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "%s.%s", module, path);

    if (syscfg_get(NULL, optv, value, UCI_MAX_STR_LEN) == 0 ) {
        if (strlen(value) > 0){
            dprintf(MSG_INFO,"emex_platform_get success for %s.%s",module,path);
            ret = 1 ;
        }
    }
    dprintf(MSG_INFO,"emex_platform_get failed for %s.%s",module,path);
    return ret;
}
#endif


/* Callback from IEEE 1905 module to init  EM+ extensions. */
static int emex_init_cb()
{
    int ret = -1;
    int uci_ret = 0;
    char ucival[UCI_MAX_STR_LEN] = {'\0'};

#ifndef BUILD_RDKB
    uci_ret = emex_platform_get_uci(EZMESH_UCI_MODULE, EMEX_UCI_ENABLE, ucival);
#else
    uci_ret = emex_platform_get(EZMESH_UCI_MODULE, EMEX_UCI_ENABLE, ucival);
#endif
    dprintf(MSG_INFO, "EMEX_UCI_ENABLE val %s \n",ucival);

    if (( uci_ret == 0 || strcmp(ucival,"0")) && !emex_active) {
         ret = emex_init(EMEX_TYPE_WSPLCD, (void *)NULL);
         dprintf(MSG_INFO,"EMEX init was %s\n", ret == 0 ? "succeeded" : "failed");
    } else {
         dprintf(MSG_INFO,"EMEX is NOT enabled or already active.\n");
    }
    return ret;
}

/* Callback from IEEE 1905 module to deinit  EM+ extensions. */
static void emex_deinit_cb()
{
    if (emex_active) {
        emex_deinit();
        emex_active=0;
        dprintf(MSG_INFO,"EMEX deinit was performed.\n");
    }
}

static int emex_prepare_tlv_cb(uint8_t type, uint16_t emex_type, uint8_t **pbuf, uint16_t *payload_len)
{
    int ret = -1;
    if (emex_active){
        ret = emex_prepare_tlv(type, emex_type, pbuf, payload_len);
        dprintf(MSG_INFO,"EMEX TLV generation was %s for type : 0x%04X\n",
                ret == 0 ? "succeeded" : "failed", emex_type);
    }
    return ret;
}

static void register_ieee1905_callbacks(void)
{
    int ret = 0;
    char ucival[UCI_MAX_STR_LEN] = {'\0'};
#ifndef BUILD_RDKB
    ret = emex_platform_get_uci(EZMESH_UCI_MODULE, EMEX_UCI_ENABLE, ucival);
#else
    ret = emex_platform_get(EZMESH_UCI_MODULE, EMEX_UCI_ENABLE, ucival);
#endif
    dprintf(MSG_INFO, "EMEX_UCI_ENABLE val %s\n",ucival);

    if (ret == 0 || strcmp(ucival,"0")) {
        cbs.emex_get_config = emex_get_config_cb;
        cbs.emex_init = emex_init_cb;
        cbs.emex_deinit = emex_deinit_cb;
        cbs.emex_prepare_tlv = emex_prepare_tlv_cb;
        cbs.emex_vendor_message = emex_vendor_message_cb;
    }
}
#endif

int main(int argc, char **argv)
{
    apacInfo_t apacInfo;
#ifdef SON_MEMORY_DEBUG
    apacHyfi20Config_t *ptrConfig = &apacInfo.hyfi20.config;
#endif


    memset(&apacInfo, 0, sizeof(apacInfo_t));

    apacHyfi20CheckArgList(&argc, argv);
    apacHyfi20CmdLogFileModeName(argc, argv);

    if (logFileMode == APAC_LOG_FILE_APPEND) {
        pLogFile = fopen(g_log_file_path, "a");
    } else if (logFileMode == APAC_LOG_FILE_TRUNCATE) {
        pLogFile = fopen(g_log_file_path, "w");
    }

    /* enable command line configuration or read config file */
    optind = 0;
#if CALLTRACE_SUPPORT
    signal(SIGINT, wsplcdShutdownSignalHandler);
    signal(SIGTERM, wsplcdShutdownSignalHandler);
    signal(SIGSEGV, wsplcdShutdownSignalHandler);
    signal(SIGQUIT, wsplcdShutdownSignalHandler);
    signal(SIGILL, wsplcdShutdownSignalHandler);
    signal(SIGTRAP, wsplcdShutdownSignalHandler);
    signal(SIGABRT, wsplcdShutdownSignalHandler);
    signal(SIGFPE, wsplcdShutdownSignalHandler);
#endif

    apacHyfi20CmdConfig(&apacInfo.hyfi20, argc, argv);

    if (wlanIfConfigInit(apacInfo.hyfi20.isCfg80211))
    {
        dprintf(MSG_INFO, "wlanIfConfigInit Failed\n");
        return -1;
    }

#if HYFI10_COMPATIBLE
    /* set up default configuration */
    wsplcd_hyfi10_init(&apacInfo.hyfi10);
#endif

    apacHyfi20ConfigInit(&apacInfo.hyfi20);

#ifdef QCA_EXT_EM_CONTROLLER
    register_ieee1905_callbacks();
#endif

    /* Start wsplcd daemon */
    dprintf(MSG_INFO, "wsplcd daemon starting. cfg80211 config %d \n",apacInfo.hyfi20.isCfg80211);

#ifdef QCA_EXT_EM_CONTROLLER
    int ret = 0;
    char ucival[UCI_MAX_STR_LEN] = {'\0'};
#ifndef BUILD_RDKB
        ret = emex_platform_get_uci(EZMESH_UCI_MODULE, EMEX_UCI_ENABLE, ucival);
#else
        ret = emex_platform_get(EZMESH_UCI_MODULE, EMEX_UCI_ENABLE, ucival);
#endif

    dprintf(MSG_INFO, "EMEX_UCI_ENABLE val %s\n",ucival);

    if ( ret == 0 || strcmp(ucival,"0") ) {
        if (!emex_init(EMEX_TYPE_WSPLCD, (void *)NULL)) {
            dprintf(MSG_INFO, "EMEX init in wsplcd\n");
            emex_active = 1;
        } else {
            dprintf(MSG_INFO, "EMEX init failed in wsplcd\n");
        }
    } else {
        dprintf(MSG_INFO, "EMEX is NOT enabled.\n");
    }
#endif

    eloop_init(&apacInfo);

#ifdef SON_MEMORY_DEBUG
    if (ptrConfig->enable_mem_debug && ptrConfig->report_interval) {
        eloop_register_timeout(ptrConfig->report_interval, 0, apacSonMemDebugPeriodicTimeoutHandler, &apacInfo.hyfi20, NULL);
    }
#endif

    if (apacHyfi20Init(&apacInfo.hyfi20) <0)
    {
        dprintf(MSG_INFO, "%s, Failed to initialize\n", __func__);
        return -1;
    }

    /* Get Version Compatibility check */
#if SON_ENABLED
    wsplcdGetVersionCompatibilityCheck(&apacInfo.hyfi20, WSPLCD_SON_VERSION);
#elif MAP_ENABLED
    wsplcdGetVersionCompatibilityCheck(&apacInfo.hyfi20, WSPLCD_MAP_VERSION);
#endif

#if MAP_ENABLED
    if (!apacHyfiMapInit(HYFI20ToMAP(&apacInfo.hyfi20))) {
        if (!apacHyfiMapPfComplianceEnabled(HYFI20ToMAP(&apacInfo.hyfi20))) {
            dprintf(MSG_INFO, "%s: Failed to initialize EasyMesh\n", __func__);
            return -1;
        }
    }
#endif /* MAP_ENABLED */

    apacHyfi20ConfigDump(&apacInfo.hyfi20);

    apacHyfi20AtfConfigDump(&apacInfo.hyfi20);

#if MAP_ENABLED
    apacHyfiMapConfigDump(HYFI20ToMAP(&apacInfo.hyfi20));
#endif /* MAP_ENABLED */
    /* Restore QCA VAPIndependent flag*/
    if (apacInfo.hyfi20.config.manage_vap_ind)
    {
        apac_mib_set_vapind(&apacInfo.hyfi20, apacInfo.hyfi20.config.manage_vap_ind);
    }

    /* UCPK Init*/
    if (strlen(apacInfo.hyfi20.config.ucpk) > 0){
        char wpapsk[62+1];
        char plcnmk[32+1];
        if (ucpkHyfi20Init(apacInfo.hyfi20.config.ucpk,
            apacInfo.hyfi20.config.salt,
            apacInfo.hyfi20.config.wpa_passphrase_type,
            wpapsk,
            plcnmk) < 0)
        {
            dprintf(MSG_INFO, "%s :Invalid 1905.1 UCPK\n", __func__);
        }
        else
        {
            apac_mib_set_ucpk(&apacInfo.hyfi20, wpapsk, plcnmk);
        }
    }

    apacHyfi20Startup(&apacInfo.hyfi20);

#if HYFI10_COMPATIBLE
    /* check compatiblility with Hyfi-1.0 */
    if (apacInfo.hyfi20.config.hyfi10_compatible)
    {
        wsplcd_hyfi10_startup(&apacInfo.hyfi10);
    }
#endif

#if MAP_ENABLED
    if (apacInfo.mapData.vEnabled) {
        if (apacGetPIFMapCap(&apacInfo.hyfi20) < 0) {
            dprintf(MSG_INFO, "%s: Alert MAP Radio Basic Capability failed Aborting \n", __func__);
            return -1;
        }
    }
#endif

    eloop_register_signal(SIGINT, wsplcdMeshSignalHandler, &apacInfo.hyfi20);
    eloop_register_signal(SIGTERM, wsplcdMeshSignalHandler, &apacInfo.hyfi20);
    eloop_register_signal(SIGQUIT, wsplcdMeshSignalHandler, &apacInfo.hyfi20);

    eloop_run();
    eloop_destroy();

#if MAP_ENABLED
    if (apacInfo.mapData.vEnabled) {
        apacHyfiMapDInit(&apacInfo.mapData);
    }
#endif

#if HYFI10_COMPATIBLE
    if (apacInfo.hyfi20.config.hyfi10_compatible)
    {
        wsplcd_hyfi10_stop(&apacInfo.hyfi10);
    }
#endif

    wlanIfConfigExit();

    apacHyfi20DeinitSock(&apacInfo.hyfi20);

    if (pLogFile) {
        fclose(pLogFile);
    }

    /* Probably won't get here... */
    printf("Leaving wsplcd executive program\n");

    return 0;
}


