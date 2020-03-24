#ifndef GNODE_OBJECT_H_
#define GNODE_OBJECT_H_

#include <glib.h>
#include <stdint.h>
#include <json-c/json.h>
#include <linux/wireless.h>
#include <linux/limits.h>

#define NETDEVICE(o) (NetDevice*)(o)

struct net_device;

///	@brief	the struct stores the required information of network device in routes
typedef struct net_device
{
	///	@brief	if the struct is set the has value TRUE
	gboolean is_set;
	/// @brief	the depth of device in the local routing tree
	uint32_t dev_pos;
	///	@brief	the device name
	char dev_name[16];
	///	@brief	the type device	(physical and virtual)
	char dev_type[20];
	///	@brief	the method device uses (e.g. wifi)
	char dev_method[30];
	///	@brief	the device IPV4
	char dev_ip4[16];
	///	@brief	the device IPV6
	char dev_ip6[40];
	///	@brief	the gateway device uses
	///	@note	may contain IPV6
	char dev_gateway[40];
	///	@brief	the dns device uses
	///	@note	may contain IPV6
	char dev_dns[40];
	///	@brief	the status of device (ACTIVE and NOACTIVE)
	char dev_active[10];
	///	@brief	the access point name for wifi networks
	char dev_wifi_ap[IW_ESSID_MAX_SIZE + 1];
	///	@brief	the index in the static global physical interfaces list. It may indicates the associated interface of a TUN device.
	size_t phy_index;
	///	@brief	the vpn profile name if the device used for a VPN.
	char dev_vpn_profile[NAME_MAX];
	///	@brief	the pointer to the public (remote) device. It has value if the device is a physical device and it has a connection to the internet
	struct net_device *public_device;
	///	@brief	the JSON representation of this device
	json_object *jobj;
} NetDevice;


/**
 *  @brief Creates a new ne_device object.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@return	NULL if cannot allocate object. Otherwise, the newly created object
 */
NetDevice* net_device_new();

/**
 *  @brief destroys the NetDevice object.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@todo	TODO	free JSNO objects
 *
 * 	@param[in]	nd	the object to be destroyed.
 */
void net_device_free(NetDevice *nd);

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
gboolean traverse_json_func(GNode * node, gpointer data);

/**
 *  @brief adds all objects of a tree to a JSON array.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	node	the root of the tree to be traversed.
 * 	@param[in, out]	data	the data passed to call-back function. It may be used as both input and output
 * 	@return	the TRUE shows that stop traverse
 */
gboolean traverse_json_array_func(GNode *node, gpointer data);

#endif
