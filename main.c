#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <search.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/wireless.h>

#include <arg-info.h>
#include <ethtool-info.h>
#include <gnode-object.h>
#include <route-tree.h>
#include <app-profile.h>

#include <json-c/json.h>
#include <public-ip.h>

// TODO: use all elemnts.
/* Root of each route */
static GNode *route_roots[MAX_ROOTS_NUMBER];
static int roots = 0;
static GNode *kernel_route_roots[MAX_ROOTS_NUMBER];
static int kernel_roots = 0;
static int kernel_primary_root = 0;
// Name of physical interfaces
static char phy_if[MAX_PHYS_IFS][IFNAMSIZ];
static size_t phy_index;
static size_t primary_if_index;

static char tap_if[MAX_TAP_IFS][IFNAMSIZ];
static size_t tap_index;

static char tun_if[MAX_TUN_IFS][IFNAMSIZ];
static size_t tun_index;
static int vpn_reachable[MAX_TUN_IFS];

static VpnMethod *detected_vpn_method = NULL; 
static char profile_name[MAX_VPN_PROFILE_NAME];

/// @brief	The traverse mode used for traversing group of interfaces
enum IF_TRAVERSE_MODE
{
	/// Means physical interface
	PHY,
	/// Means tap-based interfaces
	TAP,
	/// Means tun-based interfaces
	TUN,
	/// Means public interfaces. Not used now.
	PUB
};

/// @brief	The interface mode used to retrieve public IP
enum IF_MODE
{
	/// The public IP is accessible via physical interface
	PHY_MODE,
	/// The public IP is accessible via tap interface
	TAP_MODE,
	/// The public IP is accessible via tun interface
	TUN_MODE,
	/// The public IP is accessibility is not determined
	IF_NONE
};

/**
 *  @brief Finds the primary interface.
 *
 *  @details
 *   The function finds the primary physical interface through which, the main traffic of the machine goes.
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO Support tap-based interfaces.
 *
 *	@pre	The gstatic global variable kernel_route_roots must be set.
 *	@note	It sets the static global variable primary_if_index that indicates the index of primary physical interface.
 */
void find_primary_if_index()
{
	char *primary_if_name = (ROUTENODE(kernel_route_roots[kernel_primary_root]->data))->if_name;
	for (int i = 0; i < phy_index; i++)
	{
		if (!strncmp(primary_if_name, phy_if[i], 16))
		{
			primary_if_index = i;
		}
	}
}

/**
 *  @brief The function get one interface information.
 *
 *  @details
 *   The function uses many different API and libraries to gather interfaces information.
 *
 *  @see	https://wiki.pantherx.org
 *
 *	@param[in]	ifname	The input network interface to be checked for wifi information.
 *	@param[out]	protocol	The running link-layer protocol of wifi (e.g IEEE 802.a).
 *	@param[out]	essid_name	The name of the access point that the wifi interface is connected to.
 *	@note	It uses kernel's wifi interface.
 */
int get_wifi_info(const char* ifname, char* protocol, char *essid_name)
{
	int sock = -1;
	struct iwreq pwrq;
	memset(&pwrq, 0, sizeof(pwrq));
	strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return 0;
	}

	if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1)
	{
		if (protocol) strncpy(protocol, pwrq.u.name, IFNAMSIZ);

		memset(&pwrq, 0, sizeof(pwrq));
		char essid[IW_ESSID_MAX_SIZE + 1];	/* ESSID */
		char pessid[IW_ESSID_MAX_SIZE + 1];	/* Pcmcia format */
		unsigned int i;
		unsigned int j;
		memset(essid, 0, sizeof(essid));
		/* Get ESSID */
		pwrq.u.essid.pointer = (caddr_t) essid;
		pwrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
		pwrq.u.essid.flags = 0;

		strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);
		if (ioctl(sock, SIOCGIWESSID, &pwrq) >= 0);
		{
			strncpy(essid_name, essid, sizeof(essid));
		}

		close(sock);
		return 1;
	}

	close(sock);
	return 0;
}

