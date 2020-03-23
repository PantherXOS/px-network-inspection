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

#define MAX_ROOTS_NUMBER 10
#define MAX_PHYS_IFS MAX_ROOTS_NUMBER
#define MAX_TAP_IFS 10
#define MAX_TUN_IFS 10
#define MAX_VPN_NAME 50
#define MAX_VPN_PROFILE_NAME 256

#define ROUTENODE(o) (RouteNode*)(o)
#define VPNMETHOD(o) (VpnMethod*)(o)

enum VPN_METHODS
{
	OPENVPN,
	WIREGUARD,
	ANYCONNECT,
	VPN_METHODS_NUMBERS,
	NO_VPN_METHOD,
	VPN_METHOD_UNKNOWN
};

static char vpn_methods_string[VPN_METHOD_UNKNOWN + 1][MAX_VPN_NAME] =
{
	"OPENVPN",
	"WIREGUARD",
	"ANYCONNECT",
	"VPN_METHODS_NUMBERS",
	"NO_VPN_METHOD",
	"VPN_METHOD_UNKNOWN"
};

typedef struct vpnmethod
{
	enum VPN_METHODS vpn_method;
	char vpn_name[MAX_VPN_NAME];
} VpnMethod;

VpnMethod  *vpn_method_new();
void vpn_method_free();
VpnMethod *detect_vpn_method();

typedef struct route_node
{
	char if_name[IFNAMSIZ];
	char dst_ipv4[16];
	char gateway_ipv4[16];
	int priority;
}RouteNode;

RouteNode* route_node_new();
void route_node_free(RouteNode *nd);
int analyze_kernel_route(GNode *[MAX_ROOTS_NUMBER], int *kernel_roots, enum VPN_METHODS vpn_method);
GNode* get_kernel_route_node(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int roots, char *ifa_name);

#endif
