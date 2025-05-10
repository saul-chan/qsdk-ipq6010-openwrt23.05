/*
 * Copyright (c) 2017-2018, 2022-2024 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2016-2017 The Linux Foundation. All rights reserved.
 */

#ifndef __PLD_COMMON_STUB_H__
#define __PLD_COMMON_STUB_H__

#if CONFIG_PLD_STUB
#include <platform_device.h>
#include <qdf_module.h>
#include <linux/types.h>
static inline u64 pld_get_q6_time(struct device *dev)
{
	return 0;
}
static inline int pld_set_bar_addr(struct device *dev, void __iomem *mem)
{
	return 0;
}
static inline int pld_reg_read(struct device *dev, u32 addr, u32 *val, void __iomem *base)
{
	return 0;
}
static inline int pld_reg_write(struct device *dev, u32 addr, u32 val, void __iomem *base)
{
	return 0;
}
static inline int pld_init(void) {return 0;}
static inline void pld_deinit(void) { }

static inline int pld_register_driver(struct pld_driver_ops *ops)
{
	return 0;
}

static inline void pld_unregister_driver(void){ }

static inline bool pld_is_dev_initialized(struct device *dev)
{
	return false;
}

static inline int pld_wlan_enable(struct device *dev,
				  struct pld_wlan_enable_cfg *config,
				  enum pld_driver_mode mode)
{
	return 0;
}
static inline int pld_wlan_disable(struct device *dev,
				   enum pld_driver_mode mode)
{
	return 0;
}
static inline void pld_is_pci_link_down(struct device *dev)
{
}
static inline int pld_wlan_pm_control(struct device *dev, bool vote)
{
	return 0;
}
static inline void pld_intr_notify_q6(struct device *dev)
{
}

static inline int pld_get_user_msi_assignment(struct device *dev,
			char *user_name, int *num_vectors,
			uint32_t *user_base_data, uint32_t *base_vector)
{
	return -EINVAL;
}

/* should not be called if pld_get_user_msi_assignment returns error */
static inline int pld_get_msi_irq(struct device *dev, unsigned int vector)
{
	return -EINVAL;
}

static inline int pld_get_pci_slot(struct device *dev)
{
	return 0;
}

/* should not be called if pld_get_user_msi_assignment returns error */
static inline void pld_get_msi_address(struct device *dev,
				       uint32_t *msi_addr_low,
				       uint32_t *msi_addr_high)
{
	return;
}

static inline int pld_get_irq(struct device *dev, int ce_id)
{
	return -EINVAL;
}

static inline int pld_ce_request_irq(struct device *dev, unsigned int ce_id,
				     irqreturn_t (*handler)(int, void *),
				     unsigned long flags, const char *name,
				     void *ctx)
{
	return 0;
}

static inline void pld_wait_for_fw_ready(struct device *dev)
{
}

static inline void *pld_subsystem_get(struct device *dev, int device_id)
{
	return NULL;
}
static inline void pld_subsystem_put(struct device *dev)
{

}

static inline void *pld_get_pci_dev_from_plat_dev(void *pdev)
{
	return NULL;
}

static inline void *pld_get_pci_dev_id_from_plat_dev(void *pdev)
{
	return NULL;
}

static inline int pld_ce_free_irq(struct device *dev,
				  unsigned int ce_id, void *ctx)
{
	return 0;
}
static inline int pld_get_soc_info(struct device *dev,
				   struct pld_soc_info *info)
{
	return 0;
}
static inline int pld_get_ce_id(struct device *dev, int irq)
{
	return 0;
}
static inline void pld_runtime_init(struct device *dev, int auto_delay)
{
}
static inline void pld_runtime_exit(struct device *dev)
{
}
static inline int pld_athdiag_read(struct device *dev,
				   uint32_t offset, uint32_t memtype,
				   uint32_t datalen, uint8_t *output)
{
	return 0;
}
static inline int pld_athdiag_write(struct device *dev,
				    uint32_t offset, uint32_t memtype,
				    uint32_t datalen, uint8_t *input)
{
	return 0;
}
static inline struct sk_buff *pld_nbuf_pre_alloc(size_t size)
{
	return NULL;
}
static inline int pld_nbuf_pre_alloc_free(struct sk_buff *skb)
{
	return 0;
}
static inline void *pld_get_pdev_device_id(int device_id, enum pld_bus_type type)
{
	return NULL;
}
static inline void pld_remove_bus(enum pld_bus_type type)
{

}
static inline int pld_rescan_bus(enum pld_bus_type type)
{
	return -EINVAL;
}

