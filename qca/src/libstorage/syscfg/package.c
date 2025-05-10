/*
 * @File: package.c
 *
 * Copyright (c) 2023 Qualcomm Technologies, Inc.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#include "wlanif_cmn.h"
#include "package.h"
#include "../storage.h"
#include "syscfg/syscfg.h"

#define DEBUG_PKG
#ifdef DEBUG_PKG
 #define DEBUG(args...) printf(args)
#else
 #define DEBUG(args...)
#endif

static struct pkgmgr_s pkgMgr = {NULL, 0, NULL, 0};
struct wlanif_config *wlanIfLS=NULL;

static int pkgLockInit(struct pkgmgr_s *pkgm)
{
    int semid;
    key_t key;
    union semun{
        int val;
    } semarg;

    if (pkgm->semid)
        return 0;

    key = ftok( "/proc/version", 127);
    if (key == -1)
        return -1;

    semid = semget(key, 0, 0);
    if (semid == -1)
    {
        semid = semget(key, 1, IPC_CREAT|IPC_EXCL|0666);
        if (semid == -1)
            return -1;

        semarg.val = 1;
        if (semctl(semid, 0, SETVAL, semarg) == -1)
        {
            semctl(semid, 0, IPC_RMID, semarg);
            return -1;
        }
    }

    pkgm->semid = semid;
    return 0;
}

static int pkgLock(struct pkgmgr_s *pkgm)
{
    struct sembuf sem;

    if (pkgLockInit(pkgm) == -1)
        return -1;

    sem.sem_num  = 0;
    sem.sem_op   = -1;
    sem.sem_flg  = SEM_UNDO;

    if (semop(pkgm->semid, &sem, 1) == -1)
    {
        return -1;
    }

    return 0;
}

static int pkgUnlock(struct pkgmgr_s *pkgm)
{
    struct sembuf sem;

    if (pkgLockInit(pkgm) == -1)
        return -1;

    sem.sem_num  = 0;
    sem.sem_op   = 1;
    sem.sem_flg  = SEM_UNDO;

    if (semop(pkgm->semid, &sem, 1) == -1)
    {
        return -1;
    }

    return 0;
}

int pkgInit(struct pkgmgr_s *pkgm)
{
    int ret = 0;
    struct package_s *pkg = pkgm->head;

    DEBUG("Package Init\n");

    while(pkg)
    {
        if (pkg->init
            && pkg->init(pkg))
        {
            ret = -1;
            printf("libstorage: package[%s] initial failed\n", pkg->name);
        }
        pkg = pkg->next;
    }
    return ret;
}

int pkgPrepare(struct pkgmgr_s *pkgm, char *name, char *value)
{
    int ret = 0;
    struct package_s *pkg = pkgm->head;

    while(pkg)
    {
        if (strcmp ("wireless", pkg->name) == 0) {
            ret = pkg->prepare(pkg, name, value);
            break;
        }
        pkg = pkg->next;
    }

    if (!pkg)
    {
        printf("libstorage: unhandled optoin %s:%s\n", name, value);
    }
    else if (ret)
    {
        printf("libstorage: package[%s] set %s:%s failed\n", pkg->name, name, value);
        return -1;
    }

    return ret;
}

int pkgSet(struct pkgmgr_s *pkgm, char *name, char *value)
{
    int ret = 0;
    struct package_s *pkg = pkgm->head;

    while(pkg)
    {
        if ( (pkg->flt1 && memcmp(pkg->flt1, name, strlen(pkg->flt1)) == 0)
            || (pkg->flt2 && memcmp(pkg->flt2, name, strlen(pkg->flt2)) == 0)
            || (pkg->flt3 && memcmp(pkg->flt3, name, strlen(pkg->flt3)) == 0))
        {
            ret = pkg->set(pkg, name, value);
            break;
        }
        pkg = pkg->next;
    }

    if (!pkg)
    {
        printf("libstorage: unhandled optoin %s:%s\n", name, value);
    }
    else if (ret)
    {
        printf("libstorage: package[%s] set %s:%s failed\n", pkg->name, name, value);
        return -1;
    }

    return ret;
}

int pkgApply(struct pkgmgr_s *pkgm)
{
    int ret = 0;
    struct package_s *pkg = pkgm->head;

    /*lock to prevent parellel apply*/
    if (pkgLock(pkgm))
        DEBUG("Package lock failed\n");

    while(pkg)
    {
        if (pkg->apply
            && pkg->apply(pkg))
        {
            ret = -1;
            printf("libstorage: package[%s] apply failed\n", pkg->name);
        }
        pkg = pkg->next;
    }
    /*unlock*/
    if (pkgUnlock(pkgm))
        DEBUG("Package unlock failed\n");

    return ret;
}

