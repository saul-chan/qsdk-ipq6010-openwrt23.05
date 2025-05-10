/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

#if QCA_PPE_VP
#include "osif_private.h"
#include <wlan_opts.h>
#define OSIF_TO_NETDEV(_osif) (((osif_dev *)(_osif))->netdev)
#include <ppe_vp_public.h>
#include <ppe_vp_tx.h>
#if ATH_PERF_PWR_OFFLOAD
#include <ol_if_athvar.h>
#endif
#include <cdp_txrx_ctrl.h>
#include "cfg_ucfg_api.h"
#ifdef WLAN_SUPPORT_PPEDS
#include <cdp_txrx_ppe.h>
#endif
#include <wlan_repeater_api.h>
#include "target_type.h"

static const char *osif_ppe_vp_str[PPE_VP_USER_TYPE_MAX] = { "none",
							     "passive",
							     "active",
							     "ds" };

#define OSIF_INVALID_VP_NUM -1

#ifdef QCA_SUPPORT_WDS_EXTENDED
/*
 * osif_ppe_vp_ext_vap_set_mtu
 * 	Update ppe vp with new mtu
 */
bool osif_ppe_vp_ext_vap_set_mtu(osif_peer_dev *osifp, int mtu)
{
	if (!osifp->ppe_vp_num) {
		return true;
	}

	if (ppe_vp_mtu_set(osifp->ppe_vp_num, mtu) != PPE_VP_STATUS_SUCCESS) {
		qdf_err("\ndev:%p, dev->name:%s osnetdev:%p, \
				mtu %d set failed ", osifp->netdev,
				osifp->netdev->name, osifp, mtu);
		return false;
	}
	return true;
}

/*
 * osif_ppe_vp_ext_vap_set_mac_addr
 * 	Update ppe vp with new mac
 */
bool osif_ppe_vp_ext_vap_set_mac_addr(osif_peer_dev *osifp, uint8_t *mac_addr)
{
	if (!osifp->ppe_vp_num) {
		return true;
	}

	if (ppe_vp_mac_addr_set(osifp->ppe_vp_num, mac_addr) != PPE_VP_STATUS_SUCCESS) {
		qdf_err("\ndev:%p, dev->name:%s osnetdev:%p, \
				mac addr change failed ", osifp->netdev,
				osifp->netdev->name, osifp);
		return false;
	}
	return true;
}
#endif

/*
 * osif_ppe_vp_set_mtu
 * 	Update ppe vp with new mtu
 */
bool osif_ppe_vp_set_mtu(struct net_device *dev, int32_t ppe_vp_num, int mtu)
{
	if (!ppe_vp_num) {
		return true;
	}

	if (ppe_vp_mtu_set(ppe_vp_num, mtu) != PPE_VP_STATUS_SUCCESS) {
		qdf_info("\ndev:%p, dev->name:%s mtu %d vp num = %dset failed ",
                         dev,dev->name, mtu, ppe_vp_num);
		return false;
	}
	return true;
}

/*
 * osif_ppe_vp_set_mac_addr
 * 	Update ppe vp with new mac
 */
bool osif_ppe_vp_set_mac_addr(struct net_device *dev,
                              int32_t ppe_vp_num,
                              uint8_t *mac_addr)
{
	if (!ppe_vp_num) {
		return true;
	}

	if (ppe_vp_mac_addr_set(ppe_vp_num, mac_addr) != PPE_VP_STATUS_SUCCESS) {
		qdf_info("\ndev:%p, dev->name:%s, vp_num = %d, \
                         mac addr change failed", dev, dev->name, ppe_vp_num);
		return false;
	}
	return true;
}

/*
 * osif_is_mld_link_vap
 * 	return true in case of mld link vap else false
 */
static inline bool
osif_is_mld_link_vap(wlan_if_t vap, struct net_device *dev)
{
	if (vap->vif_type == VIF_TYPE_MLD_LINK_VAP) {
		/* for wds ext, parent vap can be MLD_LINK_VAP
		 * whereas for mld link vap netdev won't be created
		 */
		if (!dev)
			return true;
	}

	return false;
}

#ifdef WLAN_FEATURE_11BE_MLO
/*
 * osif_get_mld_vp_num
 * 	return the PPE VP number for mld
 */
static inline int32_t osif_get_mld_vp_num(osif_dev *osdev)
{
	if (!osdev)
		return -1;

	if (osdev->dev_type != OSIF_NETDEV_TYPE_VAP)
		return -1;

	/* non mld vap */
	if (!osdev->mldev)
		return -1;

	return osdev->mldev->ppe_vp_num;
}

/*
 * osif_get_mld_ppe_vp_type
 * 	return the PPE VP type for mld
 */
static inline unsigned long
osif_get_mld_ppe_vp_type(osif_dev *osdev)
{
	if (!osdev)
		return -1;

	if (osdev->dev_type != OSIF_NETDEV_TYPE_VAP)
		return -1;

	/* non mld vap */
	if (!osdev->mldev)
		return -1;

	return osdev->mldev->ppe_vp_type;
}

#else
static inline int32_t osif_get_mld_vp_num(osif_dev *osdev)
{
	return -1;
}

static inline unsigned long
osif_get_mld_ppe_vp_type(osif_dev *osdev)
{
	return -1;
}
#endif

/*
 * osif_pltfrm_delete_ppe_vp
 *  Delete PPE VP for this vap
 */