/**
 *  @brief The function get one interface information.
 *
 *  @details
 *   The function uses many different API and libraries to gather interfaces information.
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO Support tap-based interfaces.
 *
 *	@pre	The input must be provided. The kernel_route_roots must be set. The VPN type and profile must be detected.
 *	@param[in]	ifa	The input network interface.
 *	@param[in]	family	The protocol family of the interface (i.e inet or inet6).
 *	@param[in]	tr_mode	The type of interfaces to be traversed.
 *	@note	It uses kernel_route_roots and affects other static global variables and arrays.
 */
void get_if_info(struct ifaddrs *ifa, int family, enum IF_TRAVERSE_MODE tr_mode)
{
	int s, n;
	struct rtnl_link *link;
	struct nl_sock *sk;
	int err = 0;
	// TODO: Do it for other interfaces other than AF_PACKET
	// TODO: consider AF_INET6
	if ((family == AF_PACKET && ifa->ifa_data != NULL) || (family == AF_INET))
	{
		struct rtnl_link_stats *stats = ifa->ifa_data;

		sk = nl_socket_alloc();
		err = nl_connect(sk, NETLINK_ROUTE);
		if (err < 0) {
			nl_perror(err, "nl_connect");
			return;
		}
		if ((err = rtnl_link_get_kernel(sk , 0, ifa->ifa_name, &link)) < 0)
		{
			nl_perror(err, "");
		}
		else
		{
			if (rtnl_link_get_arptype(link) == 772)	// Means loopback interface
				return;

			//TODO: trace to root. As now, there is only one element in each root
			//printf("\t\t ****************\n\t%s\n", ifa->ifa_name);
			struct ethtool_drvinfo drvinfo;
			int drv_info_res = get_drv_info(ifa->ifa_name, &drvinfo);

			// TODO: handle the follwoing conditions.
			if (!drv_info_res)
			{
				if (!strncmp(drvinfo.bus_info, "tap", sizeof("tap"))) return;
				if (!strncmp(drvinfo.driver, "bridge", sizeof("bridge"))) return;
			}

			if (tr_mode == TUN && !(!strncmp(drvinfo.driver, "tun", sizeof("tun")) && !strncmp(drvinfo.bus_info, "tun", sizeof("tun")))) return;

			GNode *krt_node = get_kernel_route_node(kernel_route_roots, kernel_roots, ifa->ifa_name);

			NetDevice *new_device = net_device_new();
			new_device->is_set = TRUE;
			new_device->phy_index = phy_index;
			new_device->public_device = NULL;
			bzero(new_device->dev_name, sizeof(new_device->dev_name));
			memcpy(new_device->dev_name, ifa->ifa_name, sizeof(ifa->ifa_name));
			if (tr_mode == PHY)
			{
				memcpy(phy_if[phy_index++], ifa->ifa_name, sizeof(ifa->ifa_name));
				memcpy(new_device->dev_type, "physical", sizeof("physical"));
				char protocol[IFNAMSIZ]  = {0};
				//TODO: check bluetooth as well.
				char essid[IW_ESSID_MAX_SIZE + 1] = {'\0'};
				int is_wireless = get_wifi_info(ifa->ifa_name, protocol, essid);
				strcpy(new_device->dev_wifi_ap, is_wireless ? essid : "");
				strcpy(new_device->dev_method, is_wireless ? "WIFI" : "LAN");
			}
			else if (tr_mode == TUN)
			{
				memcpy(tun_if[tun_index++], ifa->ifa_name, sizeof(ifa->ifa_name));
				memcpy(new_device->dev_type, "virtual", sizeof("virtual"));
				memcpy(new_device->dev_vpn_profile, profile_name, MAX_VPN_PROFILE_NAME);
				strcpy(new_device->dev_method, detected_vpn_method->vpn_name);	// TODO: detects actual method: openvpn, cisco or wireguard ...
			}
			else
			{
				printf("Unsupported request\n");
				exit(1);
			}

			int fd;
			struct ifreq ifr;
			fd = socket(AF_INET, SOCK_DGRAM, 0);
			/* I want to get an IPv4 IP address: TODO: AF_INET6 */
			ifr.ifr_addr.sa_family = AF_INET;
			strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
			ioctl(fd, SIOCGIFADDR, &ifr);
			close(fd);

			strncpy(new_device->dev_dns, "", sizeof(""));
			strncpy(new_device->dev_ip6, "", sizeof(""));
			char *ipv4 = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

			if (rtnl_link_get_carrier(link))
				strcpy(new_device->dev_ip4, ipv4);
			else
				strcpy(new_device->dev_ip4, "");

			// TODO: calc the heigh and therefore the pos.
			new_device->dev_pos = krt_node ? g_node_depth(krt_node) : 1;

			if (tr_mode == PHY)
			{
				strcpy(new_device->dev_active, rtnl_link_get_carrier(link) ? "ACTIVE" : "NOACTIVE");

				if (krt_node)
				{
					RouteNode *rn = ROUTENODE(krt_node->data);
					strncpy(new_device->dev_gateway, rn->gateway_ipv4, sizeof(rn->gateway_ipv4));
				}
				else
				{
					strncpy(new_device->dev_gateway, "", sizeof(""));
				}

				GNode *new_node = g_node_new(new_device);
				route_roots[roots++] = new_node;
			}
			else if (tr_mode == TUN)
			{
				if (rtnl_link_get_carrier(link))
				{
					RouteNode *rn = ROUTENODE(krt_node->data);
					char cmd[120];
					bzero(cmd, 120);

					if (detected_vpn_method->vpn_method == OPENVPN)
						//sprintf(cmd, "fping  %s", rn->gateway_ipv4);
						sprintf(cmd, "ping -c 1  %s > /dev/null 2>&1", rn->gateway_ipv4);
					else
						//sprintf(cmd, "fping my-ip.pantherx.org");
						sprintf(cmd, "ping -c 1 my-ip.pantherx.org > /dev/null 2>&1");

					if (!system(cmd))
					{
						strcpy(new_device->dev_active, "ACTIVE");
						vpn_reachable[tun_index - 1] = 1;
					}
					else
					{
						strcpy(new_device->dev_active, "NOACTIVE");
						vpn_reachable[tun_index - 1] = 0;
					}
				}
				else
				{
					strcpy(new_device->dev_active, "NOACTIVE");
				}

				if (krt_node)
				{
					RouteNode *rn = ROUTENODE(g_node_get_root(krt_node)->data);	// TODO: find the parent.
					/* The gateway of the VPN interface is the destination of the parent node in the route table */
					strncpy(new_device->dev_gateway, rn->dst_ipv4, sizeof(rn->dst_ipv4));
				}
				else
				{
					strncpy(new_device->dev_gateway, "", sizeof(""));
				}

				// TODO: more levels. Have to travers from root to the current node. Previous nodes has t be created.
				GNode *parent = route_roots[primary_if_index];	// default parent is the primary
				new_device->phy_index = primary_if_index;

				GNode *new_node = g_node_new(new_device);
				g_node_append(parent, new_node);
			}
			else
			{
				printf("Unsupported request\n");
				exit(1);
			}

			//printf("Found device: %s and type: %s arptype: %d carrier: %d\n", rtnl_link_get_name(link),
			//		rtnl_link_get_type(link), rtnl_link_get_arptype(link), rtnl_link_get_carrier(link));
	}
}
}

