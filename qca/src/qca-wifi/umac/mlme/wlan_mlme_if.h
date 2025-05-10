/*
 * Copyright (c) 2011-2014, 2017-2021,2023-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2014 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#ifndef _WLAN_MLME_IF_H
#define _WLAN_MLME_IF_H

#include <qdf_types.h>
#include <qdf_trace.h>
#include <ieee80211_mlme_priv.h>
#include <ieee80211_objmgr_priv.h>
#include <wlan_serialization_api.h>
#include <wlan_cm_public_struct.h>
#include <wlan_cm_api.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include "wlan_mlo_mgr_sta.h"
#endif
#include <cfg80211_external.h>
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
#include <telemetry_agent_wifi_driver_if.h>
#endif

#define wlan_mlme_err(format, args...) \
    QDF_TRACE_ERROR(QDF_MODULE_ID_MLME, format, ## args)
#define wlan_mlme_info(format, args...) \
    QDF_TRACE_INFO(QDF_MODULE_ID_MLME, format, ## args)
#define wlan_mlme_debug(format, args...) \
    QDF_TRACE_DEBUG(QDF_MODULE_ID_MLME, format, ## args)

#define wlan_mlme_nofl_err(format, args...) \
    QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_MLME, format, ## args)
#define wlan_mlme_nofl_info(format, args...) \
    QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_MLME, format, ## args)
#define wlan_mlme_nofl_debug(format, args...) \
    QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_MLME, format, ## args)

/* Serialization command timeout in milliseconds */
#ifdef QCA_WIFI_QCA5332
#define WLAN_MLME_SER_CMD_TIMEOUT_MS (65000*3)
#else
#define WLAN_MLME_SER_CMD_TIMEOUT_MS (65000)
#endif
#define MLME_SER_RESTART_CMD_TIMEOUT_MS 90000

#define WLAN_SERIALZATION_CANCEL_WAIT_ITERATIONS 1000
#define WLAN_SERIALZATION_CANCEL_WAIT_TIME 100

/*
 * enum wlan_mlme_notify_mod - Call the notification cb
 * @WLAN_MLME_NOTIFY_NONE - No handlers
 * @WLAN_MLME_NOTIFY_MLME - Notify mlme layer
 * @WLAN_MLME_NOTIFY_OSIF - Notify osif layer
 */
enum wlan_mlme_notify_mod {
    WLAN_MLME_NOTIFY_NONE,
    WLAN_MLME_NOTIFY_MLME,
    WLAN_MLME_NOTIFY_OSIF,
};

/*
 * enum mlme_cmd_activation_ctx - Activation context of cmd
 * @MLME_CTX_DIRECT - Activated from caller context
 * @MLME_CTX_SCHED - Activated in scheduler context
 */
enum mlme_cmd_activation_ctx {
    MLME_CTX_DIRECT,
    MLME_CTX_SCHED,
};

/*
 * struct wlan_mlme_ser_data - Data in serialization command
 * @vdev: VDEV object associated to the command
 * @flags: Store restart or start status flag
 * @notify_osif: On completion of cmd execution indicate whether
 *  the post processing handlers to be called
 * @cmd_in_sched: Mark if a cmd is activated in scheduler context
 * @activation_ctx: Denote the context in which the cmd was activated
 */
struct wlan_mlme_ser_data {
    struct wlan_objmgr_vdev *vdev;
    int32_t flags;
    uint8_t notify_osif;
    uint8_t cmd_in_sched;
    uint8_t activation_ctx;
};

/*
 * struct wlan_mlme_sched_data - Scheduler context execution
 * @vdev: Objmgr vdev object
 * @cmd_type: Command type to be processed
 * @notify_status: Whether to notify MLME and OSIF
 */
struct wlan_mlme_sched_data {
    struct wlan_objmgr_vdev *vdev;
    enum wlan_serialization_cmd_type cmd_type;
    uint8_t notify_status;
};

/*
 * osif_get_num_active_vaps(): Get number of active vaps
 * @comhandle: legacy ic handle
 *
 * Return: number of active vaps
 */
u_int32_t osif_get_num_active_vaps(wlan_dev_t comhandle);