static inline void osif_pltfrm_delete_ppe_vp(wlan_if_t vap, int32_t ppe_vp_num, struct net_device *dev)
{
#ifdef WLAN_SUPPORT_PPEDS
        ol_txrx_soc_handle soc;
        uint8_t vdev_id;
#ifdef QCA_SUPPORT_WDS_EXTENDED
	osif_peer_dev *osif_peer;
#endif
	struct cdp_ds_vp_params vp_params = {0};
	osif_dev *osdev;
#endif
	if (ppe_vp_num <= 0) {
		qdf_warn("Invalid VP number %d\n", ppe_vp_num);
		return;
	}

	/*
	 * Free PPE-VP for DS VAP.
	 */
#ifdef WLAN_SUPPORT_PPEDS
	if (vap->cp.icp_flags & IEEE80211_PPE_VP_DS_ACCEL) {
		/*
		 * Free PPE-VP for DS VAP.
		 */
		vdev_id = wlan_vdev_get_id(vap->vdev_obj);
		soc = wlan_psoc_get_dp_handle(wlan_vdev_get_psoc(vap->vdev_obj));

		/* for mld link vap netdev won't be avaliable
		 * get the osdev from vap.
		 */
		if (osif_is_mld_link_vap(vap, dev))
			osdev = (osif_dev *)vap->iv_ifp;
		else
			osdev = ath_netdev_priv(dev);

		/*
		 * Fetch the ppe vp profile info from osif_dev / osif_peer_dev
		 * according to the wds_ext mode set.
		 */
#ifdef QCA_SUPPORT_WDS_EXTENDED
		if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
			osif_peer = ath_netdev_priv(dev);
			vp_params.ppe_vp_profile_idx = osif_peer->ppe_vp_profile_idx;
			vp_params.wds_ext_mode = true;
			goto entry_detach;
		}
#endif
		vp_params.ppe_vp_profile_idx = osdev->ppe_vp_profile_idx;
#ifdef QCA_SUPPORT_WDS_EXTENDED
entry_detach:
#endif
		/* for mld link vap mld destroy will take care of vp free
		 * in all other cases free the VP here
		 */
		if (!osif_is_mld_link_vap(vap, dev))
			cdp_ppeds_entry_free(soc, ppe_vp_num);

		/* detach the VP */
		cdp_ppesds_entry_detach(soc, vdev_id, &vp_params);
#ifdef QCA_SUPPORT_WDS_EXTENDED
		if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
			if (osif_peer->ppe_vp_type != PPE_VP_USER_TYPE_DS) {
				qdf_info("%p: Invalid osif peer got detached, vdev id %d\n", osif_peer, vdev_id);
				return;
			}

			osif_peer->ppe_vp_type = 0;
			osif_peer->ppe_vp_profile_idx = -1;
			osif_peer->ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
			return;
		}
#endif
		if (osdev->ppe_vp_type != PPE_VP_USER_TYPE_DS) {
			qdf_info("%p: Invalid osdev got detached, vdev id:%d\n", osdev, vdev_id);
			return;
		}

		osdev->ppe_vp_type = 0;
		osdev->ppe_vp_profile_idx = -1;
		osdev->ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
		return;
	}
#endif
	/*
	 * Free PPE-VP for active and passive cases.
	 * for mld link vap mld destroy will take care of vp free
	 */
	if (!osif_is_mld_link_vap(vap, dev))
		ppe_vp_free(ppe_vp_num);
}

/*
 * osif_vap_passive_vp_support_only
 *  Check if we do not need DS for these features
 *  Enable passive mode when below modes are detected.
 *  DS cannot do software processing for below modes.
 */
static inline bool
osif_vap_passive_vp_support_only(wlan_if_t vap)
{
	cdp_config_param_type val = {0};
	struct wlan_objmgr_vdev *vdev_obj = vap->vdev_obj;
	struct wlan_objmgr_psoc *psoc = wlan_vdev_get_psoc(vdev_obj);
	uint8_t vdev_id = wlan_vdev_get_id(vap->vdev_obj);
	ol_txrx_soc_handle soc = wlan_psoc_get_dp_handle(psoc);

	if (wlan_rptr_vdev_is_wrap(vdev_obj) ||
	    wlan_rptr_vdev_is_psta(vdev_obj) ||
            wlan_rptr_vdev_is_mpsta(vdev_obj)) {
		qdf_info("PPE-DS attach in PASSIVE mode \n");
		return true;
	}

#ifdef MESH_MODE_SUPPORT
	cdp_txrx_get_vdev_param(soc, vdev_id, CDP_MESH_MODE, &val);
#endif
	cdp_txrx_get_vdev_param(soc, vdev_id, CDP_MESH_RX_FILTER, &val);
	cdp_txrx_get_vdev_param(soc, vdev_id, CDP_ENABLE_NAWDS, &val);
	cdp_txrx_get_vdev_param(soc, vdev_id, CDP_ENABLE_WRAP, &val);

	if (val.cdp_vdev_param_mesh_rx_filter ||
	    val.cdp_vdev_param_mesh_mode || val.cdp_vdev_param_nawds ||
	    val.cdp_vdev_param_wrap ||
	    wlan_vdev_mlme_get_opmode(vdev_obj) == QDF_MONITOR_MODE) {
		qdf_info("PPE-DS attach in PASSIVE mode \n");
		return true;
	}

	return false;
}

#ifdef WLAN_SUPPORT_PPEDS
/*
 * osif_ppe_vp_ds_stats_update_cb
 *  PPE_VP callback to update the VP port stats
 *  in case of direct switch.
 */
