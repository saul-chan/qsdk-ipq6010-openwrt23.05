/*
 * Copyright (c) 2015,2017-2022,2024 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2015 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifdef IPA_OFFLOAD
#include <wlan_ipa_ucfg_api.h>
#endif
#include <hif.h>
#include <osdep.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/if_arp.h>
#include "ath_pci.h"
#include "ol_ath.h"
#include "hif.h"
#include "hif_napi.h"
#include "pld_common.h"
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <acfg_api_types.h>
#include <acfg_event_types.h>
#include <qdf_mem.h>
#include <osif_private.h>
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_if.h>
#endif
#if CONFIG_LEDS_IPQ
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
#include <drivers/leds/leds-ipq.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
#include <drivers/leds/leds-ipq40xx.h>
#endif
#endif
#include <init_deinit_lmac.h>
#include <hif_main.h>
#include "qal_vbus_dev.h"
#if QLD
#include "qld_api.h"
#endif
#include "qal_devnode.h"
#include <target_if.h>
#include <pktlog_ac.h>

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
#include <wlan_mlo_mgr_setup.h>
#include <wlan_mlo_mgr_public_structs.h>
#include <wmi_unified_11be_param.h>
#include <wlan_mlo_mgr_peer.h>
#include <wlan_mlo_mgr_main.h>
#include <wlan_mlo_mgr_cmn.h>
extern void qca_ol_update_mlo_capability(void);

#endif

#define GPIO_SET_DIR_OUT        (0 << 0)

extern struct platform_device_id *ath_ahb_get_pdev_id(struct platform_device *pdev);
extern ol_ath_soc_softc_t *ol_global_soc[GLOBAL_SOC_SIZE];
extern int ol_num_global_soc;
extern void cnss_dump_qmi_history(void);
extern void init_deinit_mlo_update_soc_ready(struct wlan_objmgr_psoc *psoc);
extern void init_deinit_mlo_update_pdev_ready(struct wlan_objmgr_psoc *psoc,
					      uint8_t num_radios);
extern void mlo_setup_init(uint8_t total_grp);
extern void mlo_setup_deinit(void);
void *ol_hif_open(struct device *dev, void *bdev, void *bid,
        enum qdf_bus_type bus_type, bool reinit, qdf_device_t qdf_dev);
void ol_hif_close(void *hif_ctx);
int ol_ath_soc_start(struct ol_ath_soc_softc *soc);
int ol_ath_soc_stop(struct ol_ath_soc_softc *soc, bool flush_wq);
int ol_ath_target_start(struct ol_ath_soc_softc *scn);
QDF_STATUS ol_mlo_glb_h_shmem_arena_ctx_deinit(ol_ath_soc_softc_t *soc);
int osif_recover_vap_delete(struct ieee80211com *ic);
void ath_vdev_recover_delete(struct wlan_objmgr_psoc *psoc, void *obj, void *args);
void ol_ath_tx_stop(struct ol_ath_soc_softc *soc);
void ol_ath_target_timer_stop(struct ol_ath_soc_softc *soc);
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
void ol_ath_delete_all_mlo_peers(struct recovery_soc_info *soc_info, struct ol_ath_soc_softc *soc);
#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP
void ol_ath_delete_all_mlo_peers_wsi(struct ol_ath_soc_softc *soc);
#endif
#endif
void ol_ath_wifi_recovery_remove(struct platform_device *pdev);
int ol_ath_wifi_recovery_probe(struct platform_device *pdev, const struct platform_device_id *id);
void hif_set_target_status(struct hif_opaque_softc *hif_ctx, enum
                           hif_target_status status);
struct net_device *pci_dev_to_net_dev(struct pci_dev *pdev);
void ath_vdev_recover_peer_deauth_handler(struct wlan_objmgr_psoc *psoc,
                                           void *obj, void *args);

void ol_ath_pci_remove(struct pci_dev *pdev);
int ol_ath_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id);

int wifi3_ko_exit_in_progress = 0;
qdf_export_symbol(wifi3_ko_exit_in_progress);
extern unsigned int testmode;

int wifi_exit_in_progress = 0;
qdf_export_symbol(wifi_exit_in_progress);

#if QLD
int  qld_process_list(struct ol_ath_soc_softc *soc);
int  qld_dump_wait(struct ol_ath_soc_softc *soc);
#endif

#if UMAC_SUPPORT_WEXT
void ol_ath_iw_attach(struct net_device *dev);
void ol_ath_iw_detach(struct net_device *dev);
#endif

extern qdf_mutex_t probe_lock;
/*
 * This function should not be called for non-lithum PCI chipsets.
 * Should not be used for Wifi2.0 PCI chipsets(AR900B and QCA9984).
 */
struct net_device *platform_dev_to_net_dev(struct platform_device *pdev)
{
    struct pld_plat_data *cnss_plat = NULL, *wlan_sc = NULL;
    struct net_device *dev = NULL;
    struct platform_device_id *id = NULL;
    pci_priv_data *pci_priv = NULL;
    struct pci_dev *pcidev = NULL;

    if(!pdev) {
            return NULL;
    }

    pcidev = pld_get_pci_dev_from_plat_dev((void *)pdev);

#ifndef BUILD_X86
    id = ath_ahb_get_pdev_id(pdev);
#else
    /* We dont need to get id in x86 platform. Make this NULL */
    id = NULL;
#endif

    if (id && ((id->driver_data == QCA8074_DEVICE_ID) ||
               (id->driver_data == QCA8074V2_DEVICE_ID) ||
               (id->driver_data == QCA9574_DEVICE_ID) ||
               (id->driver_data == QCN6122_DEVICE_ID) ||
               (id->driver_data == QCN9160_DEVICE_ID) ||
               (id->driver_data == QCA6018_DEVICE_ID) ||
               (id->driver_data == QCA5332_DEVICE_ID) ||
               (id->driver_data == QCA5018_DEVICE_ID) ||
               (id->driver_data == QCN6432_DEVICE_ID))) {
            /* Lithium AHB case */
            cnss_plat = platform_get_drvdata(pdev);
            wlan_sc  = cnss_plat->wlan_priv;
            if(wlan_sc)
                dev = (struct net_device *) wlan_sc->wlan_priv;
    } else if (pcidev &&
              ((pcidev->device == QCN9000_DEVICE_ID) ||
               (pcidev->device == QCN9224_DEVICE_ID))) {
            /* Lithium PCI case */
            pci_priv = pci_get_drvdata(pcidev);
            /* pci_priv will be NULL during BEFORE_POWERUP and will be
             * initialized during powerup and will be non-null during
             * AFTER_POWERUP event*/
            if(pci_priv)
                cnss_plat = (struct pld_plat_data *)pci_priv->wlan_priv;
            /* cnss_plat will be NULL during BEFORE_POWERUP and non-null
             * during AFTER_POWERUP event */
            if(cnss_plat)
                wlan_sc  = cnss_plat->wlan_priv;
            /* wlan_sc will be NULL during BEFORE_POWERUP and AFTER_POWERUP
             * event. It will get allocated during ol_ath_pci_probe when memory
             * allocation is done */
            if(wlan_sc)
                /* netdev will be assigned to dev during ol_ath_pci_probe */
                dev = (struct net_device *) wlan_sc->wlan_priv;
    } else {
        /* This path will hit for wifi2 PCI chipsets. This function
         * should not be called for non-lithium pci chipsets.
         */
        qdf_assert_always(0);
    }
    return dev;
}
qdf_export_symbol(platform_dev_to_net_dev);

struct net_device *platform_dev_id_to_net_dev(struct platform_device *pdev, const struct platform_device_id *id)
{
    struct pld_plat_data *cnss_plat, *wlan_sc;
    struct net_device *dev = NULL;

    if(!pdev) {
            return NULL;
    }
    if ((id && id->driver_data == QCA8074_DEVICE_ID) ||
        (id && id->driver_data == QCA8074V2_DEVICE_ID) ||
        (id && id->driver_data == QCA9574_DEVICE_ID) ||
        (id && id->driver_data == QCA6018_DEVICE_ID) ||
        (id && id->driver_data == QCN6122_DEVICE_ID) ||
        (id && id->driver_data == QCA5332_DEVICE_ID) ||
        (id && id->driver_data == QCN9160_DEVICE_ID) ||
        (id && id->driver_data == QCA5018_DEVICE_ID) ||
        (id && id->driver_data == QCN6432_DEVICE_ID)) {
            cnss_plat = platform_get_drvdata(pdev);
            wlan_sc  = cnss_plat->wlan_priv;
            if(wlan_sc)
	            dev = (struct net_device *) wlan_sc->wlan_priv;
    } else {
            dev = platform_get_drvdata(pdev);
    }
    return dev;

}
qdf_export_symbol(platform_dev_id_to_net_dev);

bool ol_ath_supported_dev(const struct platform_device_id *id)
{
	switch (id->driver_data) {
#ifdef QCA_WIFI_QCA8074
		case QCA8074_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCA8074V2
		case QCA8074V2_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCA9574
		case QCA9574_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCA6018
		case QCA6018_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCA5018
		case QCA5018_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCN6122
		case QCN6122_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCA5332
                case QCA5332_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCN9160
		case QCN9160_DEVICE_ID:
#endif
#ifdef QCA_WIFI_QCN6432
        case QCN6432_DEVICE_ID:
#endif
			return true;
		default:
			return false;
	}
	return false;
}
qdf_export_symbol(ol_ath_supported_dev);