int pkgIfAction(struct pkgmgr_s *pkgm, const char *name, const char *value)
{
    int ret = -1;
    struct package_s *pkg = pkgm->head;
    while(pkg)
    {
        if ((pkg->flt1 && memcmp(pkg->flt1, name, strlen(pkg->flt1)) == 0)
             || (pkg->flt2 && memcmp(pkg->flt2, name, strlen(pkg->flt2)) == 0)
             || (pkg->flt3 && memcmp(pkg->flt3, name, strlen(pkg->flt3)) == 0))
        {
            ret = pkg->ifaction(pkg, name, value);
            break;
        }

        pkg = pkg->next;
    }

    if (!pkg)
    {
        DEBUG("%s libstorage: unhandled option %s:%s\n",__func__, name, value);
    }
    else if (ret < 0 )
    {
        DEBUG("%s libstorage: package[%s] set %s:%s failed\n", __func__, pkg->name, name, value);
    }

    return ret;
}

int pkgDestroy(struct pkgmgr_s *pkgm)
{
    int ret = 0;
    struct package_s *pkg = pkgm->head;

    while(pkg)
    {
        if (pkg->destroy
            &&  pkg->destroy(pkg))
        {
            ret = -1;
            printf("libstorage: package[%s] destroy failed\n", pkg->name);
        }
        pkg = pkg->next;
    }

    if (pkgm->cookie) { // ucidestory is done and then null.
        pkgm->cookie = NULL;
    }
    return ret;
}

int pkgRegister(struct package_s *pkg)
{
    struct package_s *ipkg = pkgMgr.head;

    while(ipkg)
    {
        if (strcmp (ipkg->name, pkg->name) == 0)
        {
            return 0;
        }

        ipkg = ipkg->next;
    }

    pkg->next = pkgMgr.head;
    pkgMgr.head = pkg;
    return 0;
}

struct pkgmgr_s *pkgGetPkgmgr()
{
    if (pkgMgr.isInit)
        return &pkgMgr;

    extern int wsplc_register();
    wsplc_register();

    extern int hyfi_register();
    hyfi_register();

    extern int wireless_register();
    wireless_register();

    extern int plc_register();
    plc_register();

    pkgMgr.isInit = 1;
    return &pkgMgr;
}

int wlanif_init()
{
    char ucival[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];
    int ret;

    memset(optv, '\0', sizeof(optv));

    snprintf(optv, sizeof(optv), "qcacfg80211.config.enable");
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if (ret) {
        return -1;
    }

    if (strcmp(ucival,"0"))
    {
        wlanIfLS = wlanif_config_init(LIBSTORAGE_NL80211_CMD_SOCK,
                LIBSTORAGE_NL80211_EVENT_SOCK);
    }

    if (!wlanIfLS )
    {
        printf("%s: wlanIfLS init failed \n",__func__);
        return -1;
    }
    return 0;
}

void wlanif_deinit()
{
    if(wlanIfLS) {
        wlanif_config_deinit(wlanIfLS);
        wlanIfLS = NULL;
    }
}