bool osif_ppe_vp_ds_stats_update_cb(struct net_device *dev, ppe_vp_hw_stats_t *stats)
{
	osif_dev *osdev;
#ifdef QCA_SUPPORT_WDS_EXTENDED
	osif_peer_dev *osif_peer;
#endif
	wlan_if_t vap;
	ol_txrx_soc_handle soc_txrx_handle;
	uint16_t vdev_id;
	struct cdp_ds_vp_params vp_params = {0};

	/*
	 * In case of fw recovery, VAPs will go down but VP's will
	 * still be there. Hence this callback will keep on coming even if VAP object
	 * is down in recovery mode. So we check the recovery mode using the netdev
	 * queue and return if netdev queue is stopped.
	 */
	if (netif_queue_stopped(dev))
		return false;

	vp_params.dev = dev;
	osdev = ath_netdev_priv(dev);
	if (!osdev) {
		qdf_err("%pK: No osdev present for netdev", dev);
		return false;
	}

	vp_params.ppe_vp_type = osdev->ppe_vp_type;

#ifdef QCA_SUPPORT_WDS_EXTENDED
	/*
	 * If in case of WDS EXT mode, get the parent dev to fetch
	 * the VAP object.
	 */
	if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
		osif_peer = ath_netdev_priv(dev);
		if(!osif_peer) {
			qdf_err("%pK: No osif peer dev present for netdev wds ext", dev);
			return false;
		}

		if (osif_peer->ppe_vp_type != PPE_VP_USER_TYPE_DS) {
			return false;
		}

		if (!osif_peer->parent_osif) {
			qdf_err("%pK: No parent dev present for netdev wds ext", dev);
			return false;
		}

		osdev = osif_peer->parent_osif;
		if (!osdev) {
			qdf_err("%pK: No osdev present for netdev wds ext", dev);
			return false;
		}

		vp_params.wds_ext_mode = true;
		vp_params.peer_id = osif_peer->peer_id;
		goto stats_sync;
	}
#endif

	if (osdev->ppe_vp_type != PPE_VP_USER_TYPE_DS) {
		return false;
	}

#ifdef QCA_SUPPORT_WDS_EXTENDED
stats_sync:
#endif
	vap = osdev->os_if;
	if (!vap) {
		qdf_err("%pK: No VAP present for osdev", osdev);
		return false;
	}

	vdev_id = wlan_vdev_get_id(vap->vdev_obj);
	soc_txrx_handle = wlan_psoc_get_dp_handle(wlan_vdev_get_psoc(vap->vdev_obj));
	if (!soc_txrx_handle) {
		qdf_warn("%pK: SoC handle is null", vap);
		return false;
	}

	/*
	 * Call PPEDS stats update.
	 */
	cdp_ppesds_update_dev_stats(soc_txrx_handle, &vp_params, vdev_id, (void *)stats);
	return true;
}
#endif

/*
 * osif_pltfrm_create_ppe_vp
 *  Create PPE VP for this vap
 */
static inline int32_t osif_pltfrm_create_ppe_vp(wlan_if_t vap, struct net_device *dev)
{
	struct wlan_objmgr_vdev *vdev_obj = vap->vdev_obj;
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_psoc *psoc;
	struct ieee80211com *ic;
	struct ppe_vp_ai vpai;
	int32_t vpnum = OSIF_INVALID_VP_NUM;
	int target_type;
#ifdef WLAN_SUPPORT_PPEDS
	uint8_t vdev_id;
	QDF_STATUS ret;
	struct cdp_ds_vp_params vp_params = {0};
	osif_dev *osdev;
#ifdef QCA_SUPPORT_WDS_EXTENDED
	osif_peer_dev *osif_peer;
#endif
#endif

	psoc = wlan_vdev_get_psoc(vdev_obj);
	if (!psoc) {
		qdf_warn("%p: psoc not found\n", vap);
		return -1;
	}

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle) {
		qdf_warn("%p: SoC not found\n", vap);
		return -1;
	}

	/* for mld link vap netdev won't be avaliable
	 * get the osdev from vap.
	 */
	if (osif_is_mld_link_vap(vap, dev))
		osdev = (osif_dev *)vap->iv_ifp;
	else
		osdev = ath_netdev_priv(dev);

#ifdef WLAN_SUPPORT_PPEDS
#ifdef QCA_SUPPORT_WDS_EXTENDED
	if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
		osif_peer = ath_netdev_priv(dev);
		osif_peer->ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
	} else {
		osdev->ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
	}
#else
	osdev->ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
#endif
#endif

	memset(&vpai, 0, sizeof(struct ppe_vp_ai));

	vpai.usr_type = PPE_VP_USER_TYPE_NONE;
	vpai.type = PPE_VP_TYPE_SW_L2;
	vpai.net_dev_type = PPE_VP_NET_DEV_TYPE_WIFI;

	ic = vap->iv_ic;
	target_type = ic->ic_get_tgt_type(ic);

	/*
	 * If ppe vp acceleration set to none, then do not configure
	 * any ppe acceleration type.
	 */
	if (vap->cp.icp_flags & IEEE80211_PPE_VP_NO_ACCEL)
		goto done;

	if (vap->cp.icp_flags & IEEE80211_PPE_VP_ACTIVE_ACCEL) {
		vpai.usr_type = PPE_VP_USER_TYPE_ACTIVE;
	} else if ((osif_vap_passive_vp_support_only(vap)) ||
		   (vap->cp.icp_flags & IEEE80211_PPE_VP_PASSIVE_ACCEL)) {
		/*
		 * If ppeds is configured and vap type can not be supported by ppeds HW
		 * then force it to passive vp type.
		 */
		vpai.usr_type = PPE_VP_USER_TYPE_PASSIVE;
		IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
		vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;
	} else {
		vpai.usr_type = PPE_VP_USER_TYPE_PASSIVE;
		IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
		vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;

#ifdef WLAN_SUPPORT_PPEDS
		if (cdp_ppeds_attached(soc_txrx_handle)) {
			vpai.usr_type = PPE_VP_USER_TYPE_DS;
			IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
			vap->cp.icp_flags |= IEEE80211_PPE_VP_DS_ACCEL;
		}
#endif
	}