/**
 *  @brief The function traverses interfaces.
 *
 *  @details
 *   The function traverses a group network interfaces according to their device type: PHY, TUN, TAP, and ....
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO Support tap-based interfaces.
 *
 *	@pre	The input must be provided.
 *	@param[in]	ifaddr	List of input network interfaces to be checked and traversed.
 *	@param[in]	tr_mode	The type of interfaces to be traversed.
 *	@note	It uses kernel_route_roots and affects other static global variables and arrays.
 */
void traverse_ifs(struct ifaddrs *ifaddr, enum IF_TRAVERSE_MODE tr_mode)
{
	int root = 0;

	struct ifaddrs *ifa;
	int family, s, n;

	struct rtnl_link *link;
	struct nl_sock *sk;
	int err = 0;
	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		if (tr_mode == PHY)
		{
			// TODO: Do it for other interfaces other than AF_PACKET
			if (family == AF_PACKET && ifa->ifa_data != NULL)	// Retrive physical ifs information. TODO: Retrive tap ifs information as well.
			{
				get_if_info(ifa, family, tr_mode);
			}
		}
		else if (tr_mode == TUN)
		{
			// TODO: consider AF_INET6
			if (family == AF_INET)	// Retrive tun ifs information.
			{
				get_if_info(ifa, family, tr_mode);
			}
		}
		else
		{
			printf("Wrong function call: only PHY or TUN");
			exit(1);
		}
	}
}