/*
 * osif_get_num_running_vaps(): Get number of running vaps
 * @comhandle: legacy ic handle
 *
 * Running vaps include vaps with tx_rx capability
 * Return: number of active vaps
 */
u_int16_t osif_get_num_running_vaps(wlan_dev_t comhandle);

/*
 * osif_ht40_event_handler(): Event handler
 * vdev: vdev object
 * @arg: Channel configuration from the event notifier
 *
 * Return: void
 */
void osif_ht40_event_handler(struct wlan_objmgr_vdev *vdev, void *arg);

/*
 * osif_acs_event_handler(): ACS Event handler
 * vdev: vdev object
 * @arg: Channel configuration from the event notifier
 *
 * Return: void
 */
void osif_acs_event_handler(struct wlan_objmgr_vdev *vdev, void *arg);

/*
 * osif_mlme_notify_handler(): Cmd post processing in OSIF layer
 * @vap: legacy vap handle
 * @cmd_type: Serialization cmd type for which the handler is called
 *
 * Return: void
 */
void osif_mlme_notify_handler(wlan_if_t vap,
        enum wlan_serialization_cmd_type cmd_type);

/*
 * wlan_mlme_vdev_cmd_handler(): Cmd post processing handler in mlme layer
 * @vdev: Objmgr vdev information
 * @cmd_type: Serialization cmd type for which the handler is called
 *
 * Return: void
 */
void
wlan_mlme_vdev_cmd_handler(struct wlan_objmgr_vdev *vdev,
                           enum wlan_serialization_cmd_type cmd_type);

/*
 * mlme_ser_proc_vdev_start(): Callback called for command activation from
 * serialization
 * @cmd: serialization command information
 * @reason: Callback reason on which the cb function should operate
 *
 * Return: status of command activation after processing
 */
QDF_STATUS
mlme_ser_proc_vdev_start(struct wlan_serialization_command *cmd,
                            enum wlan_serialization_cb_reason reason);

/*
 * mlme_ser_proc_vdev_stop(): Callback called for command activation from
 * serialization
 * @cmd: serialization command information
 * @reason: Callback reason on which the cb function should operate
 *
 * Return: status of command activation after processing
 */
QDF_STATUS
mlme_ser_proc_vdev_stop(struct wlan_serialization_command *cmd,
                           enum wlan_serialization_cb_reason reason);

/*
 * wlan_mlme_release_vdev_req() - Release a cmd from the serialization queue
 * @vdev: vdev object component
 * @cmd_type: serialization command type
 * @status: return value from the command execution
 *
 * Return: Success on command removal, else error value
 */
QDF_STATUS
wlan_mlme_release_vdev_req(struct wlan_objmgr_vdev *vdev,
                           enum wlan_serialization_cmd_type cmd_type,
                           int32_t status);

/*
 * wlan_mlme_start_vdev() - Send start request to a vdev
 * @vdev: vdev object component
 * @f_scan: forcescan flag indicate is rescan is required
 * @notify_osif: post processing handler to be called after cmd execution
 *
 * Return: Success on cmd addition to the serialization queue, else error value
 */
QDF_STATUS
wlan_mlme_start_vdev(struct wlan_objmgr_vdev *vdev,
                     uint32_t f_scan, uint8_t notify_osif);

/*
 * wlan_mlme_stop_vdev() - Send stop request to a vdev
 * @vdev: vdev object component
 * @flags: stop request flags to be sent to mlme
 * @notify_osif: post processing handler to be called after cmd execution
 *
 * Return: Success on cmd addition to the serialization queue, else error value
 */
QDF_STATUS
wlan_mlme_stop_vdev(struct wlan_objmgr_vdev *vdev,
                    uint32_t flags, uint8_t notify_osif);

/*
 * wlan_mlme_cm_start() - Start connection manager for STA vdev
 * @vdev: vdev object component
 * @source: Requestor source id
 *
 * Return: Success on connect request processed, else error value
 */
QDF_STATUS wlan_mlme_cm_start(struct wlan_objmgr_vdev *vdev,
                              enum wlan_cm_source source);