#define HIF_IC_MAX_IRQ 52
irqreturn_t hif_dummy_irq_handler(int irq, void *context)
{
	return IRQ_HANDLED;
}
extern const char *ic_irqname[];
static int register_free_dummy_irq(struct platform_device *pdev, int isregister)
{
	const char *irq_name;
	int j;
	int irq = 0, ret = 0;
	int irq_entry_count = HIF_IC_MAX_IRQ;
	struct platform_device_id *id = NULL;

#ifndef BUILD_X86
	id = ath_ahb_get_pdev_id(pdev);
#else
	/* We dont need to get id in x86 platform. Make this NULL */
	id = NULL;
#endif

        if (id && (id->driver_data == QCN6122_DEVICE_ID ||
		   id->driver_data == QCN9160_DEVICE_ID ||
		   id->driver_data == QCN6432_DEVICE_ID))
		return 0;

	for (j = 0; j < irq_entry_count; j++) {
		irq_name = ic_irqname[j];
		qal_vbus_get_irq((struct qdf_pfm_hndl *)pdev, irq_name, &irq);
		if (irq < 0) {
			continue;
		}
		if (id && ((id->driver_data == QCA5018_DEVICE_ID) ||
			(id->driver_data == QCA8074_DEVICE_ID) ||
			(id->driver_data == QCA8074V2_DEVICE_ID) ||
			(id->driver_data == QCA5332_DEVICE_ID) ||
			(id->driver_data == QCA9574_DEVICE_ID) ||
			(id->driver_data == QCA6018_DEVICE_ID)) &&
			(!qdf_str_ncmp("watchdog",irq_name,strlen(irq_name)))) {
				continue;
		}

		if (isregister) {
			ret = request_irq(irq, hif_dummy_irq_handler,
					IRQF_TRIGGER_RISING,
					ic_irqname[j],
					NULL);
		} else {
			free_irq(irq, NULL);
		}
	}
	return 0;
}

uint8_t inline get_verbose_lvl(uint16_t logs_lvl)
{
    uint8_t verbose = 0;

    while (logs_lvl >>= 1)
           verbose++;

    if (verbose > QDF_TRACE_LEVEL_MAX)
        return QDF_TRACE_LEVEL_NONE;

    return verbose;
}

void ol_ath_set_logs_level_while_recovery(struct ol_ath_soc_softc *soc, uint8_t value)
{
    uint8_t i;
    uint16_t mid;
    uint32_t pr_logs = 0;
    uint8_t verbose_lvl = 0;
    uint32_t module_id[MAX_MOD_LOGS_LEVEL] = {QDF_MODULE_ID_WMI, QDF_MODULE_ID_MBSSIE,
                                          QDF_MODULE_ID_TARGET_IF, QDF_MODULE_ID_MLO};

   if (!cfg_get(soc->psoc_obj, CFG_OL_ENABLE_RECOVERY_LOGS)) {
      for (i = 0; i < MAX_MOD_LOGS_LEVEL; i++) {
           mid = module_id[i];
           if (!value) {
                if (wlan_get_print_ctrl_category_verbose(mid)) {
                    soc->recov_logs_level[i] = wlan_get_print_ctrl_category_verbose(mid);
                    pr_logs = (mid << 16) | QDF_TRACE_LEVEL_NONE;
                    wlan_set_shared_print_ctrl_category_verbose(pr_logs);
                }
           } else {
                if (!wlan_get_print_ctrl_category_verbose(mid)) {
                    verbose_lvl = get_verbose_lvl(soc->recov_logs_level[i]);
                    pr_logs = ((mid << 16) | verbose_lvl);
                    wlan_set_shared_print_ctrl_category_verbose(pr_logs);
                }
           }
      }

#ifdef CE_TASKLET_DEBUG_ENABLE
      if (!value)
              ol_ath_enable_ce_latency_stats(soc, 0);
      else
              ol_ath_enable_ce_latency_stats(soc, 1);
#endif
   }
}

#if !(defined REMOVE_PKT_LOG) && (defined PKTLOG_DUMP_UPLOAD_SSR)
static void ol_ath_pktlog_dump_upload_ssr(struct ol_ath_soc_softc *soc)
{
    struct wlan_objmgr_pdev *pdev_obj;
    uint8_t i, num_radios, pdev_id;
    struct ol_ath_softc_net80211 *scn;
    struct target_psoc_info *tgt_psoc_info;

    if (soc == NULL)
        return;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    num_radios = target_psoc_get_num_radios(tgt_psoc_info);

    for (i = 0; i < num_radios; i++) {
         pdev_obj = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, i,
                                               WLAN_MLME_SB_ID);
         if (!pdev_obj)
             continue;

         pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev_obj);
         scn = lmac_get_pdev_feature_ptr(pdev_obj);

         if (scn && scn->upload_pktlog && scn->pl_dev &&
             scn->pl_dev->pl_funcs->pktlog_set_pktlog_ssr_dump)
             scn->pl_dev->pl_funcs->pktlog_set_pktlog_ssr_dump(scn);

         wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_SB_ID);
    }
}
#endif

#if defined(CONFIG_CNSS2_SUPPORT) && defined(WLAN_FEATURE_11BE_MLO) && \
    defined(WLAN_MLO_MULTI_CHIP)
void ol_ath_mlo_crash_partner_chips(struct recovery_soc_info *soc_info,
                                    struct wlan_objmgr_psoc *psoc)
{
    uint8_t soc_idx;
    ol_ath_soc_softc_t *soc = NULL;

    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
         if ((soc_info->curr_soc_list[soc_idx] == psoc) || (!soc_info->curr_soc_list[soc_idx]))
             continue;

         soc = lmac_get_psoc_feature_ptr(soc_info->curr_soc_list[soc_idx]);

         if (!soc)
             continue;

         if (!soc->mlo_partner_hang_issued) {
             qdf_err("Inject intended internal target assert for mode0 for psoc %d",
                      wlan_psoc_get_id(soc->psoc_obj));
             ol_ath_set_fw_hang(soc->psoc_obj, 0, PANIC_TYPE_ASSERT);
             soc->mlo_partner_hang_issued = true;
         }
    }

}

void ol_ath_set_recovery_mode_for_all(struct recovery_soc_info *soc_info,
                                      struct wlan_objmgr_psoc *psoc,
                                      uint32_t recovery_mode)
{
    uint8_t soc_idx;
    ol_ath_soc_softc_t *soc = NULL;

    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
       if (!soc_info->def_soc_list[soc_idx])
           continue;

       soc = lmac_get_psoc_feature_ptr(soc_info->def_soc_list[soc_idx]);

       if (!soc)
           continue;

       soc->recovery_type = recovery_mode;
    }
}

#ifdef QCA_SUPPORT_WDS_EXTENDED
static
void ath_vdev_recover_wds_peer_delete_handler(struct wlan_objmgr_psoc *psoc,
                                              void *obj, void *args)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
    struct ieee80211vap *vap = NULL;
    osif_peer_dev *osif_peer = NULL;
    uint8_t mlo_peer_only = *(uint8_t *)args;
    uint8_t grp_id;
    bool wsi_remap_in_progress = false;


    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_info("peer_release_handler: Null vap, vdev: 0x%pK", vdev);
        return;
    }

    if (ol_ath_mlo_capable(psoc)) {
        grp_id = ol_ath_mlo_get_group_id(psoc);
        wsi_remap_in_progress = wlan_mlo_is_wsi_remap_in_progress(grp_id);
    }

    /* mlo_peer_only flag indicates to delete mlo peers alone, for that case
       proceed for mlo peers and drop slo and non mlo peers */
    if (mlo_peer_only && (!peer->mlo_peer_ctx || ((peer->mlo_peer_ctx->max_links == 1) && (!wsi_remap_in_progress)))) {
         return;
    }

    cdp_wds_ext_get_peer_osif_handle(wlan_psoc_get_dp_handle(wlan_vdev_get_psoc(vdev)),
                                     wlan_vdev_get_id(vdev),  peer->macaddr,
                                     (ol_osif_peer_handle *)&osif_peer);
    if (osif_peer) {
        osif_wds_ext_delete_netdev(osif_peer->netdev);
        cdp_wds_ext_set_peer_rx(
                           wlan_psoc_get_dp_handle(wlan_vdev_get_psoc(vdev)),
                           wlan_vdev_get_id(vap->vdev_obj),
                           peer->macaddr, NULL, NULL);
    }
}
#endif

enum mlo_chip_recovery_type get_recovery_mode(struct ol_ath_soc_softc *soc)
{
    uint32_t value = MLO_RECOVERY_MODE_0;;

#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
    if (!soc->recovery_mode_address) {
        return value;
    }

    switch (*soc->recovery_mode_address) {
    case 1: value = MLO_RECOVERY_MODE_0;
            break;
    case 2:
    case 3: value = MLO_RECOVERY_MODE_1;
            break;
    default: value = MLO_RECOVERY_MODE_0;
             break;
    }
#endif
    return value;
}

void ol_ath_mlo_trigger_mode0_recovery(struct recovery_soc_info *soc_info,
                                       struct wlan_objmgr_psoc *psoc)
{
    uint8_t soc_idx;
#ifdef QCA_SUPPORT_WDS_EXTENDED
    uint8_t mlo_peer_only = 0;
#endif

