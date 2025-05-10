/*
* Copyright (c) 2018 Qualcomm Innovation Center, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Innovation Center, Inc.
*
*/

/*
 *  Copyright (c) 2011 Atheros Communications Inc.
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

#ifndef _IEEE80211_QUIET_H
#define _IEEE80211_QUIET_H

#if UMAC_SUPPORT_QUIET

#ifdef WLAN_FEATURE_11BE_MLO
/* For 11be MLO, max Quiet count to be used is 127. Quiet count greater than 127
 * indicates the negative Quiet count. Ex: Quiet count 129 indicates that the
 * quiet interval started in the beacon interval that occurred 2 TBTTs in the
 * past.
 */
#define WLAN_QUIET_PERIOD_DEF       127
#define WLAN_QUIET_PERIOD_CONFIG_CHANGE       5
#else
#define WLAN_QUIET_PERIOD_DEF       200
#endif /* WLAN_FEATURE_11BE_MLO */
#define WLAN_QUIET_DURATION_DEF     30
#define WLAN_QUIET_OFFSET_DEF       20

/*
 * START_STOP flag value: 1 - Start, 0 - Stop
 */
#define WLAN_QUIET_FLAG_START_STOP              0x00000001
/*
 * ONE_SHOT flag value: 1 - One shot, 0 - Repeat
 * This flag is only relevant if the START_STOP flag == 1 (start).
 */
#define WLAN_QUIET_FLAG_ONE_SHOT                0x00000002
/*
 * Enable/Disable sending Quiet IE info in SWBA event from the target
 * 0 - Don't include Quiet IE in WMI SWBA Event
 * 1 - Include Quiet IE in WMI SWBA Event
 */
#define WLAN_QUIET_FLAG_INFO_IN_SWBA_START_STOP 0x00000004

struct ieee80211_quiet_param {
    u_int8_t    is_enabled;         /* flag to enable quiet period IE support */
    u_int8_t    tbttcount;          /* quiet start */
    u_int8_t    period;             /* beacon intervals between quiets*/
    u_int16_t   duration;           /* TUs of each quiet*/
    u_int16_t   offset;             /* TUs of from TBTT of quiet start*/
    u_int16_t   beacon_offset;      /* beacon_offset */
    u_int64_t   quiet_completed_time; /* save the time when quiet count is 0*/
};


/*
 * External Definitions
 */
int ieee80211_quiet_attach(struct ieee80211com *ic);
int ieee80211_quiet_detach(struct ieee80211com *ic);
int ieee80211_quiet_vattach(struct ieee80211vap *vap);
int ieee80211_quiet_vdetach(struct ieee80211vap *vap);
#else

struct ieee80211_quiet_param;

#define ieee80211_quiet_attach(ic)       /**/
#define ieee80211_quiet_detach(ic)       /**/
#define ieee80211_quiet_vattach(vap)     /**/
#define ieee80211_quiet_vdetach(vap)     /**/

#endif /* UMAC_SUPPORT_QUIET */

#endif /* _IEEE80211_QUIET_H */
