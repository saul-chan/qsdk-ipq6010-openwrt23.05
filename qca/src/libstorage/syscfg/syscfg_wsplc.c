/*
 * @File: syscfg_wsplc.c
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
#include "../storage.h"
#include "syscfg/syscfg.h"

enum {
    WSPLCRUNMODE_REGISTRAR=1,
    WSPLCRUNMODE_ENROLLEE,
};

enum {
    WSPLCCONFIGURESTA_YES=1,
    WSPLCCONFIGURESTA_NO,
};

struct wsplc_s{
    int changed;
};

struct ws_translate_option{
    char *optname;
    int (*opt2uci)(char *optval, char *ucival);
    int (*optparser)(struct wsplc_s *ws, char *name, char *value);
};

static int opt_runmode(char *optval, char *ucival)
{
    int runmode;

    if (!optval || !ucival)
        return -1;

    if (isdigit(*optval))
    {
        runmode = atoi(optval);
        if (runmode == WSPLCRUNMODE_REGISTRAR)
            snprintf(ucival, UCI_MAX_STR_LEN, "REGISTRAR");
        else if (runmode == WSPLCRUNMODE_ENROLLEE)
            snprintf(ucival, UCI_MAX_STR_LEN, "ENROLLEE");
        else
            snprintf(ucival, UCI_MAX_STR_LEN, "NONE");
    }
    else
        snprintf(ucival, UCI_MAX_STR_LEN,"%s", optval);

    return 0;
}

static int opt_parse_responsercvd(struct wsplc_s *ws, char *name, char *value)
{
    char ucival[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];
    int ret;
    
    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wsplcd.config.%s", name);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));
    if (ret || strcmp(value, ucival))
    {
	ret = syscfg_set(NULL, optv, value);
        ws->changed = 0;
    }
    syscfg_commit();
    return ret;
}

static int opt_configsta(char *optval, char *ucival)
{
    int runmode;

    if (!optval || !ucival) {
        return -1;
    }

    if (isdigit(*optval))
    {
        runmode = atoi(optval);
        if (runmode == WSPLCCONFIGURESTA_YES)
            snprintf(ucival, UCI_MAX_STR_LEN, "1");
        else if (runmode == WSPLCCONFIGURESTA_NO)
            snprintf(ucival, UCI_MAX_STR_LEN, "0");
    }
    else
        snprintf(ucival, UCI_MAX_STR_LEN, "%s", optval);

    return 0;
}

static struct ws_translate_option wsTbl[] =
{
    { "RunMode",        opt_runmode,       NULL},
    { "ConfigSta",      opt_configsta,     NULL},
    { "ResponseRcvd",   NULL,              opt_parse_responsercvd},
    { NULL }
};

static int wsplc_init(struct package_s *pkg)
{
    struct wsplc_s *ws = (struct wsplc_s *)pkg->priv;
    ws->changed = 0;
    return 0;
}

static int wsplc_set(struct package_s *pkg, char *name, char *value)
{
    char ucival[UCI_MAX_STR_LEN];
    char newval[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];
    struct wsplc_s *ws = (struct wsplc_s *)pkg->priv;
    struct ws_translate_option *iopt;
    int ret = 0;

    name += strlen(pkg->flt1);

    iopt = wsTbl;
    while(iopt->optname != NULL)
    {
        if (strcmp(name, iopt->optname) == 0)
            break;
        iopt ++;
    }

    if (iopt && iopt->optparser &&
            iopt->optparser(ws, name, value) != 0)
        return -1;

    if (iopt && iopt->opt2uci)
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
        ws->changed++;
    }
    syscfg_commit();
    return ret;
}

static int wsplc_apply(struct package_s *pkg)
{
    struct wsplc_s *ws = (struct wsplc_s *)pkg->priv;
    int ret = 0;

    syscfg_commit();
    if (ws->changed){
	printf("\n wsplcd restart");
	system("/etc/initscripts/start_wsplcd restart");
    }
    return ret;
}

static struct package_s wsplcPkg =
{
    "wsplcd",          /*name*/
    wsplc_init,        /*init*/
    wsplc_set,         /*set*/
    NULL,              /*get*/
    NULL,              /*prepare*/
    wsplc_apply,       /*apply*/
    NULL         ,     /*destroy*/
    "WSPLC.",          /*flt1*/
    NULL,              /*flt2*/
    NULL,              /*flt3*/
    NULL,              /*priv*/
    NULL,              /*next*/
    NULL               /*ifaction*/
};

int wsplc_register()
{
    struct wsplc_s *ws = malloc(sizeof(struct wsplc_s));

    if (! ws)
        return -1;

    memset(ws, 0, sizeof(struct wsplc_s));

    wsplcPkg.priv = (void *)ws;

    pkgRegister(&wsplcPkg);

    return 0;
}
