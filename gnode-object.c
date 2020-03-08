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
	gboolean result = TRUE;

	NetDevice *device = NETDEVICE(node->data);
	device->jobj = json_object_new_object();

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
	// Create joson for each node, which is a device.

	return result;
}