static inline bool pld_have_platform_driver_support(struct device *dev)
{
	return false;
}

static inline enum pld_bus_type pld_get_bus_type(struct device *dev)
{
	return PLD_BUS_TYPE_NONE;
}

static inline void pld_set_recovery_enabled(struct device *dev, bool enabled)
{
}

static inline void pld_get_ramdump_device_name(struct device *dev,
					char *ramdump_dev_name,
					size_t ramdump_dev_name_len)
{
}
static inline unsigned int pld_get_driver_mode(void)
{
	return PLD_MISSION;
}
static inline int pld_set_driver_mode(unsigned int mode)
{
	return -EINVAL;
}
static inline bool pld_get_global_mlo_support(void)
{
	return false;
}
static inline int pld_send_buffer_to_afcmem(struct device *dev, char *afcdb,
					    uint32_t len, uint8_t slotid)
{
	return -EINVAL;
}
static inline int pld_reset_afcmem(struct device *dev, uint8_t slotid)
{
	return -EINVAL;
}
static inline int pld_get_mlo_chip_id(struct device *dev)
{
	return -EINVAL;
}
int pld_set_mlo_config(struct pld_mlo_group_info *mlo_arg,
		       struct pld_mlo_param *mlo_param)
{
	return -EINVAL;
}
int pld_set_mlo_group_config(struct pld_mlo_group_info *mlo_arg,
			     uint8_t group_id)
{
	return -EINVAL;
}
int pld_reset_mlo_config(uint8_t num_groups)
{
	return -EINVAL;
}
int pld_reset_mlo_group_config(uint8_t group_id)
{
	return -EINVAL;
}
static inline bool pld_get_mlo_capable(struct device *dev)
{
	return false;
}
static inline int pld_get_num_mlo_links(struct device *dev)
{
	return -EINVAL;
}
static inline int pld_get_mlo_group_id(struct device *dev)
{
	return -EINVAL;
}
static inline int pld_get_mlo_chip_info(struct device *dev, struct pld_mlo_chip_info *pld_chip_info)
{
	return -EINVAL;
}
static inline int pld_get_num_mlo_capable_devices(unsigned int *device_id, int num_elements)
{
	return -EINVAL;
}
static inline int pld_get_max_mlo_chips(struct device *dev)
{
	return -EINVAL;
}
static inline int pld_get_dev_link_ids(struct device *dev, u8 *linkid,
				       int num_elements)
{
	return -EINVAL;
}
static inline bool pld_get_mlo_group_info(uint8_t grp_id, struct cnss_mlo_group_info *info)
{
	return -EINVAL;
}
static inline int pld_get_num_mlo_groups(void)
{
	return -EINVAL;
}
static inline int pld_get_mlo_global_config_region_info(struct device *dev,
							void **bar,
							int *num_bytes)
{
	return -EINVAL;
}
static bool pld_get_enable_intx(struct device *dev)
{
	return false;
}
static void *pld_get_plat_dev_by_bus_dev(struct device *dev)
{
	return NULL;
}

static inline void pld_set_led_gpio(int led_gpio, unsigned int value, unsigned int flags)
{
	return;
}

static inline void pld_set_recovery_mode(struct device *dev, u8 recovery_mode)
{
	return;
}
static inline void pld_set_standby_mode(struct device *dev, u8 standby_mode)
{
	return;
}
static inline void pld_set_pci_link_speed_width(struct device *dev,
						u16 link_speed,
						u16 link_width)
{
	return;
static inline int pld_set_wsi_remap(struct device *dev)
{
	return -EINVAL;
}
static inline void pld_set_wsi_remap_state(struct device *dev, bool state)
{
}
#endif
#endif
