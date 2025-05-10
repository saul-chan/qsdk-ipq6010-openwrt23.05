/*
 * @File: package.h
 *
 * Copyright (c) 2023 Qualcomm Technologies, Inc.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#ifndef _PACKAGE_H_
#define _PACKAGE_H_

#define UCI_MAX_STR_LEN 256

struct package_s{
    char *name;
    int (*init)(struct package_s *pkg);
    int (*set)(struct package_s *pkg, char *name, char *value);
    int (*get)(struct package_s *pkg, char *name, char *value);
    int (*prepare)(struct package_s *pkg, char *name, char *value);
    int (*apply)(struct package_s *pkg);
    int (*destroy)(struct package_s *pkg);
    char *flt1;
    char *flt2;
    char *flt3;
    void *priv;
    struct package_s *next;
    int (*ifaction)(struct package_s *pkg, const char *name, const char *value);
};

struct pkgmgr_s{
    struct package_s *head;
    int isInit;
    void *cookie;
    int semid;
};

int pkgInit(struct pkgmgr_s *pkgm);
int pkgSet(struct pkgmgr_s *pkgm, char *name, char *value);
int pkgPrepare(struct pkgmgr_s *pkgm, char *name, char *value);
int pkgApply(struct pkgmgr_s *pkgm);
int pkgDestroy(struct pkgmgr_s *pkgm);
int pkgRegister(struct package_s *pkg);
struct pkgmgr_s *pkgGetPkgmgr();
int pkgIfAction(struct pkgmgr_s *pkgm, const char *name, const char *value);
int wlanif_init(void);
void wlanif_deinit(void);

#ifdef SON_MEMORY_DEBUG
#include "qca-son-mem-debug.h"
#define QCA_MOD_INPUT QCA_MOD_LIBSTORAGE
#include "son-mem-debug.h"

#endif /* SON_MEMORY_DEBUG */

#endif
