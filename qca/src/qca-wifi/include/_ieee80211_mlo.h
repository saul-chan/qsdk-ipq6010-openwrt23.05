/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

/**
 * DOC: This file contains definitions required for software functionality
 * specific to IEEE 802.11be Multi-Link Operation (MLO).
 */
#ifndef __IEEE80211_MLO_H_
#define __IEEE80211_MLO_H_

#ifdef WLAN_FEATURE_11BE_MLO
#include <qdf_types.h>
#include <qdf_trace.h>

#define mloie_alert(format, args...) \
    QDF_TRACE_FATAL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_err(format, args...) \
    QDF_TRACE_ERROR(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_warn(format, args...) \
    QDF_TRACE_WARN(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_info(format, args...) \
    QDF_TRACE_INFO(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_debug(format, args...) \
    QDF_TRACE_DEBUG(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_alert(format, args...) \
    QDF_TRACE_FATAL_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_err(format, args...) \
    QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_warn(format, args...) \
    QDF_TRACE_WARN_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_info(format, args...) \
    QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_debug(format, args...) \
    QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_alert_rl(format, args...) \
    QDF_TRACE_FATAL_RL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_err_rl(format, args...) \
    QDF_TRACE_ERROR_RL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_warn_rl(format, args...) \
    QDF_TRACE_WARN_RL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_info_rl(format, args...) \
    QDF_TRACE_INFO_RL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_debug_rl(format, args...) \
    QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_alert_rl(format, args...) \
    QDF_TRACE_FATAL_RL_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_err_rl(format, args...) \
    QDF_TRACE_ERROR_RL_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_warn_rl(format, args...) \
    QDF_TRACE_WARN_RL_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_info_rl(format, args...) \
    QDF_TRACE_INFO_RL_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#define mloie_nofl_debug_rl(format, args...) \
    QDF_TRACE_DEBUG_RL_NO_FL(QDF_MODULE_ID_MLOIE, format, ## args)

#endif /* WLAN_FEATURE_11BE_MLO */
#endif /* __IEEE80211_MLO_H_ */
