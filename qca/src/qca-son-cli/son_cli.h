/* @File:   son_cli.h
 *
 * @Notes:   *
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#ifndef SON_CLI_H
#define SON_CLI_H
#define IFNAME_LEN 32
#define SON_CLI_VERSION "1.0.0"

#ifdef SON_MEMORY_DEBUG
typedef struct user_input_data {
    int memdbg_cli_port;
    int memdbg_report_interval;
    int memdbg_repeat_count;
}user_input_data_t;
#endif

#endif