    /**
     * As of now there is only single MLO group. In case of multiple groups
     * ensure to find the mlo group to which psoc belongs, and populate the
     * soc_list from the corresponding ml group
     */
    qdf_info("Trigger mode0 Recovery");

    /* cleanup mlo setup info associated with asserted SOC */
    mlo_cleanup_asserted_soc_setup_info(psoc, soc_info->grp_id);

    /* teardown MLO on other targets */
    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
         if (soc_info->curr_soc_list[soc_idx])
             mlo_link_teardown_link(soc_info->curr_soc_list[soc_idx], soc_info->grp_id,
                                    WMI_HOST_MLO_TEARDOWN_REASON_SSR);
    }

#ifdef QCA_SUPPORT_WDS_EXTENDED
    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
         if (soc_info->curr_soc_list[soc_idx]) {
             wlan_objmgr_iterate_obj_list(soc_info->curr_soc_list[soc_idx], WLAN_PEER_OP,
                                          ath_vdev_recover_wds_peer_delete_handler, &mlo_peer_only, 1,
                                          WLAN_MLME_NB_ID);
         }
    }
#endif

    /* Unmap and delete all peers across SOC participating in MLO */
    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
         if (soc_info->curr_soc_list[soc_idx]) {
             wlan_objmgr_iterate_obj_list_all(soc_info->curr_soc_list[soc_idx],
                                              WLAN_PEER_OP,
                                              wlan_recovery_peer_release_handler,
                                              NULL, 1, WLAN_MLME_NB_ID);

             /* call API to flush peers */
             wlan_objmgr_iterate_obj_list(soc_info->curr_soc_list[soc_idx], WLAN_VDEV_OP,
                                          wlan_recovery_vdev_delete_peers,
                                          NULL, 1, WLAN_MLME_NB_ID);
         }
    }

}

void __qca_ol_update_mlo_capability(bool wsi_remap, bool wsi_recv_in_progress)
{
   int grp_id = 0, total_grp = 0, i = 0;
   bool status;
   uint8_t *soc_id_list;
   struct cnss_mlo_group_info info;
   struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();


   total_grp = pld_get_num_mlo_groups();

   if (total_grp <= 0 || total_grp > WLAN_MAX_MLO_GROUPS) {
       qdf_info("MLO not supported due to total groups %d is not valid !",
               total_grp);
       return;
   }
   /* Save setup info during WSI remap */
   if (!wsi_remap)
       mlo_setup_init(total_grp);

   for (grp_id = 0; grp_id < total_grp; grp_id++) {
        status = pld_get_mlo_group_info(grp_id, &info);

        if (!status) {
            qdf_err("Unable to get mlo group info! MLO Setup sequence failed!");
            mlo_setup_deinit();
            return;
        }

        if (info.num_chips <= 0 || info.num_chips > CNSS_MAX_MLO_CHIPS || info.num_chips > MAX_MLO_CHIPS ) {
             qdf_err("Invalid number of mlo chips %d is not in limits (1,%d) or (1,%d) for group id %d !",
                     info.num_chips, MAX_MLO_CHIPS, CNSS_MAX_MLO_CHIPS, grp_id);
             mlo_setup_deinit();
             return;
        }

        if ((wsi_remap && wlan_mlo_is_wsi_remap_in_progress(grp_id)) || !wsi_remap || wsi_recv_in_progress) {
            mlo_setup_update_total_socs(grp_id, info.num_chips);
        } else {
            if (!mlo_ctx)
                return;
            mlo_ctx->setup_info[grp_id].tot_socs = info.num_chips;
            mlo_ctx->setup_info[grp_id].tot_links += 1;
        }
        soc_id_list = (uint8_t *)qdf_mem_malloc(sizeof(uint8_t) *
                                                info.num_chips);
        if (!soc_id_list) {
             qdf_err("Unable to allocate memory for soc_id_list! MLO Setup sequence failed!");
             mlo_setup_deinit();
             return;
        }

        for (i = 0; i < info.num_chips; i++) {
            soc_id_list[i] = info.chip_info[i].soc_id;
        }

        mlo_setup_update_soc_id_list(grp_id, soc_id_list);
        qdf_mem_free(soc_id_list);
   }
}
qdf_export_symbol(__qca_ol_update_mlo_capability);

#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP
QDF_STATUS ol_ath_dynamic_wsi_remap_mlo_teardown(ol_ath_soc_softc_t *soc)
{
    struct wlan_objmgr_psoc *grp_soc_list[MAX_MLO_CHIPS];
    uint8_t soc_idx;
    uint8_t grp_id;
    uint8_t tot_grp_socs;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
#ifdef QCA_SUPPORT_WDS_EXTENDED
    uint8_t mlo_peer_only = 1;
#endif
    ol_ath_soc_softc_t *temp_soc;
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();

    if (!mlo_ctx) {
        qdf_err("MLO CTX is null");
        return QDF_STATUS_E_FAILURE;
    }

    if (!psoc) {
        qdf_err("PSOC is null");
        return QDF_STATUS_E_FAILURE;
    }

    grp_id = ol_ath_mlo_get_group_id(psoc);
    tot_grp_socs = mlo_setup_get_total_socs(grp_id);

    if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
        return QDF_STATUS_E_INVAL;

    mlo_get_soc_list(grp_soc_list, grp_id, tot_grp_socs, WLAN_MLO_GROUP_CURRENT_SOC_LIST);

#ifdef QCA_SUPPORT_WDS_EXTENDED
    for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
        if (!grp_soc_list[soc_idx])
             continue;
        /* Skip the clean up for the soc that is going to be brought up dynamically*/
        if (grp_soc_list[soc_idx]->wsi_remap_add)
             continue;
        /* For Bypassed soc delete all peers netdevice, for active  soc delete only mlo peers netdevice */
        if (grp_soc_list[soc_idx]->wsi_remap_remove)
            mlo_peer_only = 0;
        else
            mlo_peer_only = 1;
        wlan_objmgr_iterate_obj_list(grp_soc_list[soc_idx], WLAN_PEER_OP,
                            ath_vdev_recover_wds_peer_delete_handler, &mlo_peer_only, 1,
                            WLAN_MLME_NB_ID);
    }
#endif
    if (psoc->wsi_remap_remove) {
        /* flush all peers from bypassed SOC */
        wlan_objmgr_iterate_obj_list(psoc, WLAN_VDEV_OP,
                                     wlan_recovery_vdev_delete_peers,
                                     NULL, 1, WLAN_MLME_NB_ID);
    }

    /* Delete all MLO peers peers associated across ML participating SOCs*/
    ol_ath_delete_all_mlo_peers_wsi(soc);

    /* Teardown MLO on all targets */
    if (mlo_link_teardown_link(psoc, grp_id,
                                WMI_HOST_MLO_TEARDOWN_REASON_DYNAMIC_WSI_REMAP) != QDF_STATUS_SUCCESS) {
         /* MLO teardown completeion not recieved within time duration */
         qdf_err("Failed to teardown MLO, Dynamic WSI REMAP failed !");
         return QDF_STATUS_E_FAILURE;
    }

    for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
        if (!grp_soc_list[soc_idx]) {
            continue;
        }
        temp_soc = lmac_get_psoc_feature_ptr(grp_soc_list[soc_idx]);
        /* Perform vdev delete on the soc that is going into bypass which will inturn do power down */
        if (grp_soc_list[soc_idx]->wsi_remap_remove) {
            wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP, ath_vdev_delete,
                                                         NULL, 1, WLAN_MLME_NB_ID);
        } else {
            /* Perform soc down for non bypass chips */
            mlo_setup_update_soc_down(grp_soc_list[soc_idx], grp_id);
        }
    }
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_dynamic_wsi_remap_mlo_setup(ol_ath_soc_softc_t *soc)
{
    uint8_t chip_idx;
    uint8_t grp_id;
    uint8_t total_grp;
    uint8_t tot_grp_socs;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    struct mlo_setup_info *setup_info;
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
    struct wlan_objmgr_psoc *tmp_psoc;

    if (!mlo_ctx) {
        qdf_err("MLO CTX is null");
        return QDF_STATUS_E_FAILURE;
    }

    if (!psoc) {
        qdf_err("PSOC is null");
        return QDF_STATUS_E_FAILURE;
    }

    qdf_info("Trigger DYNAMIC  WSI REMAP MLO SETUP ");
    total_grp = pld_get_num_mlo_groups();

    if (total_grp <= 0 || total_grp > WLAN_MAX_MLO_GROUPS) {
       qdf_info("MLO not supported due to total groups %d is not valid !",
               total_grp);
       return QDF_STATUS_E_INVAL;
    }
    /* Trigger mlo capabilites updates */
    __qca_ol_update_mlo_capability(true,psoc->wsi_remap_recovery_in_progress);

    grp_id = ol_ath_mlo_get_group_id(psoc);
    tot_grp_socs = mlo_setup_get_total_socs(grp_id);

    if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
         return QDF_STATUS_E_INVAL;

    setup_info = &mlo_ctx->setup_info[grp_id];

    if (!setup_info->tot_socs)
        return QDF_STATUS_E_INVAL;

    mlo_ctx->dynamic_wsi_bypassed = false;
    /* Update the number of active links */
    for (chip_idx = 0; chip_idx < ol_num_global_soc; chip_idx++) {
        ol_ath_soc_softc_t *temp_soc = ol_global_soc[chip_idx];
        if (!temp_soc) {
            continue;
        }
        if (!temp_soc->psoc_obj->wsi_remap_remove) {
            ol_ath_ml_link_update(temp_soc);
        }

        /* Iterate and mark if any of the chip is bypassed */
        if (temp_soc->psoc_obj->wsi_remap_remove)
            mlo_ctx->dynamic_wsi_bypassed = true;
    }

    /* Trigger SOC ready and pdev ready for active soc, it will perform MLO setup */
    for (chip_idx = 0; chip_idx < ol_num_global_soc; chip_idx++) {
        ol_ath_soc_softc_t *temp_soc = ol_global_soc[chip_idx];

        if (!temp_soc) {
            continue;
        }
        tmp_psoc = temp_soc->psoc_obj;
        if (!tmp_psoc) {
            continue;
        }

        if ((!temp_soc->psoc_obj->wsi_remap_remove)) {
            tmp_psoc->wsi_remap_recovery_in_progress = false;
            init_deinit_mlo_update_soc_ready(tmp_psoc);
            init_deinit_mlo_update_pdev_ready(tmp_psoc, ol_num_global_soc);
        }
    }

    return QDF_STATUS_SUCCESS;
}
#endif