#ifdef WLAN_FEATURE_11BE_MLO
	if (vdev_obj->mlo_dev_ctx)
		vpai.core_mask = cfg_get(psoc, CFG_DP_MLO_VP_CORE_MASK);
	else
#endif
		vpai.core_mask = wlan_pdev_get_config_params(wlan_vdev_get_pdev(vdev_obj), WLAN_CONFIG_VP_CORE_MASK);

#ifdef WLAN_SUPPORT_PPEDS
	if (vpai.usr_type == PPE_VP_USER_TYPE_DS) {
		vdev_id = wlan_vdev_get_id(vap->vdev_obj);
		vp_params.dev = dev;

#ifdef QCA_SUPPORT_WDS_EXTENDED
		if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
			/*
			 * Test the PPE-DS enabled value for WDS-EXT mode.
			 */
			osif_peer = ath_netdev_priv(dev);
			if (osif_peer->ppe_vp_type == PPE_VP_USER_TYPE_DS) {
				qdf_err("%p: PPE DS is already enabled on this osif peer for wds_ext, vdev id %d\n", osif_peer, vdev_id);
				goto done;
			}

			osif_peer->ppe_vp_profile_idx = -1;
			vp_params.peer_id = osif_peer->peer_id;
			vp_params.wds_ext_mode = true;
			goto entry_attach;
		}
#endif
		/*
		 * Test the PPE-DS enabled value.
		 */
		if (osdev->ppe_vp_type == PPE_VP_USER_TYPE_DS) {
			qdf_err("%p: PPE DS is already enabled on this osdev", osdev);
			goto done;
		}

		osdev->ppe_vp_profile_idx = -1;
#ifdef QCA_SUPPORT_WDS_EXTENDED
entry_attach:
#endif
		vpai.stats_cb = osif_ppe_vp_ds_stats_update_cb;

		/* in case of mld link vap, will be using the mld vp */
		if (osif_is_mld_link_vap(vap, dev)) {
			unsigned long mld_vp_type = 0;

			vpnum = osif_get_mld_vp_num(osdev);
			if (vpnum <= 0) {
				qdf_err("%p: mld vp is not configured", vap);
				IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
				vap->cp.icp_flags |= IEEE80211_PPE_VP_NO_ACCEL;
				goto done;
			}

			/* populate the mld vp type in vap */
			mld_vp_type = osif_get_mld_ppe_vp_type(osdev);
			if (mld_vp_type == PPE_VP_USER_TYPE_ACTIVE) {
				IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
				vap->cp.icp_flags |= IEEE80211_PPE_VP_ACTIVE_ACCEL;
				vpai.usr_type = PPE_VP_USER_TYPE_ACTIVE;
				goto done;
			} else if (mld_vp_type == PPE_VP_USER_TYPE_PASSIVE) {
				IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
				vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;
				vpai.usr_type = PPE_VP_USER_TYPE_PASSIVE;
				goto done;
			} else if (mld_vp_type == PPE_VP_USER_TYPE_NONE) {
				IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
				vap->cp.icp_flags |= IEEE80211_PPE_VP_NO_ACCEL;
				vpai.usr_type = PPE_VP_USER_TYPE_NONE;
				goto done;
			}

			ret = QDF_STATUS_SUCCESS;
		} else {
			/* allocate the DS VP */
			ret = cdp_ppeds_entry_alloc(soc_txrx_handle, &vpai, &vpnum, &vp_params);
		}

		if (ret == QDF_STATUS_SUCCESS) {
			/* attach the VP */
			ret = cdp_ppesds_entry_attach(soc_txrx_handle, vdev_id,
						      &vpai, vpnum, &vp_params);
			if (ret == QDF_STATUS_SUCCESS) {
				/*
				 * Save the vp profile info into osif_dev /
				 * osif_peer_dev area depending on the wds_ext
				 * mode is set or not.
				 */
#ifdef QCA_SUPPORT_WDS_EXTENDED
				if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
					osif_peer->ppe_vp_profile_idx = vp_params.ppe_vp_profile_idx;
					osif_peer->ppe_vp_type = PPE_VP_USER_TYPE_DS;
					osif_peer->ppeds_node_id =
							cdp_ppesds_get_node_id(soc_txrx_handle);
					goto done;
				}
#endif
				osdev->ppe_vp_profile_idx = vp_params.ppe_vp_profile_idx;
				osdev->ppe_vp_type = PPE_VP_USER_TYPE_DS;
				osdev->ppeds_node_id = cdp_ppesds_get_node_id(soc_txrx_handle);
				goto done;
			}
		}

		/* free the DS VP in case of failure
		 * skip freeing the mld vp in mld link vap case
		 */
		if (!osif_is_mld_link_vap(vap, dev) && (vpnum != OSIF_INVALID_VP_NUM)) {
			cdp_ppeds_entry_free(soc_txrx_handle, vpnum);
			vpnum = OSIF_INVALID_VP_NUM;
		}

		/*
		 * ppeds attach could fail if there are not enough ppeds rings / resource or for certain types of
		 * VAP which are not supported. in this case try whether passive vp can be allocated.
		 */
		if (ret == QDF_STATUS_E_NOSUPPORT) {
			qdf_warn("%p: ppeds not supported on this vap, fallback to passive vp\n", vap);
			vpai.usr_type = PPE_VP_USER_TYPE_PASSIVE;
			IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
			vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;
		} else if (ret == QDF_STATUS_E_RESOURCES) {
			qdf_warn("%p: ppeds resource not available, fallback to passive vp\n", vap);
			vpai.usr_type = PPE_VP_USER_TYPE_PASSIVE;
			IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
			vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;
		} else {
			qdf_warn("%p: PPE-DS CDP attach failed with err %d\n", vap, ret);
			IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
			goto done;
		}
	}
