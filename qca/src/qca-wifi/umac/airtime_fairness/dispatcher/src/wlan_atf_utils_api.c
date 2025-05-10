/*
 *
 * Copyright (c) 2017, 2019-2020, 2022-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#include <wlan_atf_utils_api.h>
#include <qdf_module.h>
#include "../../core/atf_cmn_api_i.h"

/* Do ATF Mode Configuration */
uint32_t atf_mode;
qdf_declare_param(atf_mode, uint);
qdf_export_symbol(atf_mode);

QDF_STATUS wlan_atf_init(void)
{
	if (wlan_objmgr_register_psoc_create_handler(WLAN_UMAC_COMP_ATF,
	     wlan_atf_psoc_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_register_psoc_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_psoc_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_register_pdev_create_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_pdev_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_register_pdev_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_pdev_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_register_vdev_create_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_vdev_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_register_vdev_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_vdev_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_register_peer_create_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_peer_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_register_peer_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_peer_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_atf_deinit(void)
{
	if (wlan_objmgr_unregister_psoc_create_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_psoc_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_unregister_psoc_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_psoc_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_unregister_pdev_create_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_pdev_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_unregister_pdev_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_pdev_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_unregister_vdev_create_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_vdev_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_unregister_vdev_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_vdev_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_unregister_peer_create_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_peer_obj_create_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if (wlan_objmgr_unregister_peer_destroy_handler(WLAN_UMAC_COMP_ATF,
	    wlan_atf_peer_obj_destroy_handler, NULL) != QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_atf_open(struct wlan_objmgr_psoc *psoc)
{
	if (NULL == psoc) {
		atf_err("PSOC is null!\n");
		return QDF_STATUS_E_FAILURE;
	}

	atf_open(psoc);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_atf_close(struct wlan_objmgr_psoc *psoc)
{
	struct atf_context *ac = NULL;

	if (NULL == psoc) {
		atf_err("PSOC is null!\n");
		return QDF_STATUS_E_FAILURE;
	}
	ac = wlan_objmgr_psoc_get_comp_private_obj(psoc, WLAN_UMAC_COMP_ATF);
	if (NULL == ac) {
		atf_err("atf_context is null!\n");
		return QDF_STATUS_E_FAILURE;
	}
	ac->atf_mode = 0;
	ac->atf_fmcap = 0;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_atf_enable(struct wlan_objmgr_psoc *psoc)
{
	if (NULL == psoc) {
		atf_err("PSOC is null!\n");
		return QDF_STATUS_E_FAILURE;
	}

	atf_enable(psoc);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_atf_disable(struct wlan_objmgr_psoc *psoc)
{
	if (NULL == psoc) {
		atf_err("PSOC is null!\n");
		return QDF_STATUS_E_FAILURE;
	}

	atf_disable(psoc);

	return QDF_STATUS_SUCCESS;
}

#define ATF_SET_CFG_PEER_ID()                                                  \
	if (cfg->peer_id[i].cfg_flag) {                                        \
		cfg->peer_id[i].index_vdev = 0xff;                             \
		cfg->peer_id[i].sta_cal_value = 0;                             \
		cfg->peer_id[i].sta_assoc_status = 0;                          \
		ATF_CLEAR_DIRTY_PEER(cfg->peer_id[i]);                         \
	} else {                                                               \
		/* non configured peer take care of index change */            \
		cfg->peer_id[i].cfg_flag = cfg->peer_id[j].cfg_flag;           \
		cfg->peer_id[i].sta_cfg_mark = cfg->peer_id[j].sta_cfg_mark;   \
		qdf_mem_copy(cfg->peer_id[i].sta_cfg_value,                    \
				cfg->peer_id[j].sta_cfg_value,                 \
				sizeof(cfg->peer_id[i].sta_cfg_value));        \
		cfg->peer_id[i].index_vdev = cfg->peer_id[j].index_vdev;       \
		cfg->peer_id[i].sta_cal_value = cfg->peer_id[j].sta_cal_value; \
		cfg->peer_id[i].sta_assoc_status =                             \
					cfg->peer_id[j].sta_assoc_status;      \
		qdf_mem_copy(cfg->peer_id[i].sta_mac, cfg->peer_id[j].sta_mac, \
							QDF_MAC_ADDR_SIZE);    \
		cfg->peer_id[i].index_group = cfg->peer_id[j].index_group;     \
		cfg->peer_id[i].is_sta_vap_node =                              \
					cfg->peer_id[j].is_sta_vap_node;       \
		cfg->peer_id[i].peer_cal_flag = cfg->peer_id[j].peer_cal_flag; \
		ATF_COPY_DIRTY_PEER_FLAG(cfg->peer_id[i], cfg->peer_id[j]);    \
		cfg->peer_id[j].cfg_flag = 0;                                  \
		cfg->peer_id[j].sta_cfg_mark = 0;                              \
		qdf_mem_zero(&cfg->peer_id[j].sta_cfg_value[0],                \
				sizeof(cfg->peer_id[i].sta_cfg_value));        \
		qdf_mem_zero(&cfg->peer_id[j].sta_mac[0], QDF_MAC_ADDR_SIZE);  \
		cfg->peer_id[j].index_vdev = 0;                                \
		cfg->peer_id[j].sta_cal_value = 0;                             \
		cfg->peer_id[j].sta_assoc_status = 0;                          \
		cfg->peer_id[j].peer_cal_flag = false;                         \
		cfg->peer_id[j].index_group = 0;                               \
		cfg->peer_id[j].is_sta_vap_node = false;                       \
		ATF_CLEAR_DIRTY_PEER(cfg->peer_id[j]);                         \
	}

void
wlan_atf_peer_join_leave(struct wlan_objmgr_peer *peer, const uint8_t type)
{
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct pdev_atf *pa = NULL;
	struct vdev_atf *va = NULL;
	struct atf_context *ac = NULL;
	struct atf_config *cfg = NULL;
	struct peer_atf *pa_obj = NULL;
	uint16_t i, j;
	uint8_t *peer_mac = NULL, *vdev_mac = NULL, *temp_mac = NULL;
	bool is_sta_vap = false;

	if (NULL == peer) {
		atf_err("peer is NULL!\n");
		return;
	}
	vdev = wlan_peer_get_vdev(peer);
	if (NULL == vdev) {
		atf_err("vdev is NULL!\n");
		return;
	}
	pa_obj = wlan_objmgr_peer_get_comp_private_obj(peer,
						       WLAN_UMAC_COMP_ATF);
	if (NULL == pa_obj) {
		atf_err("peer atf object is NULL\n");
		return;
	}
	peer_mac = wlan_peer_get_macaddr(peer);

	pa = atf_get_pdev_atf_from_vdev(vdev);
	if (NULL == pa) {
		atf_err("pdev_atf object is NULL!\n");
		return;
	}
	ac = atf_get_atf_ctx_from_vdev(vdev);
	if (NULL == ac) {
		atf_err("atf context is NULL!\n");
		return;
	}
	pdev = wlan_vdev_get_pdev(vdev);
	if (NULL == pdev) {
		atf_err("pdev is NULL!\n");
		return;
	}
	vdev_mac = wlan_vdev_mlme_get_macaddr(vdev);
	cfg = &pa->atfcfg_set;

	if (QDF_STA_MODE == wlan_vdev_mlme_get_opmode(vdev))
		is_sta_vap = true;

	/* ATF commit is enabled and MACs are not equal */
	if ((pa->atf_commit) &&
	    (qdf_mem_cmp(vdev_mac, peer_mac, QDF_MAC_ADDR_SIZE))) {
		atf_info("ATF is re-allocating airtime because %pM"
                         "is now  %s\n", peer_mac, (type ? "active" : "inactive"));
	}

	/* Add join node */
	if (type) {
		for (i = 0; i < cfg->max_clients; i++) {
			if (cfg->peer_id[i].index_vdev == 0) {
				qdf_mem_copy(cfg->peer_id[i].sta_mac, peer_mac,
					     QDF_MAC_ADDR_SIZE);
				cfg->peer_id[i].index_vdev = 0xff;
				cfg->peer_id[i].sta_assoc_status = 1;
				cfg->peer_id[i].peer_cal_flag = true;
				cfg->peer_id[i].is_sta_vap_node = is_sta_vap;
				ATF_SET_DIRTY_PEER(cfg->peer_id[i]);
				break;
			} else {
				/* MACs are equal */
				if (!qdf_mem_cmp(cfg->peer_id[i].sta_mac,
						 peer_mac, QDF_MAC_ADDR_SIZE)) {
					if (cfg->peer_id[i].cfg_flag) {
						cfg->peer_id[i].sta_assoc_status
									= 1;
						ATF_SET_DIRTY_PEER(cfg->peer_id[i]);
						break;
					} else {
						return;
					}
				}
			}
		}
		ATF_SET_FW_JOIN_UPDATE(pa);
		atf_set_assign_group(pa, peer, 0xFF);
	} else { /* Remove leave node */
		for (i = 0, j = 0; i < cfg->max_clients; i++) {
			if (cfg->peer_id[i].index_vdev != 0)
				j = i;
		}

		for (i = 0; i < cfg->max_clients; i++) {
			/* vdev index is not zero and MACs are equal */
			if ((cfg->peer_id[i].index_vdev != 0) &&
			    (!qdf_mem_cmp(cfg->peer_id[i].sta_mac, peer_mac,
				      QDF_MAC_ADDR_SIZE))) {
				if (j != i) {
					ATF_SET_CFG_PEER_ID()
					break;
				} else {
					if (cfg->peer_id[i].cfg_flag) {
						cfg->peer_id[i].index_vdev =
									0xff;
					} else {
						temp_mac =
						&cfg->peer_id[i].sta_mac[0];
						qdf_mem_zero(temp_mac,
							QDF_MAC_ADDR_SIZE);
						cfg->peer_id[i].index_vdev = 0;
						cfg->peer_id[i].index_group = 0;
						cfg->peer_id[i].peer_cal_flag =
							false;
						cfg->peer_id[i].is_sta_vap_node =
							false;
					}
					cfg->peer_id[i].sta_cal_value = 0;
					cfg->peer_id[i].sta_assoc_status = 0;
					ATF_CLEAR_DIRTY_PEER(cfg->peer_id[i]);
					break;
				}
			}
		}
		ATF_SET_FW_LEAVE_UPDATE(pa);
		va = wlan_objmgr_vdev_get_comp_private_obj(vdev,
							   WLAN_UMAC_COMP_ATF);
		if (NULL == va) {
			atf_err("vdev_atf component object NULL!\n");
			return;
		}
		if (pa_obj->ac_blk_cnt) {
			atf_debug("pa->ac_blk_cnt : %d va->ac_blk_cnt : %d \n",
				  pa_obj->ac_blk_cnt, va->ac_blk_cnt);
			if (va->ac_blk_cnt >= pa_obj->ac_blk_cnt) {
				va->ac_blk_cnt -= pa_obj->ac_blk_cnt;
				pa_obj->ac_blk_cnt = 0;
				atf_debug ("va->ac_bl_cnt changed to %d \n",
					   va->ac_blk_cnt);
			}
		}
	}

	if (i == cfg->max_clients)
		return;

	/* Wake up timer to update alloc table*/
	atf_start_cfg_timer(vdev, pa, ac);

	if (pa->atf_tput_based && pa_obj->atf_tput && pa->atf_commit)
		atf_update_tput_tbl(pdev, peer, type);
}

uint32_t wlan_atf_get_peer_airtime(struct wlan_objmgr_peer *peer)
{
	uint32_t value = 0;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_vdev *vdev;
	struct wlan_lmac_if_tx_ops *tx_ops;

	if (NULL == peer) {
		atf_err("peer is NULL!\n");
		return value;
	}
	vdev = wlan_peer_get_vdev(peer);
	if (NULL == vdev) {
		atf_err("vdev is NULL!\n");
		return value;
	}
	psoc = wlan_vdev_get_psoc(vdev);
	if (NULL == psoc) {
		atf_err("psoc is NULL!\n");
		return value;
	}

	tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
	if (!tx_ops) {
		atf_err("tx_ops is NULL \n");
		return value;
	}

	value = tx_ops->atf_tx_ops.atf_get_peer_airtime(peer);

	return value;
}

void wlan_atf_pdev_reset(struct wlan_objmgr_pdev *pdev)
{
	struct pdev_atf *pa;

	if (NULL == pdev) {
		atf_err("PDEV is NULL!\n");
		return;
	}
	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_ATF);
	if (NULL == pa) {
		atf_err("PDEV atf object is NULL!\n");
		return;
	}

	pa->atf_txbuf_max = ATF_MAX_BUFS;
	pa->atf_txbuf_min = ATF_MIN_BUFS;
	pa->atf_txbuf_share = 1;
	pa->atf_sched = 0;    /*reset*/
	pa->atf_sched &= ~ATF_SCHED_STRICT; /*Disable Strict queue*/
	pa->atf_sched |= ATF_SCHED_OBSS;    /*Enable OBSS*/

	/* Clear ATF stats and unsubcribe from PPDU stats processing */
	atf_stats_clear(pdev, pa);
}

