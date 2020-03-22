#include <route-tree.h>
#include <limits.h>

VpnMethod* vpn_method_new()
{
	VpnMethod *vm = g_new(VpnMethod, 1);
	vm->vpn_method = VPN_METHOD_UNKNOWN;
	bzero(vm->vpn_name, MAX_VPN_NAME);
	return vm;
}

void vpn_method_free()
{}

RouteNode* route_node_new()
{
	RouteNode *rn;
	rn = g_new(RouteNode, 1);
	return rn;
}

void route_node_free(RouteNode *nd)
{}

typedef struct node_params
{
	GNode **node;
	int *roots;
	char buf[16];
	char gateway_ipv4[16];
} NodeParams;

static void print_usage(void)
{
	printf(
	"Usage: nl-route-list [OPTION]... [ROUTE]\n"
	"\n"
	"Options\n"
	" -c, --cache           List the contents of the route cache\n"
	" -f, --format=TYPE	Output format { brief | details | stats }\n"
	" -h, --help            Show this help\n"
	" -v, --version		Show versioning information\n"
	"\n"
	"Route Options\n"
	" -d, --dst=ADDR        destination prefix, e.g. 10.10.0.0/16\n"
	" -n, --nexthop=NH      nexthop configuration:\n"
	"                         dev=DEV         route via device\n"
	"                         weight=WEIGHT   weight of nexthop\n"
	"                         flags=FLAGS\n"
	"                         via=GATEWAY     route via other node\n"
	"                         realms=REALMS\n"
	"                         e.g. dev=eth0,via=192.168.1.12\n"
	" -t, --table=TABLE     Routing table\n"
	"     --family=FAMILY	Address family\n"
	"     --src=ADDR        Source prefix\n"
	"     --iif=DEV         Incomming interface\n"
	"     --pref-src=ADDR   Preferred source address\n"
	"     --metrics=OPTS    Metrics configurations\n"
	"     --priority=NUM    Priotity\n"
	"     --scope=SCOPE     Scope\n"
	"     --protocol=PROTO  Protocol\n"
	"     --type=TYPE       { unicast | local | broadcast | multicast }\n"
	);
	exit(0);
}

void first_entry_cb(struct nl_object * obj, void * data)
{
	enum VPN_METHODS *vpn_method = (enum VPN_METHODS*)data;
	if ((*vpn_method) != VPN_METHOD_UNKNOWN) return;
	struct rtnl_route *r = (struct rtnl_route *) obj;
	char buf[64];
	bzero(buf, 64);
	size_t dst_len =  nl_addr_get_len(rtnl_route_get_dst(r));
	if (dst_len == 0)
	{
		if (rtnl_route_get_scope(r) == 253)	// link scope = 253
		{
			(*vpn_method) = ANYCONNECT;
		}
		else
		{
			(*vpn_method) = NO_VPN_METHOD;
		}
	}
	else
	{
		nl_addr2str(rtnl_route_get_dst(r), buf, sizeof(buf));
		if (!strncmp(buf, "0.0.0.0/1", sizeof("0.0.0.0/1")))
		{
			if (rtnl_route_get_scope(r) == 0)	// global scope = 0
			{
				(*vpn_method) = OPENVPN;
			}
			else if (rtnl_route_get_scope(r) == 253)	// link scope = 253
			{
				(*vpn_method) = WIREGUARD;
			}
			else
			{
				(*vpn_method) = NO_VPN_METHOD;
			}
		}
		else
		{
				(*vpn_method) = NO_VPN_METHOD;
		}
	}
}

void next_hop_entry_cb(struct rtnl_nexthop *nh, void *data)
{
	NodeParams *nparam = (NodeParams *) data;
	*(nparam->roots) = *(nparam->roots) + 1;
	struct nl_cache *link_cache;
	link_cache = nl_cache_mngt_require_safe("route/link");
	rtnl_link_i2name(link_cache, rtnl_route_nh_get_ifindex(nh), nparam->buf, 16);
	nl_addr2str(rtnl_route_nh_get_gateway(nh), nparam->gateway_ipv4, 16);
}