#endif

	/*
	 * Allocate ppe vp for active or passive ppe vp user
	 * will be using the mld vp in case of mld link vap
	 */
	if (osif_is_mld_link_vap(vap, dev))
		vpnum = osif_get_mld_vp_num(osdev);
	else
		vpnum = ppe_vp_alloc(dev, &vpai);

done:
	qdf_info("vp user type: %s vpai.core_mask: %u target type: %d vpnum: %d icp_flags: %x\n",
		osif_ppe_vp_str[vpai.usr_type], vpai.core_mask, target_type, vpnum, vap->cp.icp_flags);
	return vpnum;
}

#ifdef WLAN_SUPPORT_PPEDS
/*
 * osif_pltfrm_setup_ppe_vp_802_3_redir_ds_vap_fw_recovery
 *  Handle VAP setup request for DS mode in case of FW recovery
 */
static void osif_pltfrm_setup_ppe_vp_802_3_redir_ds_vap_fw_recovery(osif_dev *osif, bool recover)
{
	wlan_if_t vap = osif->os_if;
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_vdev *vdev_obj = vap->vdev_obj;
	struct wlan_objmgr_psoc *psoc;
	uint16_t vp_profile_idx;
	uint8_t vdev_id;
	QDF_STATUS ret;

	/*
	 * Do not setup DS VP in case if it is not a recovery mode,
	 * or it is not a DS VP.
	 */
	if (!recover || !(osif->ppe_vp_type == PPE_VP_USER_TYPE_DS))
		return;

	psoc = wlan_vdev_get_psoc(vdev_obj);
	if (!psoc) {
		qdf_warn("%p: psoc not found\n", vap);
		return;
	}

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle) {
		qdf_warn("%p: SoC not found\n", vap);
		return;
	}

	vdev_id = wlan_vdev_get_id(vap->vdev_obj);
	vp_profile_idx = osif->ppe_vp_profile_idx;

	ret = cdp_ppesds_vp_setup_fw_recovery(soc_txrx_handle, vdev_id, vp_profile_idx);
	if (ret != QDF_STATUS_SUCCESS) {
		qdf_warn("\n Unable to setup DS VP in fw recovery mode osif %p, VP # %d, vdev_id %d, ret %d", osif, osif->ppe_vp_num, vdev_id, ret);
		return;
	}

	qdf_info("\nosnetdev in fw recovery mode:%p, VP # %d, vdev_id %d ", osif, osif->ppe_vp_num, vdev_id);
}
#endif

/*
 * osif_pltfrm_create_ppe_vp_802_3_redir_vap()
 *  Create VAP to that accepts packets in 802.3 format
 */
static void osif_pltfrm_create_ppe_vp_802_3_redir_vap(osif_dev *osif)
{
	struct net_device *dev = OSIF_TO_NETDEV(osif);
	wlan_if_t vap = osif->os_if;

	if (((osif->ppe_vp_num = osif_pltfrm_create_ppe_vp(vap, dev)) <=0)) {
		qdf_info("\ndev:%p, dev->name:%s osnetdev:%p, VP allocation failed ", dev, dev->name, osif);
		osif->ppe_vp_num = 0;
		return;
	}
	qdf_info("\ndev:%p, dev->name:%s osnetdev:%p, allocated VP # %d ", dev, dev->name, osif, osif->ppe_vp_num);
}

/*
 * osif_pltfrm_update_ppe_vp
 *   Change DS acceleration to PASSIVE
 */
int osif_pltfrm_update_ppe_vp(osif_dev *osif)
{
	struct net_device *dev = OSIF_TO_NETDEV(osif);
	wlan_if_t vap = osif->os_if;
	int32_t vp_num = osif->ppe_vp_num;

	/*
	 * Deleting ds ppe vp vap only
	 */
	if (vp_num && (vap->cp.icp_flags & IEEE80211_PPE_VP_DS_ACCEL)) {
		osif->ppe_vp_num = 0;
		osif_pltfrm_delete_ppe_vp(vap, vp_num, dev);
		qdf_info("\ndev:%p, DS VP %d deleted", dev, vp_num);
	} else {
		/*
		 * DS ppe vp vap does not exist. Return without
		 * destroying/recreating.
		 */
		return -1;
	}

	/*
	 * Creating vap with passive VP.
	 */
	vap->cp.icp_flags &= ~(IEEE80211_PPE_VP_ACTIVE_ACCEL | IEEE80211_PPE_VP_DS_ACCEL);
	vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;

	if (((osif->ppe_vp_num = osif_pltfrm_create_ppe_vp(vap, dev)) <=0)) {
		osif->ppe_vp_num = 0;
		qdf_info("\ndev:%p, VP allocation failed", dev);
		return -1;
	}

	qdf_info("\ndev:%p, VP allocation successful, ppe_vp num:%d", dev,  osif->ppe_vp_num);

	return osif->ppe_vp_num;
}
qdf_export_symbol(osif_pltfrm_update_ppe_vp);

