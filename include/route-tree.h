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

#define ROUTENODE(o) (RouteNode*)(o)

typedef struct route_node
{
	char if_name[IFNAMSIZ];
	char dst_ipv4[16];
	char gateway_ipv4[16];
	int priority;
}RouteNode;

RouteNode* route_node_new();
void route_node_free(RouteNode *nd);
int analyze_kernel_route(GNode *[MAX_ROOTS_NUMBER], int *kernel_roots);
GNode* get_kernel_route_node(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int roots, char *ifa_name);

#endif
