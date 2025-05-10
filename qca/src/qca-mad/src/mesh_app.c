/*
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/* C and system library includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <fcntl.h>
#include <signal.h>
#define _LINUX_IF_H /* Avoid redefinition of stuff */
#include "cfg80211_nlwrapper_api.h"

/* Hyfi Common Dependencies */
#include <evloop.h>
#include <csh.h>
#include <dbg.h>

/* Mesh Application includes */
#include <mesh_app.h>
#include <../meshevent/meshEvent.h>
#include <../ieee1905/meshIeee1905.h>
#include <../dataelements/dataElements.h>
#include <../dataelements/dataElementsWlan.h>
#include <wlan_son_ioctl.h>

static struct dbgModule *dbgModule;
wifi_cfg80211_context *mesh_cfg80211_ctx;

/* State info for mesh app main level */
struct {
    int DoDaemonize;
    int Mode;
    struct dbgModule *DebugModule;
    int Port; /*telnet port used for debug output */
} meshAppS;

/* Debugging options */
const struct dbgInitOptions meshAppDbgInitOptions = {
    .ProgramName = "MESH",
    .EnvName = "MESH_DBG_LEVELS",
};

/**
 * @brief React to a signal to shut down the daemon by marking the event
 *        event loop as terminated.
 */
static void meshAppShutdownSignalHandler(int signal)
{
    evloopAbort();
}

/**
 * @brief Function to invoke the cfg call to get version
 *        compatibility check with UMAC SON driver module
 */
void meshGetVersionCompatibilityCheck(char *ifname, char *ver_str) {
    int ret = -1;
    char *umac_driv_msg = "UMAC Driver Version Compatibility check";

    fprintf(stdout, "MAD Application version string [%s]\n", ver_str);

    /* Peform UMAC driver version compatibility */
    ret = getVersionCompatibility(mesh_cfg80211_ctx, ifname, ver_str, strlen(ver_str));

    if (ret < 0) {
        fprintf(stderr, "Unable to get UMAC driver Version Compatibility check ifname[%s]!!!\n", ifname);
    } else if (ret == 0) {
        fprintf(stdout, "%s - PASSED !!!\n", umac_driv_msg);
    } else if (ret == 1) {
        fprintf(stdout, "%s - FAILED !!!\n", umac_driv_msg);
    } else {
        fprintf(stderr, "%s - Unknown return status [%s]!!!\n", umac_driv_msg, ifname);
    }
}

/**
 * @brief Function to get interface name
 *        and call the version compatibility check API
 */
void meshVersionCompatibilityCheck()
{
    char *ifname = NULL;
    if (deFindFirstWLANInterface(&ifname) > 0) {
        fprintf(stderr, "%s: WLAN interface not found\n", __func__);
        return;
    }

    /* get version compatibility check for both SON and EasyMesh */
    if (ifname) {
        fprintf(stdout, "%s: WLAN Interface found [%s]\n", __func__, ifname);
        meshGetVersionCompatibilityCheck(ifname, MAD_SON_VERSION);
        meshGetVersionCompatibilityCheck(ifname, MAD_MAP_VERSION);
        free(ifname);
    }
}

/**
 * @brief Wlan socket initialization
 */
int meshAppWlanInit(void)
{
    int ret = -1;
    mesh_cfg80211_ctx = malloc(sizeof(wifi_cfg80211_context));
    if (!mesh_cfg80211_ctx) {
        dbgf(dbgModule, DBGERR, "%s: Memmory allocation failed\n",__func__);
        return MESH_NOK;
    }
    ret = wifi_init(mesh_cfg80211_ctx);
    if (ret != 0) {
        dbgf(dbgModule, DBGERR, "%s: Socket creation failed \n",__func__);
        return MESH_NOK;
    }
    return MESH_OK;
}

/**
 * @brief Init Mesh Application
 */