QDF_STATUS ol_ath_mlo_trigger_mode1_recovery(struct recovery_soc_info *soc_info,
                                             struct ol_ath_soc_softc *soc)
{
    uint8_t soc_idx;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
#ifdef QCA_SUPPORT_WDS_EXTENDED
    uint8_t mlo_peer_only = 0;
#endif

    qdf_info("Trigger mode1 Recovery");

    soc->mode1_recov_inprogress = 1;

    /* cleanup mlo setup info associated with asserted SOC */
    mlo_cleanup_asserted_soc_setup_info(psoc, soc_info->grp_id);

    /* teardown MLO on other targets */
    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
         if ((soc_info->curr_soc_list[soc_idx] == psoc) || (!soc_info->curr_soc_list[soc_idx]))
             continue;
         if (mlo_link_teardown_link(soc_info->curr_soc_list[soc_idx], soc_info->grp_id,
                                    WMI_HOST_MLO_TEARDOWN_REASON_MODE1_SSR) != QDF_STATUS_SUCCESS) {
             /* MLO teardown completeion not recieved within time duration. switch to mode 0 behaviour */
             qdf_err("Failed to teardown MLO, switch to mode0");
             return QDF_STATUS_E_FAILURE;
         }

    }

#ifdef QCA_SUPPORT_WDS_EXTENDED
    if (!soc->retain_mlo_peer) {
        for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
             if (soc_info->curr_soc_list[soc_idx]) {
                 /* For asserted soc delete all peers netdevice, for non asserted soc delete only mlo peers netdevice */
                 mlo_peer_only = (soc_info->curr_soc_list[soc_idx] != psoc) ? 1 : 0;
                 wlan_objmgr_iterate_obj_list(soc_info->curr_soc_list[soc_idx], WLAN_PEER_OP,
                                     ath_vdev_recover_wds_peer_delete_handler, &mlo_peer_only, 1,
                                     WLAN_MLME_NB_ID);
             }
        }
    } else {
       /* For asserted SoC delete all peers SLO/LEGACY/MLO netdevices */
        mlo_peer_only = 0;
        wlan_objmgr_iterate_obj_list(psoc, WLAN_PEER_OP,
                             ath_vdev_recover_wds_peer_delete_handler, &mlo_peer_only, 1,
                              WLAN_MLME_NB_ID);
    }
#endif

    /* flush all peers from asserted SOC */
    wlan_objmgr_iterate_obj_list(psoc, WLAN_VDEV_OP,
                                 wlan_recovery_vdev_delete_peers,
                                 NULL, 1, WLAN_MLME_NB_ID);

    /* API to delete all mlo peers from all Non-Asserted partner links */
    if (!soc->retain_mlo_peer)
        ol_ath_delete_all_mlo_peers(soc_info, soc);

    return QDF_STATUS_SUCCESS;
}


#define MLO_RECOVERY_FALLBACK_TIMEOUT 20000
void ol_ath_check_if_other_tgt_start_inprogress(struct recovery_soc_info *soc_info,
                                                struct ol_ath_soc_softc *soc)
{
   ol_ath_soc_softc_t *temp_soc;
   QDF_STATUS status;
   uint8_t soc_idx;

   for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
        temp_soc = lmac_get_psoc_feature_ptr(soc_info->def_soc_list[soc_idx]);
        if (!temp_soc || temp_soc == soc)
            continue;
        if (temp_soc->recovery_in_progress && temp_soc->mode1_recov_inprogress && temp_soc->target_ready) {
            qdf_info("Target ready inprogress/wait for completion for Psoc id %d",
                     temp_soc->psoc_obj->soc_objmgr.psoc_id);
            status = qdf_wait_for_event_completion(
                      &temp_soc->fallback_event,
                      MLO_RECOVERY_FALLBACK_TIMEOUT);
            if (QDF_IS_STATUS_ERROR(status)) {
                qdf_err("Target ready not complete for Psoc id %d",
                         temp_soc->psoc_obj->soc_objmgr.psoc_id);
            }

        }
   }

}

bool ol_ath_check_mlo_recovery_count(struct recovery_soc_info *soc_info,
                                     struct wlan_objmgr_psoc *psoc)
{
    ol_ath_soc_softc_t *temp_soc;
    uint8_t soc_idx;

    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
         if (!soc_info->def_soc_list[soc_idx])
             continue;
         temp_soc = lmac_get_psoc_feature_ptr(soc_info->def_soc_list[soc_idx]);
         if (temp_soc && temp_soc->recovery_count) {
              return true;
         }
    }

   return false;
}

/*  Entire API is called with rtnl_lock held, hence additional or seperate locks
 *  not required
 */
void ol_ath_wifi_mlo_recovery_remove(struct recovery_soc_info *soc_info,
                                     struct platform_device *pdev)
{
   struct net_device *dev = platform_dev_to_net_dev(pdev);
   ol_ath_soc_softc_t *soc;
   bool fallback_mode0 = false;

   soc = ath_netdev_priv(dev);
   if (!soc) {
       qdf_err("SOC is Null for MLO Recovery");
       QDF_BUG(0);
   }

   if (mlo_is_ml_soc(soc->psoc_obj, soc_info->grp_id)) {
       soc->target_ready = 0;
       qdf_info("Recovery FW of type %d", soc->recovery_type);

       if (soc->recovery_type == MLO_RECOVERY_MODE_1) {
           if (ol_ath_mlo_trigger_mode1_recovery(soc_info, soc))
               fallback_mode0 = true;
       }

       if (((soc->recovery_type == MLO_RECOVERY_MODE_0) &&
           (soc->mlo_partner_hang_issued == false)) || fallback_mode0) {
           if (fallback_mode0) {
               ol_ath_set_recovery_mode_for_all(soc_info, soc->psoc_obj, MLO_RECOVERY_MODE_0);
               soc->mode1_recov_inprogress = 0;
           }

           ol_ath_check_if_other_tgt_start_inprogress(soc_info, soc);
           ol_ath_mlo_trigger_mode0_recovery(soc_info, soc->psoc_obj);
           soc->mlo_partner_hang_issued = true;
           ol_ath_mlo_crash_partner_chips(soc_info, soc->psoc_obj);
       }

        pld_set_recovery_mode(&pdev->dev, soc->recovery_type);
   }
}

void ol_ath_mlo_set_target_in_reset(struct recovery_soc_info *soc_info,
                                    ol_ath_soc_softc_t *soc)
{
    struct pdev_op_args oper;
    ol_ath_soc_softc_t *temp_soc;
    uint8_t soc_idx;

    /* API sets all the targets in Mode0 recovery and
       issue recovery in progress */

   for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
        temp_soc = lmac_get_psoc_feature_ptr(soc_info->def_soc_list[soc_idx]);
        if (temp_soc) {
             temp_soc->recovery_type = MLO_RECOVERY_MODE_0;

             temp_soc->recovery_in_progress = 1;

             temp_soc->target_status = OL_TRGET_STATUS_RESET;
         }
   }

   oper.type = PDEV_ITER_TARGET_ASSERT;
   wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                    wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
   /* Per Radio tgt_asserts is incremented above, increment for the SOC here */
   soc->soc_stats.tgt_asserts++;

}
#endif

void ol_ath_set_target_in_reset(ol_ath_soc_softc_t *soc)
{
    struct pdev_op_args oper;

    oper.type = PDEV_ITER_TARGET_ASSERT;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                     wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

    soc->recovery_in_progress = 1;

    soc->target_status = OL_TRGET_STATUS_RESET;
    /* Per Radio tgt_asserts is incremented above, increment for the SOC here */
    soc->soc_stats.tgt_asserts++;
}