/**
 *  @brief The function retrieve all public IPs.
 *
 *  @details
 *   The main function gets publlic IPs based on the VPN model.
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO Support tap-based VPNs.
 *
 *	@pre	All routes must be extracted.
 */
void public_ip_retrieve()
{
	enum IF_MODE if_mode = IF_NONE;
	char if_public_ips[MAX_PHYS_IFS][40];
	if (tun_index == 0)
	{
		if_mode = PHY_MODE;
		get_public_ip(phy_if, phy_index, if_public_ips);
	}
	else	// TODO: handle presence of tap interfaces
	{
		if_mode = TUN_MODE;
		get_public_ip(tun_if, tun_index, if_public_ips);
	}
	//else
	//{
	//	printf("Invalid Interface for Public IP retrieval");
	//	exit(0);
	//}

	// TODO: fill according to the VPN status
	int loop_limit = if_mode == TUN_MODE ? tun_index : roots;
	typeof(&phy_if) ifs;
	//char *ifs[MAX_PHYS_IFS][16];
	//ifs = if_mode == TUN_MODE ? &tun_if : &phy_if;
	ifs = if_mode == TUN_MODE ? &tun_if : &phy_if;
	for (int i = 0; i < loop_limit; i++)
	{
		char *root_if_name;
		// Finding the root [hysical NIC that routes the request
		if (if_mode == TUN_MODE)
		{
			GNode *krt_node = get_kernel_route_node(kernel_route_roots, kernel_roots, (*ifs)[i]);
			GNode *krt_node_root = g_node_get_root(krt_node);
			//strncpy(root_if_name, (ROUTENODE(krt_node_root->data)->if_name), 16);
			root_if_name = (ROUTENODE(krt_node_root->data))->if_name;
		}
		else
			root_if_name = phy_if[i];

		for (int j = 0; j < roots; j++)
		{
			NetDevice *nd = NETDEVICE(route_roots[j]->data);
			nd->public_device = NULL;
			if (!strncmp(if_public_ips[i],"", sizeof(""))) break;
			if (strncmp(root_if_name, nd->dev_name, sizeof(nd->dev_name)))	continue;
			if (if_mode == TUN_MODE)
			{
				if (vpn_reachable[i] == 0) continue;
			}

			NetDevice *pnd = net_device_new();
			nd->public_device = pnd;
			pnd->is_set = TRUE;
			pnd->dev_pos = 0;
			bzero(pnd->dev_type, sizeof(pnd->dev_type));
			strncpy(pnd->dev_type, "display", sizeof("display"));
			bzero(pnd->dev_name, sizeof(pnd->dev_name));
			strncpy(pnd->dev_name, "PUBLIC", sizeof("PUBLIC"));
			bzero(pnd->dev_method, sizeof(pnd->dev_method));
			strncpy(pnd->dev_method, "NONE", sizeof("NONE"));
			bzero(pnd->dev_ip4, sizeof(pnd->dev_ip4));
			strncpy(pnd->dev_ip4, if_public_ips[i], sizeof(if_public_ips[i]));
			bzero(pnd->dev_ip6, sizeof(pnd->dev_ip6));	// TODO: check if it is an IPv6
			bzero(pnd->dev_gateway, sizeof(pnd->dev_gateway));	// TODO: check if it is an IPv6
			bzero(pnd->dev_dns, sizeof(pnd->dev_dns));	// TODO: check if it is an IPv6
			bzero(pnd->dev_active, sizeof(pnd->dev_active));
			strncpy(pnd->dev_active, "ACTIVE", sizeof("ACTIVE"));
			pnd->phy_index = nd->phy_index;
			pnd->public_device = NULL;
		}
	}
}