/*
 * osif_pltfrm_handle_type_change
 * 	Handle vap type change event
 */
bool osif_pltfrm_handle_type_change(osif_dev *osif, uint32_t val)
{
	struct net_device *dev = OSIF_TO_NETDEV(osif);
	wlan_if_t vap = osif->os_if;

	/*
	 * Val =0 is raw mode and =1 is native wifi and ppe cannot
	 * understand them.
	 */
	if (val >= osif_pkt_type_ethernet) {
		/*
		 * If VP exist nothing to be done.
		 */
		if (osif->ppe_vp_num) {
			return true;
		}

		if (((osif->ppe_vp_num = osif_pltfrm_create_ppe_vp(vap, dev)) <=0)) {
			osif->ppe_vp_num = 0;
			qdf_info("\ndev:%p, dev->name:%s osnetdev:%p, VP allocation failed", dev, dev->name, osif);
			return false;
		}
		qdf_info("\ndev:%p, dev->name:%s osnetdev:%p, VP allocation successful, ppe_vp num:%d", dev, dev->name, osif, osif->ppe_vp_num);
	} else {
		if (!(vap->cp.icp_flags & IEEE80211_PPE_VP_NO_ACCEL ||
                      vap->cp.icp_flags & IEEE80211_PPE_VP_PASSIVE_ACCEL))
			qdf_err("requested operation is not supported");
	}
	return true;
}
qdf_export_symbol(osif_pltfrm_handle_type_change);

/*
 * osif_pltfrm_setup_ds_vap_fw_recovery
 *  Handle VAP setup request on FW recovery in DS mode.
 */
#ifdef WLAN_SUPPORT_PPEDS
void osif_pltfrm_setup_ds_vap_fw_recovery(osif_dev *osif, bool recover)
{
	osif_pltfrm_setup_ppe_vp_802_3_redir_ds_vap_fw_recovery(osif, recover);
}
#endif

/*
 * osif_pltfrm_create_vap
 *  Create VP corresponding to this vap
 */
void osif_pltfrm_create_vap(osif_dev *osif)
{
	osif_pltfrm_create_ppe_vp_802_3_redir_vap(osif);
}

/*
 * osif_pltfrm_delete_vap
 *  Delete VP corresponding to this vap
 */
void osif_pltfrm_delete_vap(osif_dev *osif)
{
	wlan_if_t vap = osif->os_if;
	int32_t vp_num = osif->ppe_vp_num;
	struct net_device *dev = OSIF_TO_NETDEV(osif);
	if (vp_num <= 0) {
		qdf_warn("Invalid VP number %d\n", vp_num);
		return;
	}

	osif->ppe_vp_num = 0;
	osif_pltfrm_delete_ppe_vp(vap, vp_num, dev);
	qdf_info("\nosifdev:%p, VP %d deleted", osif, vp_num);
}

#ifdef QCA_SUPPORT_WDS_EXTENDED
/*
 * osif_pltfrm_create_ppe_vp_802_3_ext_redir_vap()
 *  Create VAP to that accepts packets in 802.3 format
 */
static void osif_pltfrm_create_ppe_vp_802_3_ext_redir_vap(osif_peer_dev *osif)
{
	struct net_device *dev = osif->netdev;
	osif_dev *osif_parent = osif->parent_osif;
	wlan_if_t pvap = osif_parent->os_if;

	if (((osif->ppe_vp_num = osif_pltfrm_create_ppe_vp(pvap, dev)) <=0)) {
		osif->ppe_vp_num = 0;
		qdf_info("\ndev:%p, dev->name:%s osnetdev:%p, VP allocation failed ", dev, dev->name, osif);
		return;
	}

	qdf_info("\ndev:%p, dev->name:%s osnetdev:%p, allocated VP # %d ", dev, dev->name, osif, osif->ppe_vp_num);
}

/*
 * osif_pltfrm_get_vp_core_mask()
 *	Return the VP core mask.
 */
uint8_t osif_pltfrm_get_vp_core_mask(struct wlan_objmgr_vdev *vdev_obj, struct wlan_objmgr_psoc *psoc)
{
#ifdef WLAN_FEATURE_11BE_MLO
	if (vdev_obj->mlo_dev_ctx)
		return cfg_get(psoc, CFG_DP_MLO_VP_CORE_MASK);
	else
#endif
		return wlan_pdev_get_config_params(wlan_vdev_get_pdev(vdev_obj), WLAN_CONFIG_VP_CORE_MASK);
}

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
#ifdef QCA_SUPPORT_WDS_EXTENDED
/*
 * osif_pltfrm_attach_ext_ppe_vp()
 *	Attach PPE_VP for a VAP in case of Direct Switch.
 */
