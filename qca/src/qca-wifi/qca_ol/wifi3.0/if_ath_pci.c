/*
 * Copyright (c) 2013-2018, 2020-2021, 2023-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2013-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
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

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <linux/version.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include "ol_if_athvar.h"
#include "ath_pci.h"
#include <pld_common.h>

extern struct semaphore reset_in_progress;
extern bool driver_registered;
extern unsigned int testmode;
extern unsigned int is_wifi_3_0_installed;

extern struct nss_wifi_soc_ops nss_wifili_soc_ops;
extern void osif_nss_register_module(OL_WIFI_DEV_TYPE target_type,
                                     struct nss_wifi_soc_ops *soc_ops);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
extern void init_mlo_cfg(void);
extern void deinit_mlo_cfg(void);
#endif
void wmi_tlv_init(void);
void ce_service_srng_init(void);
int ath_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id);
void ath_pci_remove(struct pci_dev *pdev);
int
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10)
ath_pci_suspend(struct pci_dev *pdev, pm_message_t state);
#else
ath_pci_suspend(struct pci_dev *pdev, u32 state);
#endif
int ath_pci_resume(struct pci_dev *pdev);
int ol_ath_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id);
void ol_ath_pci_remove(struct pci_dev *pdev);
int ol_ath_pci_suspend(struct pci_dev *pdev, pm_message_t state);
int ol_ath_pci_resume(struct pci_dev *pdev);
#if UMAC_SUPPORT_WEXT
void ol_ath_iw_detach(struct net_device *dev);
void ol_ath_iw_attach(struct net_device *dev);
#endif
QDF_STATUS create_target_if_ctx(void);
void ol_if_register_wifi3_0(void);
void ol_if_unregister_wifi3_0(void);
#ifdef ATH_AHB
extern int wifi3_ko_exit_in_progress;
extern int wifi_exit_in_progress;
#endif
#include "osif_bus.h"
#include "osif_private.h"
#include <acfg_api_types.h>   /* for ACFG_WDT_REINIT_DONE */
#include <acfg_drv_event.h>
#include <wlan_cmn.h>
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
#include <wlan_mlo_mgr_cmn.h>
#include <wlan_mlo_mgr_setup.h>
extern void __qca_ol_update_mlo_capability(bool wsi_remap, bool wsi_recv_in_progress);
#endif

#ifndef ATH_BUS_PM
#ifdef CONFIG_PM
#define ATH_BUS_PM
#endif /* CONFIG_PM */
#endif /* ATH_BUS_PM */

#ifdef WIFI_TARGET_TYPE_3_0
/*
 * Use a static table of PCI id's for now.  While this is the
 * "new way" to do things, we may want to switch back to having
 * the HAL check them by defining a probe method.
 */