void ol_ath_trigger_recover_create(struct ol_ath_soc_softc *soc)
{
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
   ol_ath_soc_softc_t *temp_soc;
   struct wlan_objmgr_psoc *grp_soc_list[MAX_MLO_CHIPS];
   uint8_t soc_idx;
   uint32_t grp_id;
   uint32_t tot_grp_socs = 0;

   if ((ol_ath_mlo_capable(soc->psoc_obj)) &&
       (soc->recovery_type == MLO_RECOVERY_MODE_0)) {
       grp_id = ol_ath_mlo_get_group_id(soc->psoc_obj);

       tot_grp_socs = mlo_setup_get_total_socs(grp_id);
       if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
             return;

        mlo_get_soc_list(grp_soc_list, grp_id, tot_grp_socs, WLAN_MLO_GROUP_DEFAULT_SOC_LIST);
       /* In case of MLD, check all the other soc targets are started
          before creating the vaps, as in target recovery case, we receive
          target start followed by vap create/start for each soc seperately*/

       for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
            temp_soc = lmac_get_psoc_feature_ptr(grp_soc_list[soc_idx]);
            if (temp_soc && ol_ath_mlo_capable(temp_soc->psoc_obj))
                if (temp_soc->down_complete && temp_soc->mlo_partner_hang_issued) {
                    return;
                }
       }

       for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
           temp_soc = lmac_get_psoc_feature_ptr(grp_soc_list[soc_idx]);
           if (temp_soc && ol_ath_mlo_capable(temp_soc->psoc_obj)) {
              struct pdev_op_args oper;
              oper.type = PDEV_ITER_RECOVERY_PROBE;
              wlan_objmgr_iterate_obj_list(temp_soc->psoc_obj, WLAN_PDEV_OP,
                          wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
           }
       }
   } else
#endif
        {
                 struct pdev_op_args oper;
                 oper.type = PDEV_ITER_RECOVERY_PROBE;
                 wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                             wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
        }
}

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
void ol_ath_mlo_trigger_recover_start(struct recovery_soc_info *soc_info,
                                  struct ol_ath_soc_softc *soc)
{
   ol_ath_soc_softc_t *temp_soc;
   uint8_t soc_idx;

       if (soc->recovery_type == MLO_RECOVERY_MODE_0) {

               /* In case of MLD, check all the other soc targets are started
                  before creating the vaps, as in target recovery case, we receive
                  target start followed by vap create/start for each soc seperately*/

           for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
                temp_soc = lmac_get_psoc_feature_ptr(soc_info->def_soc_list[soc_idx]);
                if (temp_soc && ol_ath_mlo_capable(temp_soc->psoc_obj))
                    if (temp_soc->down_complete && temp_soc->mlo_partner_hang_issued) {
                        return;
                    }
           }

           for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
                temp_soc = lmac_get_psoc_feature_ptr(soc_info->def_soc_list[soc_idx]);
                if (temp_soc && ol_ath_mlo_capable(temp_soc->psoc_obj)) {
                    struct pdev_op_args oper;
                   /* Send event to user space */
                   oper.type = PDEV_ITER_POWERUP;
                   wlan_objmgr_iterate_obj_list(temp_soc->psoc_obj, WLAN_PDEV_OP,
                               wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
                }
           }
       } else {
           struct pdev_op_args oper;

           /* Send event to user space */
           oper.type = PDEV_ITER_POWERUP;
           wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                                        wlan_pdev_operation, &oper, 0,
                                        WLAN_MLME_NB_ID);
       }

       if (ol_ath_recover_work_queue_reset()) {
           qdf_err("Unable to reset recovery Work queue, disabling recovery");

           for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
                temp_soc = lmac_get_psoc_feature_ptr(soc_info->def_soc_list[soc_idx]);
                if (temp_soc)
                    temp_soc->recovery_enable = 0;
           }

           ol_ath_recover_work_queue_deinit();
       }
}
#endif

void ol_ath_trigger_recover_start(struct ol_ath_soc_softc *soc)
{
       struct pdev_op_args oper;

       /* Send event to user space */
       oper.type = PDEV_ITER_POWERUP;
       wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                   wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
}

#ifdef CONFIG_CNSS2_SUPPORT
int ol_ath_wifi_ssr(void *bdev, struct platform_device_id *id,
                    unsigned long code, enum pld_bus_type bus_type)
{
    struct ol_ath_soc_softc *soc = NULL;
    struct platform_device *pdev = bdev;
    int ret = -1;
    int recovery_enable = 0;
    int recovery_in_progress = 0;
    struct net_device *dev;
    struct hif_opaque_softc *hif_hdl = NULL;
    struct pdev_op_args oper;
    bool delay = false;
    struct pci_dev *pcidev = NULL;
    const struct pci_device_id *pci_dev_id = NULL;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    struct recovery_soc_info soc_info = {0};
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
#endif

    qdf_info("SSR event %lu bus_type %d", code, bus_type);

    if (!pdev) {
        qdf_err("pdev null\n");
        return NOTIFY_DONE;
    }

    /*
     * 'dev' and 'soc' are only valid if the driver is already
     * in initialized. The first time they are NULL.
     */

#ifdef CONFIG_OF
    dev = platform_dev_to_net_dev(pdev);
#else
    if(pdev->id_entry) {
        dev = platform_dev_id_to_net_dev(pdev, pdev->id_entry);
    }
    else {
        dev = platform_dev_id_to_net_dev(pdev, id);
    }
#endif

    if (dev) {
        soc = ath_netdev_priv(dev);
        if (soc) {
            recovery_enable = soc->recovery_enable;
            recovery_in_progress = soc->recovery_in_progress;
            hif_hdl = lmac_get_hif_hdl(soc->psoc_obj);
        } else {
            qdf_print("Invalid soc pointer");
            return NOTIFY_DONE;
        }
    }

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (recovery_enable && ol_ath_mlo_capable(soc->psoc_obj)) {
        soc_info.grp_id = ol_ath_mlo_get_group_id(soc->psoc_obj);
        soc_info.tot_grp_socs = mlo_setup_get_total_socs(soc_info.grp_id);

        if (!soc_info.tot_grp_socs || (soc_info.tot_grp_socs > MAX_MLO_CHIPS)) {
             qdf_err("WIFI-SSR : Failed for state(%ld) as total mlo group socs is  %d",
                      code, soc_info.tot_grp_socs);
             return NOTIFY_DONE;
        }

        mlo_get_soc_list(soc_info.def_soc_list, soc_info.grp_id, soc_info.tot_grp_socs,
                         WLAN_MLO_GROUP_DEFAULT_SOC_LIST);
        mlo_get_soc_list(soc_info.curr_soc_list, soc_info.grp_id, soc_info.tot_grp_socs,
                         WLAN_MLO_GROUP_CURRENT_SOC_LIST);
    }
#endif

#ifndef BUILD_X86
    /* Now handle the notification event from SSR */
    switch (code) {
    case CNSS_BEFORE_SHUTDOWN:
#if QLD
        /* Check if live dump is enabled */
        if (is_qld_enable()) {
            if (soc && soc->recovery_in_progress) {
                /*
                 * Wait till application completes
                 * collection of memory registered for debug
                 */
                qld_dump_wait(soc);
            }
        }
#endif

        if(testmode == PLD_WALTEST) {
            return NOTIFY_DONE;
        }
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (soc && soc->nss_soc.ops) {
            qdf_print("nss-wifi: soc_stop soc->soc_idx: %d ", soc->soc_idx);
            soc->nss_soc.ops->nss_soc_wifi_stop(soc);
        } else if (soc == NULL) {
            qdf_print("Invalid soc pointer");
            return NOTIFY_DONE;
        }
#endif

        if (wifi3_ko_exit_in_progress) {
            if (bus_type == PLD_BUS_TYPE_AHB || bus_type == PLD_BUS_TYPE_AHB_FW_SIM) {
                ol_ath_ahb_remove(pdev);
            } else if (bus_type == PLD_BUS_TYPE_PCIE || bus_type == PLD_BUS_TYPE_PCIE_FW_SIM) {
                pcidev = pld_get_pci_dev_from_plat_dev((void *)pdev);
                ol_ath_pci_remove(pcidev);
            } else {
                qdf_err("Unknown bus_type %d \n", bus_type);
            }
        } else {

            if (!soc)
                 break;

            if (soc->down_complete) {
                /* Target already down */
                return NOTIFY_DONE;
            }
            /* Current Hawkeye FW is common for all Radios. Any FW assert on a SOC,
             * needs update to all SCNs associated with it.
             */
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
            if (ol_ath_mlo_capable(soc->psoc_obj)) {
                soc->recovery_type = get_recovery_mode(soc);

            if (mlo_ctx && mlo_ctx->dynamic_wsi_bypassed) {
                soc->recovery_type = MLO_RECOVERY_MODE_0;
                qdf_info("If a CHIP is bypassed via WSI remap only MODE0 Recovery is supported");
            }
               if (soc->recovery_type == MLO_RECOVERY_MODE_0)
                   ol_ath_mlo_set_target_in_reset(&soc_info, soc);
               else
                   ol_ath_set_target_in_reset(soc);
            } else
#endif
                ol_ath_set_target_in_reset(soc);

#ifdef IPA_OFFLOAD
           if (ucfg_ipa_is_enabled()) {
               struct wlan_objmgr_pdev *pdev_obj = NULL;

               pdev_obj = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, 0, WLAN_MLME_SB_ID);
               if (pdev_obj) {
                   ucfg_ipa_uc_force_pipe_shutdown(pdev_obj);
                   ucfg_ipa_uc_ssr_cleanup(pdev_obj);
                   wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_SB_ID);
               }
           }
#endif

            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "Resetting the radio\n");
            /* Take the rtnl_lock, so that there is no user intervention while
             * we recover.*/
            rtnl_lock();
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
             if (ol_ath_mlo_capable(soc->psoc_obj))
                 ol_ath_wifi_mlo_recovery_remove(&soc_info, pdev);
