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


void get_route_trees(int argc, char *argv[]);

#endif