/*
 * wlan_mlme_cm_start() - Start connection manager for STA vdev
 * @vdev: vdev object component
 * @source: Requestor source id
 * @reason: Disconnect reason
 * @sync: Indicate if disconnect has to processed in sync mode
 *
 * Return: Success on connect request processed, else error value
 */
QDF_STATUS wlan_mlme_cm_stop(struct wlan_objmgr_vdev *vdev,
                             enum wlan_cm_source source,
                             enum wlan_reason_code reason,
                             bool sync);

/*
 * wlan_mlme_stop_start_vdev() - Send stop and start request to a vdev
 * with vdev command held.
 * @vdev: vdev object component
 * @f_scan: forcescan flag indicate is rescan is required
 * @notify_osif: post processing handler to be called after cmd execution
 *
 * Return: Success on cmd addition to the serialization queue, else error value
 */
QDF_STATUS wlan_mlme_stop_start_vdev(struct wlan_objmgr_vdev *vdev,
                                     uint32_t f_scan, uint8_t notify_osif);

/*
 * wlan_mlme_dev_restart() - Send restart request to a either pdev or vdev
 * @vdev: vdev object component
 * @type: indicate if it is pdev or vdev restart
 *
 * Return: Success on cmd addition to the serialization queue, else error value
 */
QDF_STATUS wlan_mlme_dev_restart(struct wlan_objmgr_vdev *vdev,
                                 enum wlan_serialization_cmd_type type);

/*
 * wlan_mlme_pdev_csa_restart() - Send csa restart request to pdev
 * @vdev: vdev object component
 * @flags: flags if required
 * @notify_osif: post processing handler to be called after cmd execution
 *
 * Return: Success on cmd addition to the serialization queue, else error value
 */
QDF_STATUS wlan_mlme_pdev_csa_restart(struct wlan_objmgr_vdev *vdev,
        uint32_t flags, uint8_t notify_osif);

/*
 * wlan_mlme_inc_act_cmd_timeout() - increase serialization active cmd timeout
 * @vdev: vdev object component
 * @cmd_type: serialization command type
 *
 * Return: void
 */
void
wlan_mlme_inc_act_cmd_timeout(struct wlan_objmgr_vdev *vdev,
                              enum wlan_serialization_cmd_type cmd_type);

/*
 * wlan_mlme_wait_for_cmd_completion() - wait for vdev active cmds completion
 * @vdev: vdev object component
 *
 * Return: void
 */
void wlan_mlme_wait_for_cmd_completion(struct wlan_objmgr_vdev *vdev);

/*
 * wlan_mlme_wait_for_scan_cmd_completion() - wait for scan cmds completion
 * @vdev: vdev object component
 *
 * This API is expected to be called from non-interrupt context only.
 * qdf_wait_single_event() called in this API asserts if it is called from
 * interrupt context.
 *
 * Return: void
 */
void wlan_mlme_wait_for_scan_cmd_completion(struct wlan_objmgr_vdev *vdev);

/*
 * mlme_vdev_add_stop_start_to_pq_head() - Add stop-start cmd to the head of the
 * pending queue.
 * @vdev: Object manager vdev
 *
 * Enqueue exception start followed by stop cmd to the head of the
 * pending queue (which results in stop-start sequence)
 *
 * Return: Success on adding the cmds to the pending queue.
 */
QDF_STATUS mlme_vdev_add_stop_start_to_pq_head(struct wlan_objmgr_vdev *vdev);

bool ieee80211_is_6g_valid_rate(struct ieee80211vap *vap, u_int32_t rate);
int ieee80211_rate_is_valid_basic(struct ieee80211vap *, u_int32_t);

void wlan_mlme_cm_event_init(struct ieee80211vap *vap);
void wlan_mlme_cm_event_deinit(struct ieee80211vap *vap);

#if SM_ENG_HIST_ENABLE
void wlan_mlme_cm_action_history_init(struct ieee80211vap *vap);
void wlan_mlme_cm_action_history_deinit(struct ieee80211vap *vap);
void wlan_mlme_cm_action_print_history(struct wlan_cm_action_history *cm_action_history);
#endif

QDF_STATUS wlan_mlme_cm_ext_bss_peer_create_req(struct wlan_objmgr_vdev *vdev,
						struct qdf_mac_addr *peer_mac,
						struct qdf_mac_addr *mld_mac,
						bool is_assoc_link);