#endif
            ol_ath_wifi_recovery_remove(pdev);
            rtnl_unlock();
        }
        break;
    case CNSS_AFTER_SHUTDOWN:
        break;
    case CNSS_RAMDUMP_NOTIFICATION:
        /* Upload PKTLOG data file to TFTP */
#if !(defined REMOVE_PKT_LOG) && (defined PKTLOG_DUMP_UPLOAD_SSR)
        ol_ath_pktlog_dump_upload_ssr(soc);
#endif
        /* Send the FWDUMP_READY event to acfg daemon.
         * If hotplug is disabled, then acfg daemon will collect the dump
         */
#if UMAC_SUPPORT_ACFG_RECOVERY
        /* Just send the event on 1st scn. The RAMDUMP is common for a soc */
        if (soc) {
            struct wlan_objmgr_pdev *pdev_obj;
            struct ieee80211com *ic;

            pdev_obj = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, 0, WLAN_MLME_SB_ID);
            if (pdev_obj == NULL) {
                qdf_print("pdev object is NULL in RAMDUMP notification");
                break;
            }
            ic = wlan_pdev_get_mlme_ext_obj(pdev_obj);
            if (ic) {
                OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_WDT_FWDUMP_READY);
#if QCMAP_SUPPORT
                OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_QCMAP_FW_DUMP_READY);
#endif
            }
            else {
                qdf_err("ic object is NULL in RAMDUMP notification");
            }

            wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_SB_ID);
        }
#endif
        break;
    case CNSS_RAMDUMP_DONE:
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
        if (soc)
            soc->target_ready = 1;

        if (mlo_ctx && mlo_ctx->dynamic_wsi_bypassed)
            soc->psoc_obj->wsi_remap_recovery_in_progress = true;
#endif
        if (ol_ath_recover_start(soc)) {
            qdf_err("Failed to start target");
            QDF_BUG(0);
        }

        break;
    case CNSS_PREPARE_FOR_FATAL_SHUTDOWN:
        /* Unlike other notification, this event is sent from
         * interrupt context, immediately after the Target Asserts.
         * So do minimal and let the BEFORE_SHUDOWN do the rest
         */
#ifdef CONFIG_QCA_MINIDUMP
        /* check if mini dump feature is enabled */
        if (soc && !cfg_get(soc->psoc_obj, CFG_OL_DISABLE_MINIDUMP_FW_ASSERT))
            do_minidump();
        /* Triggers a hotplug event to collect snapshot of data structres
         * registered with minidump framework.
         */
#endif

#if QLD
        /* check if live dump feature is enabled */
        if (is_qld_enable()) {
            /*
             * collect debug memory that is registered with
             * framework . The registered memory is maintained
             * as linked list, this routine traverses the list and
             * sends it to application
             */
            qld_process_list(soc);
        }
#endif
        if (!wifi3_ko_exit_in_progress) {
            /*
             * This is the Q6FW crash case.
             * set the Targt status to reset and disable interrupts
             * so that no more interrupts are serviced.
             * Also stop tx queue of every VAP.
             */
            if (hif_hdl && soc) {

                if (recovery_enable)
                    ol_ath_set_logs_level_while_recovery(soc, 0);

                /* CE RING DATA DUMP WHEN TARGET ASSERT */
                hif_display_stats((struct hif_opaque_softc *)hif_hdl);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
                if (ol_ath_mlo_capable(soc->psoc_obj)) {
                     soc->fallback_mode0 = ol_ath_check_mlo_grp_recovery(soc->psoc_obj, 0);
                     if (soc->fallback_mode0 || ol_ath_check_mlo_recovery_count(&soc_info, soc->psoc_obj)) {
                         if (cfg_get(soc->psoc_obj, CFG_OL_DISABLE_MULTI_RECOVERY)) {
                             qdf_err("Already SSR in progress, Follow up recovery disabled!");
                             recovery_enable = 0;
                         } else {
                             qdf_err("Already SSR in progress, Switch to mode0");
                         }

                     }
                }
#endif
                if (!recovery_enable){
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
                if (ol_ath_mlo_capable(soc->psoc_obj))
                    pld_set_recovery_mode(&pdev->dev, MLO_RECOVERY_MODE_0);
#endif
                    qdf_err("[%s]: XXX TARGET ASSERTED XXX",
                              soc->sc_osdev->netdev->name);
                    /* crash the system */
                    qdf_err("***Recovery is not enabled***");
                    qdf_target_assert_always(0);
                }

                /* Set Target status to RESET */
                hif_set_target_status((struct hif_opaque_softc *)hif_hdl, TARGET_STATUS_RESET);

                /* Stop tx queue of VAPs*/
                ol_ath_tx_stop(soc);

                /* stop target_if timers */
                ol_ath_target_timer_stop(soc);

                /* Disable interrupts */
                hif_nointrs(HIF_GET_SOFTC(hif_hdl));

                /* Notify Asserted SoC to UMAC Reset */
                ol_ath_notify_asserted_soc(soc);

                /* Handle pdev level operation.
                 * This will set ic level recovery flag
                 * and also calls wmi_stop for each pdev
                 */
                oper.type = PDEV_ITER_FATAL_SHUTDOWN;
                wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                        wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

                /* set soc level flag */
                soc->recovery_in_progress = 1;
                soc->target_status = OL_TRGET_STATUS_RESET;
            } else {
                /* Assert if Early Target Assert is detected, except in
                 * WALTEST Mode. In WALTEST mode, Q6 dump would be collected
                 * and target will be powered on again in WALTEST mode
                 */
                if (testmode != PLD_WALTEST) {
                    qdf_err("Early assert cannot be handled");
                    cnss_dump_qmi_history();
                    qdf_target_assert_always(0);
                }
                return NOTIFY_DONE;
            }
        }

        break;
    case CNSS_BEFORE_POWERUP:
        break;
    case CNSS_POWERUP_FAILURE:
        if (!soc)
            break;
        /* Currently, there is no recovery from here.
         * But we still need to flag this, so that other Driver routines
         * shouldn't process.
         * So set recovery_in_progress to true, which is honoured by other
         * Driver routines.
         */
        oper.type = PDEV_ITER_FATAL_SHUTDOWN;
        wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                        wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

        /* set soc level flag */
        soc->recovery_in_progress = 1;
        break;
    case CNSS_AFTER_POWERUP:
    {
#if UMAC_SUPPORT_ACFG_RECOVERY
        struct wlan_objmgr_pdev *pdev_obj;
        struct ieee80211com *ic;
#endif
        if(soc) {
            delay = soc->delay_bug_on;
            if (delay) {
                /* We had reference leak in recovery now trigger assert*/
                qdf_err("Trigger Bug_ON due to reference leak");
                BUG_ON(1);
            }
        }

        if (recovery_enable && recovery_in_progress) {
            rtnl_lock();
            ret = ol_ath_wifi_recovery_probe(pdev, NULL);
            rtnl_unlock();
            if (ret != 0) {
                qdf_print("Recovery probe failed ");
#if UMAC_SUPPORT_ACFG_RECOVERY
                pdev_obj = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, 0, WLAN_MLME_NB_ID);
                if (pdev_obj == NULL) {
                    qdf_print("pdev object is NULL in AFTER POWERUP notification");
                    break;
                }
                ic = wlan_pdev_get_mlme_ext_obj(pdev_obj);
                if (ic) {
                    OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_WDT_INIT_FAILED);
                }
                else {
                    qdf_err("ic object is NULL in RAMDUMP notification");
                }
                wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_NB_ID);
#endif
                break;
            }

            soc->recovery_in_progress = 0;
            soc->target_status = OL_TRGET_STATUS_CONNECTED;

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
           if (ol_ath_mlo_capable(soc->psoc_obj))
               ol_ath_mlo_trigger_recover_start(&soc_info, soc);
           else
#endif
               ol_ath_trigger_recover_start(soc);

            ol_ath_set_logs_level_while_recovery(soc, 1);
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
            soc->recovery_count++;
            soc->mode1_recov_inprogress = 0;
            qdf_event_set(&soc->fallback_event);
            pld_set_recovery_mode(&pdev->dev, MLO_RECOVERY_MODE_0);
#endif
        } else {
            qdf_info("Waiting for probe lock");
            qdf_mutex_acquire(&probe_lock);
            qdf_info("Starting Driver Probe");
            if (bus_type == PLD_BUS_TYPE_AHB || bus_type == PLD_BUS_TYPE_AHB_FW_SIM) {
                if(!pld_is_dev_initialized(&pdev->dev)) {
                    ol_ath_ahb_probe(pdev, id);
                }
            } else if (bus_type == PLD_BUS_TYPE_PCIE || bus_type == PLD_BUS_TYPE_PCIE_FW_SIM) {
                if(!pld_is_dev_initialized(&pdev->dev)) {
                    pcidev = pld_get_pci_dev_from_plat_dev((void *)pdev);
                    pci_dev_id = pld_get_pci_dev_id_from_plat_dev((void *)pdev);
                    ol_ath_pci_probe(pcidev, pci_dev_id);
                    pld_set_recovery_mode(&pdev->dev, MLO_RECOVERY_MODE_0);
                }
            } else {
                qdf_err("Unknown bus_type %d \n", bus_type);
            }
            qdf_mutex_release(&probe_lock);
        }
    }
        break;
    default:
	break;
    }
