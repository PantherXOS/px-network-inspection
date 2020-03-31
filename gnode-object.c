#include <gnode-object.h>
#include <stdio.h>

/**
 *  @brief Creates a new ne_device object.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@return	NULL if cannot allocate object. Otherwise, the newly created object
 */
NetDevice* net_device_new()
{
	NetDevice *nd;
	nd = g_new(NetDevice, 1);
	return nd;
}

/**
 *  @brief destroys the NetDevice object.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@todo	TODO	free JSNO objects
 *
 * 	@param[in]	nd	the object to be destroyed.
 */
void net_device_free(NetDevice *nd)
{
	if (nd->jobj)
	{
		// TODO: Do free jobj.
	}
}

/**
 *  @brief creates the JSON representation.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	device	the device that is needed to have its JSON object be created
 */
void internal_traverse_json_func(NetDevice *device)
{
	device->jobj = json_object_new_object();
	if (device->public_device)
	{
		internal_traverse_json_func(device->public_device);
	}

	// Update position
	//device->dev_pos = g_node_depth(node);
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

	if (!strcmp(device->dev_method, "WIFI"))
	{
		json_object *dev_wifi_ap = json_object_new_string(device->dev_wifi_ap);
		json_object_object_add(device->jobj, "essid", dev_wifi_ap);
	}

	if (!strcmp(device->dev_type, "virtual"))
	{
		json_object *dev_vpn_profile = json_object_new_string(device->dev_vpn_profile);
		json_object_object_add(device->jobj, "profile", dev_vpn_profile);
	}
}

/**
 *  @brief creates the JSON representation for all objects of a tree.
 *
 *  @see	https://wiki.pantherx.org
 *
 *  @todo	TODO	Use user data
 *
 * 	@param[in]	node	the root of the tree to be traversed.
 * 	@param[in, out]	data	the data passed to call-back function. It may be used as both input and output
 * 	@return	the TRUE shows that stop traverse
 */
gboolean traverse_json_func(GNode * node, gpointer data)
{
	NetDevice *device = NETDEVICE(node->data);

	internal_traverse_json_func(device);
	return FALSE;
}

/**
 *  @brief adds all objects of a tree to a JSON array.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	node	the root of the tree to be traversed.
 * 	@param[in, out]	data	the data passed to call-back function. It may be used as both input and output
 * 	@return	the TRUE shows that stop traverse
 */
gboolean traverse_json_array_func(GNode *node, gpointer data)
{
	NetDevice *device = NETDEVICE(node->data);
	json_object *jarray = (json_object*)data;
	if (G_NODE_IS_ROOT(node))
	{
		// Adds the public device
		if (device->public_device)
			if (device->public_device->jobj)
				json_object_array_add(jarray, device->public_device->jobj);
	}

	if (device->jobj == NULL) return FALSE;

	json_object_array_add(jarray, device->jobj);

	return FALSE;
}