QDF_STATUS wlan_mlme_cm_vdev_down(struct wlan_objmgr_vdev *vdev);

QDF_STATUS wlan_mlme_cm_bss_peer_delete_req(struct wlan_objmgr_vdev *vdev);

QDF_STATUS wlan_mlme_cm_join_start(struct wlan_objmgr_vdev *vdev,
				   struct wlan_cm_vdev_connect_req *req);

QDF_STATUS wlan_mlme_cm_reassoc_join_start(
                        struct wlan_objmgr_vdev *vdev,
                        struct wlan_cm_vdev_reassoc_req *req);

void mlme_cm_resp_timer(void *arg);

struct scan_cache_entry *mlme_cm_get_active_scan_entry(struct ieee80211vap *vap);

/*
 * wlan_mlme_cm_connect_start() - actions on connect request
 * @vdev: Object manager vdev
 * @req: Connect request parameters
 *
 * Return: QDF_STATUS_SUCCESS if no error in actions
 */
QDF_STATUS wlan_mlme_cm_connect_start(struct wlan_objmgr_vdev *vdev,
				      struct wlan_cm_connect_req *req);

/*
 * wlan_mlme_cm_connect_active() - actions on connect acivation
 * @vdev: Object manager vdev
 * @req: Connect request parameters
 *
 * Return: QDF_STATUS_SUCCESS if no error in actions
 */
QDF_STATUS wlan_mlme_cm_connect_active(struct wlan_objmgr_vdev *vdev,
				       struct wlan_cm_vdev_connect_req *req);

/*
 * wlan_mlme_cm_connect complete() - actions on connect complete
 * @vdev: Object manager vdev
 * @rsp: Connect response parameters
 *
 * Return: QDF_STATUS_SUCCESS if no error in actions
 */
QDF_STATUS wlan_mlme_cm_connect_complete(struct wlan_objmgr_vdev *vdev,
					 struct wlan_cm_connect_resp *rsp);

/*
 * mlme_update_chan_list_and_restart_vaps() - Updates the channel list with
 * new ap power mode. Updates beacon for ap vaps and sends wmi tpc for all
 * vaps.
 * When this function is called, the channel is updated to cfg80211
 * kernel layer and hostpad get notified. If Hostapd finds no tx-able channel
 * in the channel list then it brings down the VAPs.
 * If Hostapd is bringing down the VAPs there is:
 * 1) no need to Update the beacon power info
 * 2) no need to send new 6 Ghz tx power values (setTPC command) to FW.
 * NOTE:- After StopTx is sent from FW to Host, since VAPs are going to go down,
 * we do not need to send the setTPC
 * (or any other commands that are generally sent while VAP is up) and do not
 * need to update beacon power info/mode.
 *
 * Return: void
 */
#ifdef CONFIG_AFC_SUPPORT
void
mlme_update_chan_list_and_restart_vaps(struct ieee80211com *ic,
				       bool update_bcnpwrinfo_and_fwtxpwr);

/*
 * mlme_quick_update_chan_list_and_send_tpc() - Updates the channel list
 * based on new ap power mode. Updates beacon for ap vaps and sends wmi tpc for
 * all vaps.
 * @ic - Pointer to ic.
 * @send_tpc - flag that indicates whether or not beacon update and send
 * wmi tpc are needed or not. If this flag is set to be false, then a vdev
 * restart is expected to update beacon and send wmi tpc command.
 *
 * Return: void.
 */
void
mlme_quick_update_chan_list_and_send_tpc(struct ieee80211com *ic,
				       bool send_tpc);
#else
static inline void
mlme_update_chan_list_and_restart_vaps(struct ieee80211com *ic,
				       bool update_bcnpwrinfo_and_fwtxpwr)
{
}

void
mlme_quick_update_chan_list_and_send_tpc(struct ieee80211com *ic,
                                         bool send_tpc);
#endif

/*
 * wlan_mlme_cm_disconnect_start() - actions on disconnect request
 * @vdev: Object manager vdev
 * @req: Connect request parameters
 *
 * Return: QDF_STATUS_SUCCESS if no error in actions
 */
