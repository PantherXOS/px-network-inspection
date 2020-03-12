#ifndef GNODE_OBJECT_H_
#define GNODE_OBJECT_H_

#include <glib.h>
#include <stdint.h>
#include <json-c/json.h>
#include <linux/wireless.h>

#define NETDEVICE(o) (NetDevice*)(o)

typedef struct net_device
{
	gboolean is_set;
	uint32_t dev_pos;
	char dev_name[10];
	char dev_type[20];
	char dev_method[30];
	char dev_ip4[16];
	char dev_ip6[40];
	char dev_gateway[40];	// May be ipv6
	char dev_dns[40];
	char dev_active[10];
	char dev_wifi_ap[IW_ESSID_MAX_SIZE + 1];
	size_t phy_index;
	json_object *jobj;
} NetDevice;


NetDevice* net_device_new();
void net_device_free(NetDevice *nd);
gboolean traverse_json_func(GNode * node, gpointer data);
gboolean traverse_json_array_func(GNode *node, gpointer data);

#endif