void route_entry_cb(struct nl_object * obj, void * data)
{
	struct rtnl_route *r = (struct rtnl_route *) obj;
	struct rtnl_nexthop *nh;
	rtnl_route_foreach_nexthop(r, next_hop_entry_cb, data);
	NodeParams *nparam = (NodeParams *) data;

	RouteNode *rn = route_node_new();

	strncpy(rn->if_name, nparam->buf, sizeof(nparam->buf));

	char buf[64];
	bzero(buf, 64);
	size_t dst_len =  nl_addr_get_len(rtnl_route_get_dst(r));
	if (dst_len == 0)
		strncpy(rn->dst_ipv4, "default", sizeof("default"));
	else
	{
		nl_addr2str(rtnl_route_get_dst(r), buf, sizeof(buf));
		strncpy(rn->dst_ipv4, buf, sizeof(rn->dst_ipv4));
	}

	strncpy(rn->gateway_ipv4, nparam->gateway_ipv4, sizeof(nparam->gateway_ipv4));
	rn->priority = rtnl_route_get_priority(r);

	GNode *new_node = g_node_new(rn);
	nparam->node[*(nparam->roots) - 1] = new_node;
}

enum ROUTE_QUERY
{
	ROUTE_QUERY_GET_TREE,
	ROUTE_QUERY_GET_FIRST
};

void get_route_trees(enum ROUTE_QUERY query_id, GNode *node[MAX_ROOTS_NUMBER], int *roots, int argc, char *argv[])
{
	// To set argument parsing to 1
	optind = 1;
	struct nl_sock *sock;
	struct nl_cache *link_cache, *route_cache;
	struct rtnl_route *route;
	struct nl_dump_params params = {
		.dp_fd = stdout,
		.dp_type = NL_DUMP_LINE,
	};
	int print_cache = 0;

	sock = nl_cli_alloc_socket();
	nl_cli_connect(sock, NETLINK_ROUTE);
	link_cache = nl_cli_link_alloc_cache(sock);
	route = nl_cli_route_alloc();

	for (;;) {
		int c, optidx = 1;
		enum {
			ARG_FAMILY = 257,
			ARG_SRC = 258,
			ARG_IIF,
			ARG_PREF_SRC,
			ARG_METRICS,
			ARG_PRIORITY,
			ARG_SCOPE,
			ARG_PROTOCOL,
			ARG_TYPE,
		};
		struct option long_opts[] = {
			{ "cache", 0, 0, 'c' },
			{ "format", 1, 0, 'f' },
			{ "help", 0, 0, 'h' },
			{ "version", 0, 0, 'v' },
			{ "dst", 1, 0, 'd' },
			{ "nexthop", 1, 0, 'n' },
			{ "table", 1, 0, 't' },
			{ "family", 1, 0, ARG_FAMILY },
			{ "src", 1, 0, ARG_SRC },
			{ "iif", 1, 0, ARG_IIF },
			{ "pref-src", 1, 0, ARG_PREF_SRC },
			{ "metrics", 1, 0, ARG_METRICS },
			{ "priority", 1, 0, ARG_PRIORITY },
			{ "scope", 1, 0, ARG_SCOPE },
			{ "protocol", 1, 0, ARG_PROTOCOL },
			{ "type", 1, 0, ARG_TYPE },
			{ 0, 0, 0, 0 }
		};
		c = getopt_long(argc, argv, "cf:hvd:n:t:", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
			case 'c': print_cache = 1; break;
			case 'f': params.dp_type = nl_cli_parse_dumptype(optarg); break;
			case 'h': print_usage(); break;
			case 'v': nl_cli_print_version(); break;
			case 'd': nl_cli_route_parse_dst(route, optarg); break;
			case 'n': nl_cli_route_parse_nexthop(route, optarg, link_cache); break;
			case 't': nl_cli_route_parse_table(route, optarg); break;
			case ARG_FAMILY: nl_cli_route_parse_family(route, optarg); break;
			case ARG_SRC: nl_cli_route_parse_src(route, optarg); break;
			case ARG_IIF: nl_cli_route_parse_iif(route, optarg, link_cache); break;
			case ARG_PREF_SRC: nl_cli_route_parse_pref_src(route, optarg); break;
			case ARG_METRICS: nl_cli_route_parse_metric(route, optarg); break;
			case ARG_PRIORITY: nl_cli_route_parse_prio(route, optarg); break;
			case ARG_SCOPE: nl_cli_route_parse_scope(route, optarg); break;
			case ARG_PROTOCOL: nl_cli_route_parse_protocol(route, optarg); break;
			case ARG_TYPE: nl_cli_route_parse_type(route, optarg); break;
		}
	}

	route_cache = nl_cli_route_alloc_cache(sock,
			print_cache ? ROUTE_CACHE_CONTENT : 0);

	if (query_id == ROUTE_QUERY_GET_TREE)
	{
		if (node != NULL)
		{
			NodeParams nparam;
			nparam.node = node;
			nparam.roots = roots;

			nl_cache_foreach_filter(route_cache, OBJ_CAST(route), &route_entry_cb , &nparam);
		}
	}
	else if (query_id == ROUTE_QUERY_GET_FIRST)
	{
		if (node != NULL)
		{
			enum VPN_METHODS vpn_method = VPN_METHOD_UNKNOWN;
			nl_cache_foreach_filter(route_cache, OBJ_CAST(route), &first_entry_cb, &vpn_method);
			VpnMethod *vm = (VPNMETHOD(node[0]->data));
			vm->vpn_method = vpn_method;
			strncpy(vm->vpn_name, vpn_methods_string[vpn_method], sizeof(vpn_methods_string[vpn_method]));
		}
	}
	else
	{
		printf("Wrong route query\n");
		exit(1);
	}

	//nl_cache_dump_filter(route_cache, &params, OBJ_CAST(route));

	nl_close(sock);
	nl_socket_free(sock);
	nl_cache_clear(link_cache);
	nl_cache_free(link_cache);

	nl_cache_clear(route_cache);
	nl_cache_free(route_cache);
	nl_object_free((struct nl_object *)route);
}