QDF_STATUS wlan_mlme_cm_disconnect_start(struct wlan_objmgr_vdev *vdev,
					 struct wlan_cm_disconnect_req *req);

/*
 * wlan_mlme_cm_disconnect_active() - actions on disconnect activation
 * @vdev: Object manager vdev
 * @req: Connect request parameters
 *
 * Return: QDF_STATUS_SUCCESS if no error in actions
 */
QDF_STATUS wlan_mlme_cm_disconnect_active(struct wlan_objmgr_vdev *vdev,
					  struct wlan_cm_vdev_discon_req *req);

/*
 * wlan_mlme_cm_disconnect_complete() - actions on disconnect complete
 * @vdev: Object manager vdev
 * @rsp: Connect response parameters
 *
 * Return: QDF_STATUS_SUCCESS if no error in actions
 */
QDF_STATUS wlan_mlme_cm_disconnect_complete(struct wlan_objmgr_vdev *vdev,
					    struct wlan_cm_discon_rsp *rsp);

/**
 * wlan_mlme_cm_ext_hdl_create() - Connection manager callback to create ext
 * context
 * @vdev: VDEV object
 * @ext_cm_ptr: pointer to connection manager ext pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlme_cm_ext_hdl_create(struct wlan_objmgr_vdev *vdev,
				       cm_ext_t **ext_cm_ptr);

/**
 * wlan_mlme_cm_ext_hdl_destroy() - Connection manager callback to create ext
 * context
 * @vdev: VDEV object
 * @ext_cm_ptr: connection manager ext pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlme_cm_ext_hdl_destroy(struct wlan_objmgr_vdev *vdev,
				        cm_ext_t *ext_cm_ptr);
/*
 * wlan_mlme_dispatch_cm_resp() - dispatch cm resp in deferred context
 * @vap: vap handle
 * @cm_evt_id: CM resp event id
 *
 * Return: void
 */
static inline void wlan_mlme_dispatch_cm_resp(struct ieee80211vap *vap,
					      enum cm_resp_evt cm_evt_id)
{
	cm_ext_t *cm_ext_handle = NULL;
	cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);
	if (!cm_ext_handle)
		return;

	cm_ext_handle->cm_evt_id = cm_evt_id;
	if (qdf_timer_mod(&cm_ext_handle->cm_defer_resp, 0))
		QDF_ASSERT(0);
}

/*
 * wlan_mlme_get_assoc_sm_handle() - get assoc sm handle
 * @vdev: Vdev object
 *
 * Return: void
 */
static inline
wlan_assoc_sm_t wlan_mlme_get_assoc_sm_handle(struct wlan_objmgr_vdev *vdev)
{
     cm_ext_t *cm_ext_handle = NULL;
     cm_ext_handle = wlan_cm_get_ext_hdl(vdev);
     if (!cm_ext_handle)
         return NULL;

     return cm_ext_handle->assoc_sm_handle;
}

/*
 * wlan_mlme_get_assoc_sm_scan_entry() - get assoc sm cached scan entry
 * @vdev: Vdev object
 *
 * Return: scan entry
 */
wlan_scan_entry_t wlan_mlme_get_assoc_sm_scan_entry(struct wlan_objmgr_vdev *vdev);

/*
 * wlan_mlme_assoc_sm_is_state_assoc() - check if assoc sm is in assoc state or not
 * @vdev: Vdev object
 *
 * Return: true if assoc sm is in assoc state, false otherwise
 */
bool wlan_mlme_assoc_sm_is_state_assoc(struct wlan_objmgr_vdev *vdev);

#ifdef WLAN_FEATURE_11BE_MLO
/*
 * wlan_mlme_update_assoc_sm_scan_entry() - get assoc sm cached scan entry
 * @vdev: Vdev object
 *
 * Return: none
 */
void wlan_mlme_update_assoc_sm_scan_entry(struct wlan_objmgr_vdev *vdev);
#endif