#endif

    return NOTIFY_DONE;
}
#endif

int ol_ath_do_waltest(struct device *dev)
{
	qdf_info("Sending QMI mode as waltest(%d)\n", PLD_WALTEST);
	if(pld_wlan_enable(dev, NULL, PLD_WALTEST)) {
		qdf_err("%s Error in pld_wlan_enable",__func__);
		goto err;
	}

	return 0;
err:
	qdf_err("Error while doing waltest\n");
	return -EINVAL;
}

int ol_ath_do_cold_boot_calibration(struct device *dev)
{
	if (pld_wlan_enable(dev, NULL, testmode)) {
		qdf_info("Error while doing cold boot calibration\n");
		return -EINVAL;
	}

	return 0;
}

int
ol_ath_ahb_probe(struct platform_device *pdev, const struct platform_device_id *id)
{

    struct ol_attach_t ol_cfg;
    void *hif_context = NULL;
    struct hif_target_info *tgt_info = NULL;
    struct _NIC_DEV *aps_osdev = NULL;
    qdf_device_t  qdf_dev = NULL;    /* qdf handle */
#ifdef CONFIG_CNSS2_SUPPORT
    struct pld_plat_data *cnss_plat, *wlan_sc;
#endif /* CONFIG_CNSS2_SUPPORT */
    int ret = 0;
    ol_ath_soc_softc_t *soc;
    bool flag = false;
    struct pdev_op_args oper;

    if (!ol_ath_supported_dev(id)) {
        qdf_print("Unsupported device");
        ret = -EIO;
        goto err_notsup;
    }

    pld_wait_for_fw_ready(&pdev->dev);

    if (((id->driver_data == QCA8074_DEVICE_ID) ||
         (id->driver_data == QCA8074V2_DEVICE_ID) ||
         (id->driver_data == QCA9574_DEVICE_ID) ||
         (id->driver_data == QCA5018_DEVICE_ID) ||
         (id->driver_data == QCN6122_DEVICE_ID) ||
         (id->driver_data == QCN9160_DEVICE_ID) ||
         (id->driver_data == QCA5332_DEVICE_ID) ||
         (id->driver_data == QCA6018_DEVICE_ID) ||
         (id->driver_data == QCN6432_DEVICE_ID)) &&
        testmode == PLD_WALTEST) {
        ret = ol_ath_do_waltest(&pdev->dev);
        return ret;
    }

    aps_osdev = qdf_mem_malloc(sizeof(*aps_osdev));
    if (aps_osdev == NULL) {
        qdf_print("mem alloc failure at line:%d", __LINE__);
        ret = -ENOMEM;
        goto err_notsup;
    }
    OS_MEMSET(aps_osdev, 0, sizeof(*(aps_osdev)));
    aps_osdev->bdev = pdev;
    aps_osdev->device = &pdev->dev;
    aps_osdev->bc.bc_bustype = QDF_BUS_TYPE_AHB;

    /* initialize the qdf_dev handle */
    qdf_dev = qdf_mem_malloc(sizeof(*(qdf_dev)));
    if (qdf_dev == NULL) {
        qdf_print("mem alloc failure at line:%d", __LINE__);
        ret = -ENOMEM;
        goto cleanup_aps_dev;
    }

    OS_MEMSET(qdf_dev, 0, sizeof(*(qdf_dev)));
    qdf_dev->drv_hdl = NULL; /* bus handle */
    qdf_dev->dev = &pdev->dev; /* device */
    qdf_dev->drv = aps_osdev;

    hif_context  = (void *)ol_hif_open(&pdev->dev, (void *)pdev, (void *)id, QDF_BUS_TYPE_AHB, 0,qdf_dev);
    if(hif_context == NULL) {
        qdf_nofl_info("Error in ol_hif_open\n");
        ret = -ENOMEM;
        goto cleanup_adf_dev;
    }

    tgt_info = hif_get_target_info_handle(hif_context);
    if(!tgt_info) {
        ret = -EIO;
        goto err;
    }

    ol_cfg.devid = id->driver_data;
    ol_cfg.bus_type = BUS_TYPE_AHB;
    ol_cfg.target_type = tgt_info->target_type;
    ol_cfg.platform_devid = (void *)id;
    ol_cfg.vendor_id = 0x00000000; /* There is no vendorid for AHB devices,
                                      hence assigning 0x00000000 */
#ifndef CONFIG_OF
    pdev->id_entry = (void *)id;
#endif
    ret = __ol_ath_attach(hif_context, &ol_cfg, aps_osdev, qdf_dev);
    if(ret != 0) {
        ret = -EIO;
        goto cleanup_hif_context;
    }

    soc = ath_netdev_priv(aps_osdev->netdev);

    oper.type = PDEV_ITER_PDEV_ENTRY_ADD;
    oper.ret_val = PDEV_ITER_STATUS_INIT;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                     wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
    if (oper.ret_val == PDEV_ITER_STATUS_OK) {
        flag = true;
    } else if (oper.ret_val == PDEV_ITER_STATUS_FAIL) {
        flag = true;
        goto err_attach;
    }

    ol_ath_diag_user_agent_init(soc);
#ifdef CONFIG_CNSS2_SUPPORT
    if ((id->driver_data == QCA8074_DEVICE_ID) ||
        (id->driver_data == QCA8074V2_DEVICE_ID) ||
        (id->driver_data == QCA9574_DEVICE_ID) ||
        (id->driver_data == QCA6018_DEVICE_ID) ||
        (id->driver_data == QCA5332_DEVICE_ID) ||
        (id->driver_data == QCN6122_DEVICE_ID) ||
        (id->driver_data == QCN9160_DEVICE_ID) ||
        (id->driver_data == QCA5018_DEVICE_ID) ||
        (id->driver_data == QCN6432_DEVICE_ID)) {
        wlan_sc = qdf_mem_malloc(sizeof(struct pld_plat_data));
        if(!wlan_sc) {
            qdf_info("memory allocation fails at cnss_plat_data allocation");
            goto err_attach;
        }
        wlan_sc->wlan_priv = (void *)aps_osdev->netdev;
        cnss_plat = platform_get_drvdata(pdev);
        cnss_plat->wlan_priv = wlan_sc;
    } else {
        platform_set_drvdata(pdev, aps_osdev->netdev);
    }
#endif

    return 0;

err_attach:
    if (flag) {
       oper.type = PDEV_ITER_PDEV_ENTRY_DEL;
       wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                         wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
    }


err:
cleanup_hif_context:
    if (hif_context) {
        hif_disable_isr(hif_context);
        ol_hif_close(hif_context);
    }
cleanup_adf_dev:
    qdf_mem_free(qdf_dev);
cleanup_aps_dev:
    qdf_mem_free(aps_osdev);
err_notsup:
    qdf_info("failure ret %d", ret);
#ifdef CONFIG_WIFI_EMULATION_WIFI_3_0
    if (cfg_get(soc->psoc_obj, CFG_OL_EMULATION)) {
        qdf_info("SPINNING for host debugging \n");
        while(1);
    }
#endif
    qdf_target_assert_always(0);
    return ret;
}
qdf_export_symbol(ol_ath_ahb_probe);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
void ol_ath_delete_all_mlo_peers(struct recovery_soc_info *soc_info,
                                 struct ol_ath_soc_softc *soc)
{
    uint8_t soc_idx;
    uint8_t mlo_only = 1;

    /* delete all mlo peers associated across ML participating SOCs */
    for (soc_idx = 0; soc_idx < (soc_info->tot_grp_socs); soc_idx++) {
         if ((!soc_info->curr_soc_list[soc_idx]) || (lmac_get_psoc_feature_ptr(soc_info->curr_soc_list[soc_idx]) == soc))
              continue;

         wlan_objmgr_iterate_obj_list(soc_info->curr_soc_list[soc_idx], WLAN_PEER_OP,
                                      ath_vdev_recover_peer_deauth_handler, &mlo_only, 1,
                                      WLAN_MLME_NB_ID);
     }

}

#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP
void ol_ath_delete_all_mlo_peers_wsi(struct ol_ath_soc_softc *soc)
{
    struct wlan_objmgr_psoc *grp_soc_list[MAX_MLO_CHIPS];
    uint8_t soc_idx;
    uint8_t grp_id;
    uint8_t tot_grp_socs;
    uint8_t mlo_only = 1;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;

    grp_id = ol_ath_mlo_get_group_id(psoc);

    tot_grp_socs = mlo_setup_get_total_socs(grp_id);
    if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
         return;

    mlo_get_soc_list(grp_soc_list, grp_id, tot_grp_socs, WLAN_MLO_GROUP_CURRENT_SOC_LIST);

    /* delete all mlo peers associated across ML participating SOCs */
    for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
         if ((!grp_soc_list[soc_idx]))
              continue;

         wlan_objmgr_iterate_obj_list(grp_soc_list[soc_idx], WLAN_PEER_OP,
                                      ath_vdev_recover_peer_deauth_handler, &mlo_only, 1,
                                      WLAN_MLME_NB_ID);
     }

}
#endif

