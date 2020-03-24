#ifndef ROUTE_TREE_H_
#define ROUTE_TREE_H_

#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/types.h>
#include <netlink/list.h>
#include <netlink/cli/utils.h>
#include <netlink/cli/route.h>
#include <netlink/cli/link.h>

#include <linux/netlink.h>

#include <glib.h>
#include <stdint.h>
#include <linux/limits.h>

#define MAX_ROOTS_NUMBER 10
#define MAX_PHYS_IFS MAX_ROOTS_NUMBER
#define MAX_TAP_IFS 10
#define MAX_TUN_IFS 10
#define MAX_VPN_NAME 50
#define MAX_VPN_PROFILE_NAME NAME_MAX

#define ROUTENODE(o) (RouteNode*)(o)
#define VPNMETHOD(o) (VpnMethod*)(o)

///	@brief	the supported methods for running VPN on a machine
enum VPN_METHODS
{
	OPENVPN,
	WIREGUARD,
	ANYCONNECT,
	///	@brief	represents the number of supported VPN methods
	VPN_METHODS_NUMBERS,
	NO_VPN_METHOD,
	VPN_METHOD_UNKNOWN
};

///	@brief	the associated name of the VPN methods. Used for JSON representation
static char vpn_methods_string[VPN_METHOD_UNKNOWN + 1][MAX_VPN_NAME] =
{
	"OPENVPN",
	"WIREGUARD",
	"ANYCONNECT",
	"VPN_METHODS_NUMBERS",
	"NO_VPN_METHOD",
	"VPN_METHOD_UNKNOWN"
};

///	@brief	the struct to be put both VPN method and name together
typedef struct vpnmethod
{
	enum VPN_METHODS vpn_method;
	char vpn_name[MAX_VPN_NAME];
} VpnMethod;

///	@brief	creates a new VpnMethod object
VpnMethod  *vpn_method_new();

///	@brief	destroys a VpnMethod object
void vpn_method_free(VpnMethod * vm);

/**
 *  @brief detect VPN method based on kernel routing pattern
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO	Support tap-based interfaces.
 *  	TODO	Support complicated patterns.
 */
VpnMethod *detect_vpn_method();

///	@brief	the extracted information of a node in a kernel route table
typedef struct route_node
{
	char if_name[IFNAMSIZ];
	char dst_ipv4[16];
	char gateway_ipv4[16];
	int priority;
}RouteNode;

///	@brief	creates a new RouteNode object
RouteNode* route_node_new();

///	@brief	destroys a RouteNode object
void route_node_free(RouteNode *nd);

/**
 *  @brief detect VPN method based on kernel routing pattern
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO	Support tap-based interfaces.
 *  	TODO	Support complicated patterns.
 *
 *  @param[out]	kernel_route_roots	indicates the jungle of the kernel route table
 *  @param[out]	kernel_roots	the number of roots in jungle
 *  @param[in]	vpn_method	the VPN method detected
 *  @return	the index of the primary kernel route
 *
 *  @pre	the vpn method has to be detected
 */
int analyze_kernel_route(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int *kernel_roots, enum VPN_METHODS vpn_method);

/**
 *  @brief find the node with the interface name within the jungle of kernel route table
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO	Support tap-based interfaces.
 *  	TODO	Support complicated patterns.
 *
 *  @param[in]	kernel_route_roots	indicates the jungle of the kernel route table
 *  @param[in]	roots	the number of roots in jungle
 *  @param[in]	ifa_name	the name to be searched for
 *  @return	the node if any find
 *
 *  @pre	the kernel route table has to be retrieved before
 */
GNode* get_kernel_route_node(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int roots, char *ifa_name);

#endif
