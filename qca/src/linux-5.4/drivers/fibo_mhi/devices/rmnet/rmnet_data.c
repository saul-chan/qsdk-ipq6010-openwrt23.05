#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>

static uint nss_debug = 0;
module_param( nss_debug, uint, S_IRUGO | S_IWUSR);

/* rmnet section */

#define RMNET_FLAGS_INGRESS_DEAGGREGATION         (1U << 0)
#define RMNET_FLAGS_INGRESS_MAP_COMMANDS          (1U << 1)
#define RMNET_FLAGS_INGRESS_MAP_CKSUMV4           (1U << 2)
#define RMNET_FLAGS_EGRESS_MAP_CKSUMV4            (1U << 3)
#define RMNET_FLAGS_INGRESS_COALESCE              (1U << 4)
#define RMNET_FLAGS_INGRESS_MAP_CKSUMV5           (1U << 5)
#define RMNET_FLAGS_EGRESS_MAP_CKSUMV5            (1U << 6)

#ifdef CONFIG_ARCH_IPQ807x
#define CONFIG_QCA_NSS_DRV
#endif
#ifdef CONFIG_QCA_NSS_DRV
#include "rmnet_nss.c"
#else
#include "rmnet_nss.h"
#endif

#include "rmnet_vnd.c"
#include "rmnet_map_command.c"
#include "rmnet_map_data.c"
#include "rmnet_descriptor.c"
#include "rmnet_config.c"
#include "rmnet_handlers.c"

struct rmnet_nss_cb *rmnet_nss_callbacks __rcu __read_mostly;

void rmnet_data_init(struct net_device *real_dev, u32 nr_rmnet_devs)
{
	struct rmnet_port *port;
	struct rmnet_endpoint *ep;
	struct net_device *rmnet_dev = NULL;
	u32 nr = 0;
	struct rmnet_nss_cb *nss_cb;
	int rc = 0;

	nss_cb = rcu_dereference(rmnet_nss_callbacks);
	if (!nss_cb)
	{
#ifdef CONFIG_QCA_NSS_DRV
		pr_err("%s(): initializing rmnet_nss\n", __func__);
		RCU_INIT_POINTER(rmnet_nss_callbacks, &rmnet_nss);
#endif
	}
		
	rtnl_lock();
	rc = rmnet_register_real_device(real_dev);
	rtnl_unlock();

	if (rc) {
		pr_err("%s rmnet_register_real_device = %d\n", __func__, rc);
		return;
	}

	port = rmnet_get_port_rtnl(real_dev);
	port->data_format = RMNET_FLAGS_INGRESS_DEAGGREGATION
		| RMNET_FLAGS_INGRESS_MAP_CKSUMV5 | RMNET_FLAGS_EGRESS_MAP_CKSUMV5;
	port->rmnet_mode = RMNET_EPMODE_VND;

	for (nr = 0;  nr < nr_rmnet_devs; nr++) {
		u8 mux_id = 0x81+nr;

		ep = kzalloc(sizeof(*ep), GFP_ATOMIC);

		rtnl_lock();
		rmnet_dev = alloc_netdev(sizeof(struct rmnet_priv),
					"rmnet_data%d", NET_NAME_PREDICTABLE,
					rmnet_vnd_setup);
		
		rmnet_vnd_newlink(mux_id, rmnet_dev, port, real_dev, ep);
		netdev_rx_handler_register(rmnet_dev, rmnet_rx_priv_handler, NULL);
		rtnl_unlock();

		hlist_add_head_rcu(&ep->hlnode, &port->muxed_ep[mux_id]);
	}

	port->nr_rmnet_devs = nr_rmnet_devs;
}

void rmnet_data_deinit(struct net_device *real_dev, u32 nr_rmnet_devs)
{
	struct rmnet_port *port;
	u32 nr = 0;
	struct rmnet_nss_cb *nss_cb;

	if (!real_dev || !rmnet_is_real_dev_registered(real_dev))
		return;

	port = rmnet_get_port_rtnl(real_dev);

	for (nr = 0;  nr < nr_rmnet_devs; nr++) {
		struct rmnet_endpoint *ep;
		u8 mux_id = 0x81+nr;

		ep = rmnet_get_endpoint(port, mux_id);
		if (ep) {
			hlist_del_init_rcu(&ep->hlnode);
			rmnet_vnd_dellink(mux_id, port, ep);
			synchronize_rcu();
			kfree(ep);
		}
	}

	rmnet_unregister_real_device(real_dev, port);

	nss_cb = rcu_dereference(rmnet_nss_callbacks);
	if (nss_cb) {
#ifdef CONFIG_QCA_NSS_DRV
		struct hlist_node *tmp;
		struct rmnet_nss_ctx *ctx;
		int bkt;

		pr_err("%s(): exiting rmnet_nss\n", __func__);
		RCU_INIT_POINTER(rmnet_nss_callbacks, NULL);

		/* Tear down all NSS contexts */
		hash_for_each_safe(rmnet_nss_ctx_hashtable, bkt, tmp, ctx, hnode)
			rmnet_nss_free_ctx(ctx);
#endif
	}
}