int meshAppInits(void)
{
    int meshSockfd = 0;

    /* Wlan Socket Initialization */
    if ( meshAppWlanInit() == MESH_NOK ) {
        dbgf(dbgModule, DBGERR, "%s: WLAN init failed !!! \n",__func__);
    }

    meshVersionCompatibilityCheck();

    meshSockfd = meshEventInit();
    if (meshSockfd == MESH_NOK) {
        dbgf(dbgModule, DBGERR, "%s: mesh api socket create failed\n", __func__);
        return MESH_NOK;
    }
    dbgf(dbgModule, DBGDEBUG, "%s: mesh api socket created\n", __func__);

    if ( meshAppS.Mode & IEEE1905_MODE ) {
        dbgf(dbgModule, DBGINFO, "%s: IEEE1905 Init called\n", __func__);
        meshIeee1905Init();
    }
    if ( mesh_cfg80211_ctx != NULL) {
        if (meshAppS.Mode & DE_MODE) {
            dbgf(dbgModule, DBGINFO, "%s: DataElements Init called\n", __func__);
            if (dataElementsInit(mesh_cfg80211_ctx) == DE_NOK) {
                dbgf(dbgModule, DBGINFO, "%s: DataElements Init Failed !!!\n", __func__);
            }
        } else {
            wifi_deinit(mesh_cfg80211_ctx);
        }
    }

    return MESH_OK;
}

/**
 * @brief Run Mesh Application forever
 */
void meshAppRun(void)
{
    evloopRunPrepare();

    /* Run forever */
    evloopRun();
}

/**
 * @brief Gracefully stop Mesh Application
 */
void meshAppFini(void)
{
    meshEventFini();
    dataElementsWlanFini();
}

/**
 * @brief Print Mesh Application Version
 */
static void meshAppVersion(void)
{
    fprintf(stderr, "%s\n", MESH_API_VERSION_STR);
    exit(1);
}

/**
 * @brief Mesh Application Usage
 */
void meshAppUsage()
{
    dbgf(dbgModule, DBGINFO, "Usage: mesh_app [-dhv] [-m mode] [-P Debug Output Port]");
    dbgf(dbgModule, DBGINFO, "       -d: Do Daemonize.");
    dbgf(dbgModule, DBGINFO, "       -m: Specify mode of operation.");
    dbgf(dbgModule, DBGINFO, "       -h: Show usage.");
    dbgf(dbgModule, DBGINFO, "       -v: Show version information.");
    dbgf(dbgModule, DBGINFO, "       -P: Debug Output Port.");
    exit(1);
}

/**
 * @brief Parse cmd line arguments
 */
void meshAppParseArgs(char **argv)
{
    char *Arg;

    argv++;     /* skip program name */
    while ((Arg = *argv++) != NULL) {
        if (!strcmp(Arg, "-d")) {
            meshAppS.DoDaemonize = 1;
        } else
        if (!strcmp(Arg, "-m")) { /* configuration file */
            Arg = *argv++;
            if (Arg == NULL)
                meshAppUsage();
            meshAppS.Mode = atoi(Arg);
        } else
        if (!strcmp(Arg, "-h")) { /* show usage */
            meshAppUsage();
        } else
        if (!strcmp(Arg, "-v")) { /* show Mesh App version */
            meshAppVersion();
        } else
        if (!strcmp(Arg, "-P")) { /* Debug output port */
            Arg = *argv++;
            if (Arg == NULL)
                meshAppUsage();
            meshAppS.Port = atoi(Arg);
        } else {
            dbgf(dbgModule ,DBGERR, "INVALID ARG: %s", Arg);
            meshAppUsage();
        }
    }
    return;
}

/**
 * The main function of Mesh Application
 */
int main(int argc, char **argv)
{
    meshAppS.Port = MESH_API_PORT;
    meshAppS.DoDaemonize = 0;

    /* Set Debug Options */
    dbgInit1(&meshAppDbgInitOptions);

    /* Register for debug messages from this file */
    dbgModule = dbgModuleFind("meshApp");
    meshAppS.DebugModule = dbgModule;
    meshAppS.DebugModule->Level = DBGINFO;

    meshAppParseArgs(argv);

    if (meshAppS.DoDaemonize) {
        if (daemon(0,0)) {
            perror("daemon");
            exit(1);
        }
    }

    dbgf(dbgModule, DBGDEBUG, "%s : %d start Mesh API Layer\n", __func__, __LINE__);

    /* Add debugging shell capability */
    cshInit(meshAppS.Port);

    if (meshAppInits() == MESH_NOK) {
        dbgf(dbgModule, DBGDEBUG, "%s: Mesh App Inits failed", __func__);
        return 0;
    }

    /* Register signal handlers for an orderly shutdown. */
    signal(SIGINT, meshAppShutdownSignalHandler);
    signal(SIGTERM, meshAppShutdownSignalHandler);

    /* Main event loop waits for things to happen */
    dbgf(dbgModule, DBGDEBUG, "%s: Entering evloop", __func__);
    meshAppRun();

    printf("Exiting mesh App\n");
    meshAppFini();

    return 0;
}