int analyze_openvpn_kernel_route(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int *kernel_roots)
{
	int roots = 0;

	GNode *k_rr_l1[MAX_ROOTS_NUMBER];
	char *c[40] = {"", "-f", "details", "--family", "inet", "--scope", "global"};
	get_route_trees(ROUTE_QUERY_GET_TREE, k_rr_l1, &roots, 7, c);

	//GNode *k_rr_l2[MAX_ROOTS_NUMBER];
	//int non_roots = 0;
	//char *a[40] = {"", "-f", "details", "--family", "inet", "--scope", "link", "--table", "main", "-d", "default"};
	//get_route_trees(ROUTE_QUERY_GET_TREE, k_rr_l2, &non_roots, 11, a);

	RouteNode *rn;
	int min_priority = INT_MAX;
	int primary_index = -1;
	int j = 0;
	for (int i = 0; i < roots; i++)	// First path
	{
		rn = ROUTENODE(k_rr_l1[i]->data);
		if (strncmp(rn->dst_ipv4, "0.0.0.0/1", sizeof("0.0.0.0/1")) && strncmp(rn->dst_ipv4, "128.0.0.0/1", sizeof("128.0.0.0/1")))
		{
			if (min_priority == rn->priority)	// TODO: Help to detect if there is more than one output NIC. ip rule, ip route tables and ...
			{
				primary_index = -1;
			}
			if (min_priority > rn->priority)
			{
				primary_index = j;
				min_priority = rn->priority;
			}

			kernel_route_roots[j++] = k_rr_l1[i];
		}
	}

	if (primary_index >= 0)
	{
		for (int i = 0; i < roots; i++)
		{
			rn = ROUTENODE(k_rr_l1[i]->data);
			if (!strncmp(rn->dst_ipv4, "0.0.0.0/1", sizeof("0.0.0.0/1")))
			{
				// Add to parent
				g_node_append(kernel_route_roots[primary_index], k_rr_l1[i]);
			}
		}
	}

	//printf("primary_index: %d\n", primary_index);

	//if (primary_index >= 0)
	//{
	//	rn = ROUTENODE(k_rr_l1[primary_index]->data);
	//	//printf("There are %d kernel roots: %s\t via %s to\t%s\t%d\n", roots, rn->if_name, rn->gateway_ipv4, rn->dst_ipv4, rn->priority);
	//	for (int i = 0; i < non_roots; i++)	// Assume that VPNs goes through primary
	//	{
	//		g_node_append(k_rr_l1[primary_index], k_rr_l2[i]);
	//	}
	//}
	//else TODO: implement the condition when there is two or more NICs for output/

	*kernel_roots = j;
	return primary_index;
}