#endif

void
ol_ath_wifi_recovery_remove(struct platform_device *pdev)
{
    struct net_device *dev = platform_dev_to_net_dev(pdev);
    ol_ath_soc_softc_t *soc;
    struct pdev_op_args oper;
    bool delay = false;

    soc = ath_netdev_priv(dev);
    if (dev) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
        dev_change_flags(dev, dev->flags & (~IFF_UP), NULL);
#else
        dev_change_flags(dev, dev->flags & (~IFF_UP));
#endif
    }
    oper.type = PDEV_ITER_RECOVERY_AHB_REMOVE;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                            wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);


    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_VDEV_OP, ath_vdev_recover_delete,
                                            NULL, 1, WLAN_MLME_NB_ID);

    oper.type = PDEV_ITER_RECOVERY_WAIT;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                            wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

    delay = soc->delay_bug_on;
    if(delay) {
        qdf_err("Delayed BUG_ON requested in FW recovery");
        return;
    } else {
        ol_ath_soc_stop(soc, 0);
        oper.type = PDEV_ITER_RECOVERY_STOP;
        wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                              wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
    }
}
qdf_export_symbol(ol_ath_wifi_recovery_remove);

int
ol_ath_wifi_recovery_probe(struct platform_device *pdev, const struct platform_device_id *id)
{
    struct net_device *dev = platform_dev_to_net_dev(pdev);
    struct ol_ath_soc_softc *soc = ath_netdev_priv(dev);
    int32_t ret = -1;

    if (qdf_atomic_test_and_set_bit(SOC_RESET_IN_PROGRESS_BIT,
                                    &soc->reset_in_progress)) {
        qdf_err("ERROR: SOC reset in progress");
        return -EBUSY;
    }

    ret = ol_ath_soc_start(soc);
    qdf_atomic_clear_bit(SOC_RESET_IN_PROGRESS_BIT, &soc->reset_in_progress);
    if(ret != 0) {
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
                    QDF_TRACE_LEVEL_INFO, "ERROR: Soc start failed\n");
        return ret;
    }

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    soc->mlo_partner_hang_issued = false;
#endif

    ol_ath_trigger_recover_create(soc);

    return ret;
}
qdf_export_symbol(ol_ath_wifi_recovery_probe);

#define REMOVE_VAP_TIMEOUT_COUNT  20
#define REMOVE_VAP_TIMEOUT        (HZ/10)
void
ol_ath_ahb_remove(struct platform_device *pdev)
{
    struct net_device *dev = NULL;
    ol_ath_soc_softc_t *soc;
    u_int32_t target_ver,target_type;
    struct hif_opaque_softc *sc;
#ifdef CONFIG_CNSS2_SUPPORT
    struct pld_plat_data *cnss_plat, *wlan_sc;
    struct platform_device_id *id;
#endif /* CONFIG_CNSS2_SUPPORT */
    void __iomem *mem;
    int target_paused = TRUE;
    int waitcnt = 0;
    void *hif_cxt = NULL;
    struct _NIC_DEV *aps_osdev = NULL;
    qdf_device_t  qdf_dev = NULL;    /* qdf handle */
    wmi_unified_t wmi_handle;
    struct hif_opaque_softc *hif_hdl;
    struct pdev_op_args oper;


#ifdef CONFIG_CNSS2_SUPPORT
#ifndef BUILD_X86
    id = ath_ahb_get_pdev_id(pdev);
#else
    /* We dont need to get id in x86 platform. Make this 0 */
    id = 0;
#endif
#ifdef CONFIG_OF
    dev = platform_dev_to_net_dev(pdev);
#else
    dev = platform_dev_id_to_net_dev(pdev, id);
#endif

    if (id && ((id->driver_data == QCA8074_DEVICE_ID) ||
               (id->driver_data == QCA8074V2_DEVICE_ID) ||
               (id->driver_data == QCA9574_DEVICE_ID) ||
               (id->driver_data == QCA6018_DEVICE_ID) ||
               (id->driver_data == QCN6122_DEVICE_ID) ||
               (id->driver_data == QCN9160_DEVICE_ID) ||
               (id->driver_data == QCA5332_DEVICE_ID) ||
               (id->driver_data == QCA5018_DEVICE_ID) ||
               (id->driver_data == QCN6432_DEVICE_ID)) &&
        (testmode == PLD_WALTEST)) {
        if (pld_wlan_disable(&pdev->dev, PLD_OFF))
            qdf_err("Error in pld_wlan_disable\n");
        else
            qdf_info("QMI mode off sent to target\n");

        return;
    }
#endif

    /* Attach did not succeed, all resources have been
     * freed in error handler
     */
    if (!dev) {
        qdf_print("dev is null");
        return;
    }
    soc = ath_netdev_priv(dev);
    hif_hdl = lmac_get_hif_hdl(soc->psoc_obj);
    hif_cxt = hif_hdl;
    sc = (struct hif_opaque_softc *)hif_hdl;

    mem = (void __iomem *)dev->mem_start;

    __ol_vap_delete_on_rmmod(dev);

    while (qdf_atomic_test_bit(SOC_RESET_IN_PROGRESS_BIT,
           &soc->reset_in_progress) &&
           (waitcnt < REMOVE_VAP_TIMEOUT_COUNT)) {
        schedule_timeout_interruptible(REMOVE_VAP_TIMEOUT);
        waitcnt++;
    }

    /* Suspend the target if it is not done during the last vap removal */
    if (!(soc->down_complete)) {
        /* Suspend Target */
        qdf_print("Suspending Target - with disable_intr set :%s (sc %pK) soc=%pK",dev->name,sc, soc);
        if (!ol_ath_suspend_target(soc, 1)) {
            u_int32_t  timeleft;
            qdf_print("waiting for target paused event from target :%s (sc %pK)",dev->name,sc);
            /* wait for the event from Target*/
            timeleft = wait_event_interruptible_timeout(soc->sc_osdev->event_queue,
                    (soc->is_target_paused == TRUE),
                    PDEV_SUSPEND_TIMEOUT);
            if(!timeleft || signal_pending(current)) {
                qdf_print("ERROR: Failed to receive target paused event :%s (sc %pK)",dev->name,sc);
                target_paused = FALSE;
            }
            /*
             * reset is_target_paused and host can check that in next time,
             * or it will always be TRUE and host just skip the waiting
             * condition, it causes target assert due to host already suspend
             */
            soc->is_target_paused = FALSE;
        }
    }
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    qdf_print("AHB Call to nss detach ");
    if (!(soc->down_complete)) {

            qdf_print("Call to nss detach ops %pK ", soc->nss_soc.ops);
            if (soc->nss_soc.ops) {
            qdf_print("nss-wifi: detach ");
            soc->nss_soc.ops->nss_soc_wifi_detach(soc, 100);
        }
    }
#endif

    /* Copy the pointer as netdev will be freed in __ol_ath_detach */
    aps_osdev = soc->sc_osdev;
    qdf_dev = soc->qdf_dev;

    if (!soc->down_complete) {
        ol_ath_diag_user_agent_fini(soc);
    }
    /* save target_version since soc is not valid after __ol_ath_detach */
    target_ver = lmac_get_tgt_version(soc->psoc_obj);
    target_type = lmac_get_tgt_type(soc->psoc_obj);
    wmi_handle = lmac_get_wmi_unified_hdl(soc->psoc_obj);


    oper.type = PDEV_ITER_PDEV_ENTRY_DEL;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                      wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

    if (target_paused == TRUE) {
        __ol_ath_detach(dev);
    } else {
        wmi_stop(wmi_handle);
    }

    if (target_paused != TRUE) {
        __ol_ath_detach(dev);
    }

    if(aps_osdev)
        qdf_mem_free(aps_osdev);
    if(qdf_dev)
        qdf_mem_free(qdf_dev);

#ifdef CONFIG_CNSS2_SUPPORT
    if (id && ((id->driver_data == QCA8074_DEVICE_ID) ||
               (id->driver_data == QCA8074V2_DEVICE_ID) ||
               (id->driver_data == QCA9574_DEVICE_ID) ||
               (id->driver_data == QCA6018_DEVICE_ID) ||
               (id->driver_data == QCA5332_DEVICE_ID) ||
               (id->driver_data == QCN6122_DEVICE_ID) ||
               (id->driver_data == QCN9160_DEVICE_ID) ||
               (id->driver_data == QCA5018_DEVICE_ID) ||
               (id->driver_data == QCN6432_DEVICE_ID))) {
        cnss_plat = platform_get_drvdata(pdev);
        wlan_sc  = cnss_plat->wlan_priv;
        cnss_plat->wlan_priv = NULL;
        wlan_sc->wlan_priv = NULL;
        qdf_mem_free(wlan_sc);
    }
#endif
#ifndef CONFIG_OF
    pdev->id_entry = NULL;
#endif
    qdf_nofl_info("ath_ahb_remove");
}
qdf_export_symbol(ol_ath_ahb_remove);

#if OL_ATH_SUPPORT_LED
/* Temporarily defining the core_sw_output address here as
ar900b,qca9884,qca9888 didn't define the macro in FW cmn headers */
#define CORE_SW_OUTPUT 0x82004
#endif
