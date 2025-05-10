/*
 * @File: syscfg_hyfi.c
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
#include "package.h"
#include "syscfg/syscfg.h"

enum {
    HYBRIDMODE_HYROUTER=1,
    HYBRIDMODE_HYCLIENT,
    HYBRIDMODE_EXTENDER,
};

struct hyfi_s{
    int changed;
};

struct hy_translate_option{
    char *optname;
    int (*opt2uci)(char *optval, char *ucival);
};

static int opt_hybridmode(char *optval, char *ucival)
{
    int runmode;

    if (!optval || !ucival)
        return -1;

    if (isdigit(*optval))
    {
        runmode = atoi(optval);
        if (runmode == HYBRIDMODE_HYROUTER)
            snprintf(ucival, UCI_MAX_STR_LEN, "HYROUTER");
        else if (runmode == HYBRIDMODE_HYCLIENT)
            snprintf(ucival, UCI_MAX_STR_LEN, "HYCLIENT");
        else if (runmode == HYBRIDMODE_EXTENDER)
            snprintf(ucival, UCI_MAX_STR_LEN, "HYEXTENDER");
        else
            return -1;
    }
    else
    {
        snprintf(ucival, UCI_MAX_STR_LEN, " %s", optval);
    }
    return 0;
}

static struct hy_translate_option hyTbl[] =
{
    { "Mode",  opt_hybridmode},
    { NULL}
};

static int hyfi_init(struct package_s *pkg)
{
    struct hyfi_s *hy = (struct hyfi_s *)pkg->priv;
    hy->changed = 0;
    return 0;
}

static int hyfi_set(struct package_s *pkg, char *name, char *value)
{
    char ucival[UCI_MAX_STR_LEN];
    char newval[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];
    struct hyfi_s *hy = (struct hyfi_s *)pkg->priv;
    struct hy_translate_option *iopt;
    int ret = 0;

    name += strlen(pkg->flt1);

    iopt = hyTbl;
    while(iopt->optname != NULL)
    {
        if (strcmp(name, iopt->optname) == 0)
            break;
        iopt ++;
    }

    if (iopt->optname && iopt->opt2uci)
    {
        if (iopt->opt2uci(value, newval) != 0)
            return -1;
        value = newval;
    }

    memset(optv, '\0', sizeof(optv));

    snprintf(optv, sizeof(optv), "%s.config.%s", pkg->name, name);

    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if (ret
        || strcmp(value, ucival))
    {
        ret = syscfg_set(NULL, optv, value);
        hy->changed++;
    }
    syscfg_commit();
    return ret;
}

static int hyfi_apply(struct package_s *pkg)
{
    struct hyfi_s *hy = (struct hyfi_s *)pkg->priv;
    int ret = 0;

    if (!hy->changed)
        return 0;

    syscfg_commit();
    //TODO: Need to check for RDK-B
    //system("/etc/init.d/hyd restart");
    return ret;
}

static struct package_s hyfiPkg =
{
    "hyd",              /*name*/
    hyfi_init,          /*init*/
    hyfi_set,           /*set*/
    NULL,               /*get*/
    NULL,               /*prepare*/
    hyfi_apply,         /*apply*/
    NULL         ,      /*destroy*/
    "HYFI.",            /*flt1*/
    NULL,               /*flt2*/
    NULL,               /*flt3*/
    NULL,               /*priv*/
    NULL,               /*next*/
    NULL                /*ifaction*/
};

int hyfi_register()
{
    struct hyfi_s *hy = malloc(sizeof(struct hyfi_s));

    if (! hy)
        return -1;

    memset(hy, 0, sizeof(struct hyfi_s));

    hyfiPkg.priv = (void *)hy;

    pkgRegister(&hyfiPkg);

    return 0;
}