/*
 * wlan_mbss_register_ext_ops() - register MBSS ext ops
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_register_ext_ops(void);

/*
 * wlan_mbss_start_acs() - register & trigger ACS handler
 * @vdev: vdev object
 * @arg: arg to pass for ACS
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_start_acs(struct wlan_objmgr_vdev *vdev, void *arg);

/*
 * wlan_mbss_cancel_acs() - cancel ACS
 * @vdev: vdev object
 * @arg: arg to pass for ACS
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_cancel_acs(struct wlan_objmgr_vdev *vdev, void *arg);

/*
 * wlan_mbss_ext_done_acs() - unregister ACS handler
 * @arg: input arg passed to ACS receive
 * @channel: channel returned by ACS
 * @status: status
 *
 * Return: void
 */
void wlan_mbss_done_acs(struct wlan_objmgr_vdev *vdev, void *channel,
			QDF_STATUS status);

/*
 * wlan_mbss_start_ht40() - register & trigger HT40
 * @vdev: vdev object
 * @arg: arg to pass for ACS
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_start_ht40(struct wlan_objmgr_vdev *vdev, void *arg);

/*
 * wlan_mbss_cancel_ht40() - cancel ht40 scan
 * @vdev: vdev object
 * @arg: arg to pass for ACS
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_cancel_ht40(struct wlan_objmgr_vdev *vdev, void *arg);

/*
 * wlan_mbss_ext_done_ht40() - unregister HT40 handler
 * @arg: input arg passed to HT40 receive
 * @channel: channel returned by HT40
 *
 * Return: void
 */
void wlan_mbss_done_ht40(struct wlan_objmgr_vdev *vdev, void *channel,
			 QDF_STATUS status);

void
wlan_mlme_if_mgr_ev_send(
		struct wlan_objmgr_vdev *vdev,
		enum wlan_serialization_cmd_type cmd_type);

void
wlan_mlme_if_mgr_ev_complete_send(
		struct wlan_objmgr_vdev *vdev,
		enum wlan_serialization_cmd_type cmd_type);

#if WLAN_SER_DEBUG
extern void wlan_ser_print_history(struct wlan_objmgr_vdev *vdev, u_int8_t,
                                   u_int32_t);
#else
#define wlan_ser_print_history(params...)
#endif

#if SM_ENG_HIST_ENABLE
void wlan_mlme_print_all_sm_history(void);
void wlan_mlme_print_vdev_sm_history(struct wlan_objmgr_psoc *psoc,
                                     void *obj, void *arg);
#if SM_HIST_DEBUGFS_SUPPORT
/*
 * wlan_mlme_sm_debugfs_history() - Print VDEV SM history in debugfs
 * @m: debug fs file
 * @v: context
 *
 * Return: void
 */
int wlan_mlme_sm_debugfs_history(qdf_debugfs_file_t m, void *arg);
#endif
#else
static inline void wlan_mlme_print_all_sm_history(void) {}
static inline void wlan_mlme_print_vdev_sm_history(struct wlan_objmgr_psoc *psoc,
                                                   void *obj, void *arg) {}
#if SM_HIST_DEBUGFS_SUPPORT
static int wlan_mlme_sm_debugfs_history(qdf_debugfs_file_t m, void *arg) { return 0; }
#endif
#endif

void wlan_mlme_start_restart_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
					     void *object, void *arg);

void wlan_mlme_start_sta_vdevs(struct wlan_objmgr_pdev *pdev,
			       void *object, void *arg);

void wlan_mlme_start_ap_vdevs(struct wlan_objmgr_pdev *pdev,
			      void *object, void *arg);

void wlan_mlme_start_bridge_vdevs(struct wlan_objmgr_pdev *pdev,
			      void *object, void *arg);

void wlan_mlme_stop_start_ap_vdevs(struct wlan_objmgr_pdev *pdev,
				   void *object, void *arg);

void wlan_mlme_stop_start_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
					   void *object, void *arg);

void wlan_mlme_stop_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
				     void *object, void *arg);

void wlan_mlme_stop_sta_vdevs(struct wlan_objmgr_pdev *pdev,
			      void *object, void *arg);

void wlan_mlme_start_vdevs(struct wlan_objmgr_pdev *pdev,
			   void *object, void *arg);

void wlan_mlme_start_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
				      void *object, void *arg);

void wlan_mlme_stop_vdevs(struct wlan_objmgr_pdev *pdev,
			  void *object, void *arg);

