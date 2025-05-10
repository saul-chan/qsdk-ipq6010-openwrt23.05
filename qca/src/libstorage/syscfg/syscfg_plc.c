/*
 * @File: syscfg_plc.c
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

struct plc_s{
    int changed;
};

static int plc_init(struct package_s *pkg)
{
    struct plc_s *plc = (struct plc_s *)pkg->priv;
    plc->changed = 0;
    return 0;
}

static int plc_set_nmk(struct package_s *pkg, char *value)
{
    char nmk[UCI_MAX_STR_LEN];
    char selected[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];
    struct plc_s *plc = (struct plc_s *)pkg->priv;
    int ret = 0;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "%s.config.Nmk", pkg->name);
    ret = syscfg_get(NULL, optv, nmk, sizeof(nmk));

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "%s.config.NmkSelected", pkg->name);
    ret |= syscfg_get(NULL, optv, selected, sizeof(selected));

    if ( ret
        || strcmp(value, nmk)
        || strcmp("true", selected))
    {
        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "%s.config.Nmk", pkg->name);
	ret = syscfg_set(NULL, optv, value);

        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "%s.config.NmkSelected", pkg->name);
	ret |= syscfg_set(NULL, optv, "true");

        plc->changed++;
    }

    return ret;
}

static int plc_set(struct package_s *pkg, char *name, char *value)
{
    char option[UCI_MAX_STR_LEN];
    char ucival[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];
    struct plc_s *plc = (struct plc_s *)pkg->priv;
    int ret = 0;

    name += strlen(pkg->flt1);

    if (strcasecmp(name, "NMK") == 0)
        return plc_set_nmk(pkg, value);

    memset(optv, '\0', sizeof(optv));
    snprintf(option, sizeof(option), "config.%s", name);
    snprintf(optv, sizeof(optv), "%s.%s", pkg->name, option);

    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if (ret
        || strcmp(value, ucival))
    {
	ret = syscfg_set(NULL, optv, value);
        plc->changed++;
    }

    return ret;
}

static int plc_apply(struct package_s *pkg)
{
    struct plc_s *plc = (struct plc_s *)pkg->priv;
    int ret = 0;

    if (!plc->changed)
    {
        return 0;
    }

    syscfg_commit();
    // TODO: To check for RDKB-B
    //system("/etc/init.d/plc reload");
    plc->changed = 0;

    return ret;
}

static struct package_s plcPkg =
{
    "plc",              /*name*/
    plc_init,           /*init*/
    plc_set,            /*set*/
    NULL,               /*get*/
    NULL,               /*prepare*/
    plc_apply,          /*apply*/
    NULL         ,      /*destroy*/
    "PLC.",             /*flt1*/
    NULL,               /*flt2*/
    NULL,               /*flt3*/
    NULL,               /*priv*/
    NULL,               /*next*/
    NULL                /*ifaction*/
};

int plc_register()
{
    struct plc_s *plc = malloc(sizeof(struct plc_s));

    if (! plc)
        return -1;

    memset(plc, 0, sizeof(struct plc_s));

    plcPkg.priv = (void *)plc;

    pkgRegister(&plcPkg);

    return 0;
}