void osif_pltfrm_attach_ext_ppe_vp(osif_peer_dev *osif_peer) {
	struct net_device *dev = osif_peer->netdev;
	osif_dev *osif_parent = osif_peer->parent_osif;
	wlan_if_t vap = osif_parent->os_if;
	struct wlan_objmgr_vdev *vdev_obj = vap->vdev_obj;
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_psoc *psoc;
	struct ieee80211com *ic;
	struct ppe_vp_ui vpui = {0};
	int32_t vpnum = OSIF_INVALID_VP_NUM;
	int target_type;
        int ret;
#ifdef WLAN_SUPPORT_PPEDS
	uint8_t vdev_id = wlan_vdev_get_id(vap->vdev_obj);
	struct cdp_ds_vp_params vp_params = {0};
#endif

	psoc = wlan_vdev_get_psoc(vdev_obj);
	if (!psoc) {
		qdf_warn("%p: psoc not found\n", vap);
		return;
	}

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle) {
		qdf_warn("%p: SoC not found\n", vap);
		return;
	}

	/*
	 * If PPE VP is not allocated for this dev, do not process further.
	 */
	vpnum = osif_peer->ppe_vp_num;
	if (vpnum <= 0) {
		qdf_err("%p: No VP allocated for wds_ext, dev %s\n", osif_peer, dev->name);
		return;
	}

	/*
	 * If PPE VP acceleration set to none for the parent VAP,
	 * then do not process further, free the VP straight away instead.
	 */
	if (vap->cp.icp_flags & IEEE80211_PPE_VP_NO_ACCEL) {
		qdf_err("%p: No VP acceleration is selected \n", osif_peer);
		goto cleanup;
	}

	ic = vap->iv_ic;
	target_type = ic->ic_get_tgt_type(ic);

	/*
	 * Check the base VAP type and update the type of VP accordingly.
	 */
	if (vap->cp.icp_flags & IEEE80211_PPE_VP_ACTIVE_ACCEL) {
		vpui.usr_type = PPE_VP_USER_TYPE_ACTIVE;
	} else if ((osif_vap_passive_vp_support_only(vap)) || (vap->cp.icp_flags & IEEE80211_PPE_VP_PASSIVE_ACCEL)) {
		/*
		 * If ppeds is configured and vap type can not be supported by ppeds HW
		 * then force it to passive vp type.
		 */
		vpui.usr_type = PPE_VP_USER_TYPE_PASSIVE;
		IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
		vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;
	} else {
		vpui.usr_type = PPE_VP_USER_TYPE_PASSIVE;
		IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
		vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;

#ifdef WLAN_SUPPORT_PPEDS
		if (cdp_ppeds_attached(soc_txrx_handle)) {
			vpui.usr_type = PPE_VP_USER_TYPE_DS;
			IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
			vap->cp.icp_flags |= IEEE80211_PPE_VP_DS_ACCEL;
		}
#endif
	}

	/*
	 * Update PPE VP core mask and type based on the base VAP type.
	 */
	vpui.core_mask = osif_pltfrm_get_vp_core_mask(vdev_obj, psoc);
	osif_peer->ppe_vp_type = vpui.usr_type;

	ret = ppe_vp_cfg_update(vpnum, &vpui);
	if (ret != PPE_VP_STATUS_SUCCESS) {
		qdf_err("%p: Failed to Update PPE VP for WDS ext dev %s mode %s ret %d vpnum %d\n", osif_peer, dev->name, osif_ppe_vp_str[vpui.usr_type], ret, vpnum);
		goto cleanup;
	}

	/*
	 * Skip VP attach for NON DS VP.
	 */
	if (!(vap->cp.icp_flags & IEEE80211_PPE_VP_DS_ACCEL))
		goto done;

#ifdef WLAN_SUPPORT_PPEDS
	/*
	 * Attach PPE VP profile for DS VP.
	 */
	osif_peer->ppe_vp_profile_idx = -1;
	vp_params.peer_id = osif_peer->peer_id;
	vp_params.wds_ext_mode = true;

	ret = cdp_ppesds_entry_attach(soc_txrx_handle, vdev_id, NULL, vpnum, &vp_params);

	/*
	 * Save the VP profile information into the osif object.
	 */
	if (ret == QDF_STATUS_SUCCESS) {
		osif_peer->ppe_vp_profile_idx = vp_params.ppe_vp_profile_idx;
		osif_peer->ppeds_node_id = cdp_ppesds_get_node_id(soc_txrx_handle);
		goto done;
	}

	/*
	 * ppeds attach could fail if there are not enough ppeds rings / resource or for certain types of
	 * VAP which are not supported. In case of failure, check if fallback to passive
	 * mode is possible, else free the VP.
	 */
	if ((ret == QDF_STATUS_E_NOSUPPORT) || (ret == QDF_STATUS_E_RESOURCES)) {
		qdf_warn("%p: ppeds not supported on this vap, fallback to passive vp\n", vap);
		IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
		vap->cp.icp_flags |= IEEE80211_PPE_VP_PASSIVE_ACCEL;

		vpui.core_mask = osif_pltfrm_get_vp_core_mask(vdev_obj, psoc);
		vpui.usr_type = PPE_VP_USER_TYPE_PASSIVE;

		ret = ppe_vp_cfg_update(vpnum, &vpui);
		if (ret != PPE_VP_STATUS_SUCCESS) {
			qdf_err("%p: Failed to Update PPE VP for WDS ext dev %s mode %s ret %d vp num %d\n", osif_peer, dev->name, osif_ppe_vp_str[vpui.usr_type], ret, vpnum);
			goto cleanup;
		}

		goto done;
	}
#endif

cleanup:
	ppe_vp_free(vpnum);
	osif_peer->ppe_vp_num = OSIF_INVALID_VP_NUM;
	IEEE80211_PPE_VP_ACCEL_CLEAR(vap);
	return;

