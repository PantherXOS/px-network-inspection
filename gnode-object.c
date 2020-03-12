#include <gnode-object.h>

NetDevice* net_device_new()
{
	NetDevice *nd;
	nd = g_new (NetDevice, 1);
	return nd;
}

void net_device_free(NetDevice *nd)
{
	if (nd->jobj)
	{
		// TODO: Do free jobj.
	}
}

// TODO: Use user data.
gboolean traverse_json_func(GNode * node, gpointer data)
{
	NetDevice *device = NETDEVICE(node->data);
	device->jobj = json_object_new_object();

	// Update position
	device->dev_pos = g_node_depth(node);
	json_object *dev_pos = json_object_new_int(device->dev_pos);
	json_object_object_add(device->jobj, "pos", dev_pos);

	json_object *dev_name = json_object_new_string(device->dev_name);
	json_object_object_add(device->jobj, "adapter", dev_name);

	json_object *dev_method = json_object_new_string(device->dev_method);
	json_object_object_add(device->jobj, "method", dev_method);

	json_object *dev_type = json_object_new_string(device->dev_type);
	json_object_object_add(device->jobj, "type", dev_type);

	json_object *dev_ip4 = json_object_new_string(device->dev_ip4);
	json_object_object_add(device->jobj, "ip4", dev_ip4);

	json_object *dev_ip6 = json_object_new_string(device->dev_ip6);
	json_object_object_add(device->jobj, "ip6", dev_ip6);

	json_object *dev_dns = json_object_new_string(device->dev_dns);
	json_object_object_add(device->jobj, "dns", dev_dns);

	json_object *dev_gateway = json_object_new_string(device->dev_gateway);
	json_object_object_add(device->jobj, "gateway", dev_gateway);

	json_object *dev_active = json_object_new_string(device->dev_active);
	json_object_object_add(device->jobj, "status", dev_active);

	if (!strcmp(device->dev_method, "wifi"))
	{
		json_object *dev_wifi_ap = json_object_new_string(device->dev_wifi_ap);
		json_object_object_add(device->jobj, "essid", dev_wifi_ap);
	}

	// Create joson for each node, which is a device.

	return FALSE;
}

gboolean traverse_json_array_func(GNode *node, gpointer data)
{
	if (G_NODE_IS_ROOT(node)) return FALSE;

	NetDevice *device = NETDEVICE(node->data);
	if (device->jobj == NULL) return FALSE;

	json_object *jarray = (json_object*)data;
	json_object_array_add(jarray, device->jobj);
	return FALSE;
}