#if defined(ATH_PCI) && !defined(CONFIG_AHB_FW_SIM) && !defined(CONFIG_PCIE_FW_SIM)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
const struct pci_device_id ath_pci_id_table_3_0[] = {
#else
DEFINE_PCI_DEVICE_TABLE(ath_pci_id_table_3_0) = {
#endif
    { 0xbeaf, 0xabc0, PCI_ANY_ID, PCI_ANY_ID }, /* Emulation PCIE  */
    { 0xbeaf, 0xabc1, PCI_ANY_ID, PCI_ANY_ID }, /* Emulation PCIE  */
    { 0xbeaf, 0xabc2, PCI_ANY_ID, PCI_ANY_ID }, /* Emulation PCIE  */
    { 0xbeaf, 0xabc3, PCI_ANY_ID, PCI_ANY_ID }, /* Emulation PCIE  */
    { 0xbeaf, 0xaa10, PCI_ANY_ID, PCI_ANY_ID }, /* Emulation PCIE  */
    { 0xbeaf, 0xaa11, PCI_ANY_ID, PCI_ANY_ID }, /* Emulation PCIE  */
    { 0 }
};

MODULE_DEVICE_TABLE(pci, ath_pci_id_table_3_0);

static struct pci_driver ol_ath_pci_driver = {
    .name       = "ath_ol_3_0_pci",
    .id_table   = ath_pci_id_table_3_0,
    .probe      = ath_pci_probe,
    .remove     = ath_pci_remove,
#ifdef ATH_BUS_PM
    .suspend    = ath_pci_suspend,
    .resume     = ath_pci_resume,
#endif /* ATH_BUS_PM */
    /* Linux 2.4.6 has save_state and enable_wake that are not used here */
};
#endif
#endif

#include <linux/ethtool.h>

/*
 * Module glue.
 */
#include "version.h"
#ifdef WIFI_TARGET_TYPE_3_0
static char *version = ATH_PCI_VERSION " (Atheros/multi-bss)";
static char *dev_info = "ath_ol_pci_wifi3.0";
#endif

#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif

#ifdef QCA_SINGLE_WIFI_3_0
int qdf_mod_init(void);
int qdf_mod_exit(void);
int init_umac(void);
void exit_umac(void);
int qca_ol_mod_init(void);
void qca_ol_mod_exit(void);
int spectral_init_module(void);
void spectral_exit_module(void);
void monitor_mod_init(void);
void monitor_mod_exit(void);
#endif

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
static void qca_ol_update_mlo_capability(bool wsi_remap)
{
   __qca_ol_update_mlo_capability(wsi_remap,false);
}
#endif

void __exit_ath_pci_3_0(void)
{
#ifdef WIFI_TARGET_TYPE_3_0
#ifdef ATH_AHB
    void exit_ath_ahb_3_0(void);
    wifi3_ko_exit_in_progress = 1;
    wifi_exit_in_progress = 1;
#endif

#ifdef QCA_SINGLE_WIFI_3_0
monitor_mod_exit();
#endif

    if (down_interruptible(&reset_in_progress))
        return;

#if defined(ATH_PCI) && !defined(CONFIG_AHB_FW_SIM) && !defined(CONFIG_PCIE_FW_SIM)
     pci_unregister_driver(&ol_ath_pci_driver);
#endif

    driver_registered = false;
    up(&reset_in_progress);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (pld_get_global_mlo_support())
        deinit_mlo_cfg();
#endif

#ifdef ATH_AHB
        exit_ath_ahb_3_0();
#else
    qdf_info("%s: driver unloaded", dev_info);
#endif /* ATH_AHB */

    ol_if_unregister_wifi3_0();
#ifdef QCA_SINGLE_WIFI_3_0
    qca_ol_mod_exit();
    spectral_exit_module();
    exit_umac();
    qdf_mod_exit();
#endif /* QCA_SINGLE_WIFI_3_0 */
    is_wifi_3_0_installed = 0;
    wifi3_ko_exit_in_progress = 0;
#endif
}

#ifndef QCA_SINGLE_WIFI_3_0
static void __exit exit_ath_pci_3_0(void)
#else
void exit_ath_pci_3_0(void)
#endif
{
    /* This function is defind as __exit and is defined in the .exit section
     * of the module. Call __exit_ath_pci_3_0 from here, which is kept outside
     * the .exit section as it can be caled from init_ath_pci_3_0 also.
     */
    __exit_ath_pci_3_0();
}
module_exit(exit_ath_pci_3_0);

#ifndef QCA_SINGLE_WIFI_3_0
static int __init init_ath_pci_3_0(void)
#else
int init_ath_pci_3_0(void)
#endif
{
#ifdef WIFI_TARGET_TYPE_3_0
#ifdef ATH_AHB
    int pciret = 0, ahbret = 0;
    int init_ath_ahb_3_0(void);
#endif

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (pld_get_global_mlo_support())
        init_mlo_cfg();
#endif

#ifdef QCA_SINGLE_WIFI_3_0
    qdf_mod_init();
    init_umac();
    spectral_init_module();
    qca_ol_mod_init();
#endif

    qdf_debug("WIFI3.0 Registration");
    pld_set_driver_mode(testmode);
    /* If ahb not enabled then initialize offload ops for 3.0 */
    ol_if_register_wifi3_0();
    wmi_tlv_init();
    ce_service_srng_init();
#if QCA_NSS_WIFILI_OFFLOAD_SUPPORT
    osif_nss_register_module(OL_WIFI_3_0, &nss_wifili_soc_ops);
#endif
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (pld_get_global_mlo_support())
        qca_ol_update_mlo_capability(false);
#endif
#ifdef ATH_AHB
    ahbret = init_ath_ahb_3_0();
    if(ahbret < 0 ) {
        qdf_info("ath_ahb: Error while registering ath wlan ahb driver");
    }
#endif

    qdf_debug("%s : %s", dev_info, version);

#if defined(ATH_PCI) && !defined(CONFIG_AHB_FW_SIM) && !defined(CONFIG_PCIE_FW_SIM)
    if (pci_register_driver(&ol_ath_pci_driver) < 0) {
        qdf_err("ath_pci: No devices found, driver not installed.");
        pci_unregister_driver(&ol_ath_pci_driver);
        return (-ENODEV);
    }
#else
    pciret = -ENODEV;
#endif

#ifdef ATH_AHB
    /*
     * Return failure only when there is no wlan device
     * on both pci and ahb buses.
     */
      if (ahbret && pciret) {
              /* which error takes priority ?? */
              return ahbret;
      }
#endif
#ifdef QCA_SINGLE_WIFI_3_0
	monitor_mod_init();
#endif
    driver_registered = true;
    sema_init(&reset_in_progress, 1);
    is_wifi_3_0_installed = 1;
#endif
    return 0;
}
module_init(init_ath_pci_3_0);