done:
	qdf_info("vp user type: %s vpai.core_mask: %u target type: %d vpnum: %d icp_flags: %x\n",
		osif_ppe_vp_str[vpui.usr_type], vpui.core_mask, target_type, vpnum, vap->cp.icp_flags);
	return;
}

/*
 * osif_pltfrm_alloc_ext_ppe_vp()
 *	Allocate PPE VP for a VAP
 */
void osif_pltfrm_alloc_ext_ppe_vp(osif_peer_dev *osifp, struct net_device *peer_ndev)
{
	struct ppe_vp_ai vpai = {0};
	int32_t vpnum = 0;

	vpai.usr_type = PPE_VP_USER_TYPE_DS;
	vpai.type = PPE_VP_TYPE_SW_L2;
	vpai.net_dev_type = PPE_VP_NET_DEV_TYPE_WIFI;

	/*
	 * Allocate ppe vp for wds ext interface
	 */
	vpnum = ppe_vp_alloc(peer_ndev, &vpai);
	if (vpnum <= 0) {
		/* In some cases we might go out of available VP's,
		 * continue without creating a VP in those cases
		 */
		osifp->ppe_vp_num = 0;
		qdf_info("\ndev:%p, dev->name:%s, VP allocation failed", peer_ndev, peer_ndev->name);
		return;
	}

	osifp->ppe_vp_num = vpnum;
#ifdef WLAN_SUPPORT_PPEDS
	osifp->ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
#endif
	qdf_info("vp user type: %s vpai.core_mask = %u vp_num = %d allocated VP for dev = %s", "DS", vpai.core_mask, vpnum, peer_ndev->name);
}
#endif
#endif

/*
 * osif_pltfrm_create_ext_vap
 *  Allocate VP for this vap
 */
void osif_pltfrm_create_ext_vap(osif_peer_dev *osif)
{
	osif_pltfrm_create_ppe_vp_802_3_ext_redir_vap(osif);
}

/*
 * osif_pltfrm_delete_ext_vap
 *  Delete VP corresponding to this ext vap
 */
void osif_pltfrm_delete_ext_vap(osif_peer_dev *osif)
{
	osif_dev *osif_parent = osif->parent_osif;
	wlan_if_t pvap = NULL;

	int32_t vp_num = osif->ppe_vp_num;
	if (vp_num <= 0) {
		qdf_warn("Invalid VP number %d\n", vp_num);
		return;
	}
	osif->ppe_vp_num = 0;

	if (!osif_parent) {
		qdf_warn("parent osif is NULL %d\n", vp_num);
		return;
	}

	pvap = osif_parent->os_if;
	if (!pvap) {
		qdf_info("parent vap is NULL VP %d deleted", vp_num);
		/* In case of DS vp deletion will be
		 * taken care during DP vdev detach.
		 */
		if (osif->ppe_vp_type != PPE_VP_USER_TYPE_DS) {
			ppe_vp_free(vp_num);
		} else {
			qdf_info("vp profile index = %d", osif->ppe_vp_profile_idx);
			osif->ppe_vp_profile_idx = -1;
#ifdef WLAN_SUPPORT_PPEDS
			osif->ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
#endif
		}

		return;
	}

	osif_pltfrm_delete_ppe_vp(pvap, vp_num, osif->netdev);
	qdf_info("\nosifdev:%p, VP %d deleted", osif, vp_num);
}
#endif

#if defined(VDEV_PEER_PROT_COUNT) && !defined(QCA_SUPPORT_WDS_EXTENDED)
#define osif_rx_peer_protocol_cnt(osifp, soc, vdev_id, skb) \
{ \
	if ((osifp)->peer_protocol_cnt) { \
		cdp_txrx_peer_protocol_cnt((soc), (vdev_id), (skb),\
				CDP_VDEV_PEER_PROT_IS_EGRESS, \
				CDP_VDEV_PEER_PROT_IS_RX); \
	} \
}
#else
#define osif_rx_peer_protocol_cnt(osifp, soc, vdev_id, skb)
#endif

/*
 * osif_send_to_ppe_vp
 *  Send packets to the ppe vp driver
 */
void osif_send_to_ppe_vp(os_if_t osif, struct sk_buff *skb, int32_t ppe_vp_num,
		struct net_device *dev)
{
	/* PPE programs flows for both DS and VP case, in case of DS
	 * the the flow is sent to PPE directly using REO2PPE ring.
	 * in case of VP the pkts are delivered to host, but to identify
	 * if the pkts needs to be delivered to host or PPE depends on
	 * below conditions, if a flow entry is valid (flow entry is added
	 * by PPE) then give the pkt to PPE else give it to host N/W stack
	 */

	if (!qdf_nbuf_get_rx_flow_idx_valid(skb))
		goto out;

	if (qdf_unlikely(!ppe_vp_num)) {
		goto out;
	}

	skb->next = NULL;
	if (qdf_unlikely(skb_shared(skb))) {
		goto out;
	}

	nbuf_debug_del_record(skb);
	if (qdf_likely(ppe_vp_tx_to_ppe(ppe_vp_num, skb))) {
		return;
	}

	nbuf_debug_add_record(skb);

out:
	osif_rx_peer_protocol_cnt(osifp, soc_txrx_handle,
			wlan_vdev_get_id(osifp->ctrl_vdev), skb);
	skb->protocol = eth_type_trans(skb, dev);
	skb->dev = dev;
	nbuf_debug_del_record(skb);
	netif_receive_skb(skb);
	return ;
}
qdf_export_symbol(osif_send_to_ppe_vp);
#endif