int analyze_anyconnect_kernel_route(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int *kernel_roots)
{
	int roots = 0;

	GNode *k_rr_l1[MAX_ROOTS_NUMBER];
	char *c[40] = {"", "-f", "details", "--family", "inet", "--scope", "global"};
	get_route_trees(ROUTE_QUERY_GET_TREE, k_rr_l1, &roots, 7, c);

	GNode *k_rr_l2[MAX_ROOTS_NUMBER];
	int non_roots = 0;
	char *a[40] = {"", "-f", "details", "--family", "inet", "--scope", "link", "--table", "main", "-d", "default"};
	get_route_trees(ROUTE_QUERY_GET_TREE, k_rr_l2, &non_roots, 11, a);

	RouteNode *rn;
	int min_priority = INT_MAX;
	int primary_index = -1;
	for (int i = 0; i < roots; i++)
	{
		rn = ROUTENODE(k_rr_l1[i]->data);

		if (min_priority == rn->priority)	// TODO: Help to detect if there is more than one output NIC. ip rule, ip route tables and ...
		{
			primary_index = -1;
		}
		if (min_priority > rn->priority)
		{
			primary_index = i;
			min_priority = rn->priority;
		}

		kernel_route_roots[i] = k_rr_l1[i];
	}

	//printf("primary_index: %d\n", primary_index);

	if (primary_index >= 0)
	{
		rn = ROUTENODE(k_rr_l1[primary_index]->data);
		//printf("There are %d kernel roots: %s\t via %s to\t%s\t%d\n", roots, rn->if_name, rn->gateway_ipv4, rn->dst_ipv4, rn->priority);
		for (int i = 0; i < non_roots; i++)	// Assume that VPNs goes through primary
		{
			g_node_append(k_rr_l1[primary_index], k_rr_l2[i]);
		}
	}
	//else TODO: implement the condition when there is two or more NICs for output/

	*kernel_roots = roots;
	return primary_index;
}

int analyze_kernel_route(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int *kernel_roots, enum VPN_METHODS vpn_method)
{
	int result = -1;
	switch (vpn_method)
	{
		case NO_VPN_METHOD:
		case ANYCONNECT:
			result = analyze_anyconnect_kernel_route(kernel_route_roots, kernel_roots);
			break;
		case OPENVPN:
			result = analyze_openvpn_kernel_route(kernel_route_roots, kernel_roots);
			break;
		case WIREGUARD:
		case VPN_METHODS_NUMBERS:
		case VPN_METHOD_UNKNOWN:
		default:
			printf("VPN method is not supported: %s\n", vpn_methods_string[vpn_method]);
			exit(1);
	};
}

typedef struct node_search
{
	char *ifa_name;
	GNode *krt_node;
} NodeSearch;

gboolean find_node_traverse(GNode *node, gpointer data)
{
	RouteNode *rn = ROUTENODE(node->data);
	NodeSearch *ns = (NodeSearch*)data;
	if (!strncmp(rn->if_name, ns->ifa_name, sizeof(ns->ifa_name)))
	{
		ns->krt_node = node;
		return TRUE;
	}
	return FALSE;
}

GNode* get_kernel_route_node(GNode *kernel_route_roots[MAX_ROOTS_NUMBER], int roots, char *ifa_name)
{
	NodeSearch ns_param;
	ns_param.ifa_name = ifa_name;
	ns_param.krt_node = NULL;

	for (int i = 0; i < roots; i++)
	{
		g_node_traverse(kernel_route_roots[i], G_LEVEL_ORDER, G_TRAVERSE_ALL, -1, find_node_traverse, &ns_param);
		if (ns_param.krt_node)	break;
	}
	return ns_param.krt_node;
}

VpnMethod *detect_vpn_method()
{
	int roots = 0;

	VpnMethod *vm = vpn_method_new();
	GNode *k_rr_e1[1];
	k_rr_e1[0] = g_node_new(vm);
	char *c[40] = {"", "-f", "details", "--family", "inet"};
	get_route_trees(ROUTE_QUERY_GET_FIRST, k_rr_e1, &roots, 5, c);

	//printf("%d\t%s\t%p\n", vm->vpn_method, vm->vpn_name, vm);

	return vm;
}