/**
 *  @brief The main function detects all routes to public network (Internet).
 *
 *  @details
 *   The main function traverse the route and interfaces trees to establish all routes.
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO Add complicated routes.
 *  	TODO Support tap-based VPNs.
 *
 *	@pre	It must be called after detect_vpn_method, get_vpn_profile_name, argp_parse, and analyze_kernel_route functions.
 *	@post	The detected routes have to be traversed to produce JSON output.
 */
void get_routes()
{
	// Read all NIC devices. Checks if a device is AF_PACKETS. Add it to the list.
	int root = 0;

	struct ifaddrs *ifaddr, *ifa;
	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	// PHASE1: Walk through physical NICs as well as TAPs.
	traverse_ifs(ifaddr, PHY);

	// Find primary phy_if to be used in virtual ifs. TODO: We have to find routes.
	find_primary_if_index();

	// TODO: we have to set it later based on route table
	// PHASE2: Walk though non-physical NICs and TUNs.
	traverse_ifs(ifaddr, TUN);

	// TODO: Merge it to the others if it is possible.
	// PHASE3: Walk to find the public IPs.
	public_ip_retrieve();

	freeifaddrs(ifaddr);
}

/**
 * 	@author	Sina Mahmoodi
 *  @brief The main function of the project.
 *
 *  @details
 *   The main get input arguments, uses the vpn detection, public IP detection, route parsing,
 *   and network interface to inspect the network.
 *
 *  @see   https://wiki.pantherx.org
 *
 * 	@remark	
 * 	 px-network-inspection provides JSON based output in both file and stdout.
 * 	 Use px-network-inspection --usage to see the command help.
 */
int main (int argc, char **argv)
{
	detected_vpn_method = detect_vpn_method();

	// Hard-coded filtering the supported VPNs.
	if ((detected_vpn_method->vpn_method != ANYCONNECT)
			&& (detected_vpn_method->vpn_method != NO_VPN_METHOD)
			&& (detected_vpn_method->vpn_method != OPENVPN)) return 0;

	bzero(profile_name, MAX_VPN_PROFILE_NAME);
	int profile_stat = get_vpn_profile_name(detected_vpn_method->vpn_method, profile_name);

	struct arguments arguments;

	/* Default values. */
	arguments.output_file = NULL;
	arguments.format = JSON;

	/* Parse our arguments; every option seen by parse_opt will
	   be reflected in arguments. */
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	kernel_primary_root = analyze_kernel_route(kernel_route_roots, &kernel_roots, detected_vpn_method->vpn_method);
//	for (int j = 0; j < kernel_roots; j++)
//	{
//		RouteNode *rn = ROUTENODE(kernel_route_roots[j]->data);
//		if (j == kernel_primary_root)
//		{
//			RouteNode *nhrn = ROUTENODE((g_node_first_child(kernel_route_roots[j]))->data);
//		}
//	}
	// Get objects from list.
	get_routes();
	//return 0;

	// Process list into a JSON array objects.
	int root = 0;
	for (GNode *father = route_roots[root]; father; father = route_roots[++root])
		g_node_traverse(father, G_LEVEL_ORDER, G_TRAVERSE_ALL, -1, traverse_json_func, NULL);	// TODO: user data.

	json_object *jobj = json_object_new_object();
	root = 0;
	//struct element *elem = roots[root];
	GNode *node = route_roots[root];
	while (node)
	{
		json_object *jarray = json_object_new_array();
		// TODO: handle other routes
		g_node_traverse(node, G_LEVEL_ORDER, G_TRAVERSE_ALL, -1, traverse_json_array_func, jarray);	// TODO: user data.
		char root_str[20];
		//sprintf(root_str, "%s", dev->phy_index == primary_if_index ? "primary" : "others");
		sprintf(root_str, "%s", root == primary_if_index ? "primary" : "others");
		json_object_object_add(jobj, root_str, jarray);
		node = route_roots[++root];
	}

	// Send it to output.

	/*Now printing the json object*/
	printf ("%s\n",json_object_to_json_string(jobj));

	if (arguments.output_file)
	{
		FILE* output_file;
		output_file = fopen(arguments.output_file, "w");
		fprintf(output_file, "%s", json_object_to_json_string(jobj));
		fclose(output_file);
	}


	exit (0);
}