#ifdef WLAN_FEATURE_11BE_MLO

#ifdef WLAN_MCAST_MLO
/*
 * wlan_mlme_mlo_set_mcast_vdev()- Set mcast primary vdev for MLO
 * @vdev: vdev object
 *
 * Return: none
 */
void wlan_mlme_mlo_set_mcast_vdev(struct wlan_objmgr_vdev *vdev, bool mcast_vdev);
#else
#define wlan_mlme_mlo_set_mcast_vdev(vdev, mcast_vdev)
#endif

/*
 * wlan_mlme_mlo_update_partner_info()- upate mlo partner info to mlo manager
 * @vap: vap object
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlme_mlo_update_partner_info(wlan_if_t vap);

/*
 * wlan_mlme_mlo_remove_vap_partner_mlinfo()- remove vap info from mlo partner info of parnter vaps
 * @vap: vap object
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlme_mlo_remove_vap_partner_mlinfo(wlan_if_t vap);
/*
 * mlme_mlo_validate_partnerinfo- validate mlo partner info from user space.
 * @mlo_info: pointer to mlo info
 * @vap: vap object
 * @link_id: pointer to link_id, will be used after successful validations.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlme_mlo_validate_partnerinfo(struct wlan_mlme_mlo_info *ml_info,
        wlan_if_t vap, uint8_t *link_id);

/*
 * wlan_mlme_mlo_validate_ap_phymode() - Validate AP PHY mode for MLO
 * @vap: vap object
 *
 * Validate whether the PHY mode requested can be used with MLO if MLO has been
 * requested by space/hostapd as part of start AP sequence. If the VAP is not
 * MLO capable or MLO has not been requested, then return success since the API
 * does not care about the PHY mode in this case.
 *
 * Return: QDF_STATUS_SUCCESS in the case of successful validation, QDF_STATUS
 * value giving the reason for error in the case of failure.
 */
QDF_STATUS wlan_mlme_mlo_validate_ap_phymode(wlan_if_t vap);

/*
 * mlme_mlo_link_add_get_current_batch() - Get the list of vdevs provided in
 * the current MLO link addition batch
 * @vap: vap on which start_ap() is received
 * @batch_size: (writable argument) size of the batch
 * @batch_vdev_list: (writable argument) List of vdevs in this batch
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS
mlme_mlo_link_add_get_current_batch(wlan_if_t vap,
                                    uint16_t *batch_size,
                                    struct wlan_objmgr_vdev **batch_vdev_list);
/*
 * mlme_mlo_link_add_get_current_batch_size() - Get the size of the current MLO
 * link addition batch
 * @vap: vap on which start_ap() is received
 * @batch_size: (writable argument) size of the batch
 * @batch_vdev_list: (writable argument) List of vdevs in this batch
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS
mlme_mlo_link_add_get_current_batch_size(wlan_if_t vap, uint16_t *batch_size);

/*
 * wlan_mlme_is_non_mlo_11be_ap() - Check whether the vap is operating in
 * non MLO 11be AP mode
 * @vap: vap object
 *
 * Return: true if @vap is configured in non MLO 11be mode, else
 * false
 */
bool wlan_mlme_is_non_mlo_11be_ap(wlan_if_t vap);

/*
 * wlan_mlme_is_non_mlo_11be_ap_operation_allowed() - Check whether non MLO 11be
 * AP operation is allowed in the system
 * @vap: vap object
 *
 * Return: true if non MLO 11 BE AP operation is allowed, else false
 */
bool wlan_mlme_is_non_mlo_11be_ap_operation_allowed(wlan_if_t vap);
#else
static inline
QDF_STATUS wlan_mlme_mlo_remove_vap_partner_mlinfo(wlan_if_t vap)
{
    return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_FEATURE_11BE_MLO */

#ifdef WLAN_FEATURE_11BE
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
int wlan_mlme_assoc_t2lm_params(struct wlan_objmgr_peer *peer,
				bool is_assoc_frame,
				struct wlan_t2lm_onging_negotiation_info *t2lm_info,
				enum qca_wlan_vendor_t2lm_category category);

void wlan_mlme_send_disassoc_event(struct wlan_objmgr_peer *peer,
				   enum qca_wlan_vendor_attr_app_generic_category category);

int wlan_mlme_send_assoc_event(struct wlan_objmgr_peer *peer,
				bool is_assoc_frame,
				struct wlan_t2lm_onging_negotiation_info *t2lm_info,
				enum qca_wlan_vendor_attr_app_generic_category category);

bool wlan_mlme_is_rm_app_enabled(void);
bool wlan_mlme_is_rm_admctrl_service_enabled(void);
bool wlan_mlme_is_rm_energy_service_enabled(void);

int wifi_driver_get_psoc_info(void *obj, struct agent_psoc_iface_init_obj *stats);
int wifi_driver_get_pdev_info(void *obj, struct agent_pdev_iface_init_obj *stats);
int wifi_driver_get_peer_info(void *obj, struct agent_peer_iface_init_obj *stats);
int wifi_driver_get_psoc_stats(void *obj);
int wifi_driver_get_pdev_stats(void *obj, struct agent_link_iface_stats_obj *stats);
int wifi_driver_get_peer_stats(void *obj, struct agent_peer_iface_stats_obj *stats);
#ifdef QCA_SUPPORT_CP_STATS
int wifi_driver_get_emesh_pdev_stats(void *obj,
				     struct emesh_link_iface_stats_obj *stats);
#else
static inline
int wifi_driver_get_emesh_pdev_stats(void *obj,
				     struct emesh_link_iface_stats_obj *stats)
{
	return 0;
}
#endif
int wifi_driver_get_emesh_peer_stats(void *obj,
				     struct emesh_peer_iface_stats_obj *stats);

int wifi_driver_get_deter_pdev_stats(void *obj,
				     struct deter_link_iface_stats_obj *stats);

int wifi_driver_get_deter_peer_stats(void *obj,
				     struct deter_peer_iface_stats_obj *stats);

int wifi_driver_get_erp_pdev_stats(void *obj,
				   struct erp_link_iface_stats_obj *stats);

int wifi_driver_get_admctrl_pdev_stats(void *obj,
				       struct admctrl_link_iface_stats_obj *stats);

int wifi_driver_get_admctrl_peer_stats(void *obj,
				       struct admctrl_peer_iface_stats_obj *stats);
#else
static inline
int wlan_mlme_assoc_t2lm_params(struct wlan_objmgr_peer *peer,
				bool is_assoc_frame,
				struct wlan_t2lm_onging_negotiation_info *t2lm_info,
				enum qca_wlan_vendor_t2lm_category category)
{
	return -1;
}

static inline
void wlan_mlme_send_disassoc_event(struct wlan_objmgr_peer *peer,
				   enum qca_wlan_vendor_attr_app_generic_category category)
{
	return;
}

static inline
int wlan_mlme_send_assoc_event(struct wlan_objmgr_peer *peer,
				bool is_assoc_frame,
				struct wlan_t2lm_onging_negotiation_info *t2lm_info,
				enum qca_wlan_vendor_attr_app_generic_category category)
{
	return 0;
}

static inline
bool wlan_mlme_is_rm_app_enabled(void)
{
	return false;
}

static inline
bool wlan_mlme_is_rm_admctrl_service_enabled(void)
{
       return false;
}

static inline
bool wlan_mlme_is_rm_energy_service_enabled(void)
{
       return false;
}
#endif /* WLAN_CONFIG_TELEMETRY_AGENT */
#endif /* WLAN_FEATURE_11BE */

/*
 * wlan_mlme_check_mbss_mixed_modes_config() - Check mixed mode config against
 * MBSSID groups
 * @vap: vap object
 *
 * When mixed mode is configured for a VAP with MBSSID enabled, the PHY modes of
 * all VAPs in an MBSSID group should be the same. But, the mode of different
 * MBSSID groups may vary. This function checks this per-group PHY mode sanity.
 *
 * Return: QDF_STATUS_SUCCESS if MBSSID PHY mode group sanity passes;
 *         QDF_STATUS_E_INVAL, otherwise
 */
QDF_STATUS wlan_mlme_check_mbss_mixed_modes_config(struct ieee80211vap *vap);

#endif /* _WLAN_MLME_IF_H  */
