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
#include <string.h>
#include <linux/wireless.h>

#include <arg-info.h>
#include <routing-info.h>
#include <ethtool-info.h>
#include <gnode-object.h>
#include <route-tree.h>

#include <json-c/json.h>

// TODO: use all elemnts.
/* Root of each route */
static GNode *route_roots[MAX_ROOTS_NUMBER];	// TODO: avoid fix numbers.
static int root = 0;
// Name of physical interfaces
static char phy_if[MAX_PHYS_IFS][16];	// TODO: avoid fix numbers.
static size_t phy_index;
static size_t primary_if_index;

static char tap_if[MAX_TAP_IFS][16]; 	//TODO: avoid fix numbers.
static size_t tap_index;

static char tun_if[MAX_TUN_IFS][16]; 	//TODO: avoid fix numbers.
static size_t tun_index;

void find_primary_if_index()
{
	int nl_sock = open_netlink();

	if (do_route_dump_requst(nl_sock) < 0)
	{
		perror("Failed to perfom request");
		close(nl_sock);
	}

	//get_route_dump_response(nl_sock);

	primary_if_index = get_route_dump_check_primary(nl_sock, phy_if, phy_index);

	close (nl_sock);
}

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

enum IF_TRAVERSE_MODE
{
	PHY,
	TAP,
	TUN,
	PUB
};

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
			//if (rtnl_link_get_arptype(link) == 1)	// Means ethernet
			//{
				//printf("\t\t ****************\n\t%s\n", ifa->ifa_name);
				struct ethtool_drvinfo drvinfo;
				int drv_info_res = get_drv_info(ifa->ifa_name, &drvinfo);
				//if (!drv_info_res)
				//	dump_drvinfo(&drvinfo);
				//else
				//	printf("cannot get device info <%s>\n", ifa->ifa_name);

				// TODO: handle the follwoing conditions.
				if (!drv_info_res)
				{
					if (!strncmp(drvinfo.bus_info, "tap", sizeof("tap"))) return;
					if (!strncmp(drvinfo.driver, "bridge", sizeof("bridge"))) return;
				}

				if (tr_mode == TUN && !(!strncmp(drvinfo.driver, "tun", sizeof("tun")) && !strncmp(drvinfo.bus_info, "tun", sizeof("tun")))) return;

				NetDevice *new_device = net_device_new();
				new_device->is_set = TRUE;
				//struct net_device *new_device = (struct net_device*) malloc(sizeof(struct net_device));
				new_device->phy_index = phy_index;
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
					strcpy(new_device->dev_method, is_wireless ? "wifi" : "lan");
				}
				else if (tr_mode == TUN)
				{
					memcpy(tun_if[tun_index++], ifa->ifa_name, sizeof(ifa->ifa_name));
					memcpy(new_device->dev_type, "virtual", sizeof("virtual"));
					strcpy(new_device->dev_method, "vpn");	// TODO: detects actual method: openvpn, cisco or wireguard ...
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
				strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ-1);
				ioctl(fd, SIOCGIFADDR, &ifr);
				close(fd);

				strncpy(new_device->dev_dns, "", sizeof(""));
				char *ipv4 = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

				if (rtnl_link_get_carrier(link))
					strcpy(new_device->dev_ip4, ipv4);
				else
					strcpy(new_device->dev_ip4, "");

				strcpy(new_device->dev_active, rtnl_link_get_carrier(link) ? "ACTIVE" : "NOACTIVE");

				new_device->dev_pos = 1;

				if (tr_mode == PHY)
				{
					GNode *new_node = g_node_new(new_device);
					route_roots[root++] = new_node;
				}
				else if (tr_mode == TUN)
				{
					// TODO: Fill an enqueue according to route table. For now, we assume that all connections go though primary.
					// For now, we do not have VPN or VPN.
					// Detect parent and put it in parent variable.
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
			//}

			//printf("Found device: %s and type: %s arptype: %d carrier: %d\n", rtnl_link_get_name(link),
			//		rtnl_link_get_type(link), rtnl_link_get_arptype(link), rtnl_link_get_carrier(link));
		}
	}
}

void traverse_ifs(struct ifaddrs *ifaddr, enum IF_TRAVERSE_MODE tr_mode)
{
	int root = 0;

	struct ifaddrs *ifa;
	int family, s, n;
	//char host[NI_MAXHOST];

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

/* TODO: Add other devices too. */
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

	freeifaddrs(ifaddr);
}

int main (int argc, char **argv)
{
	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	//char *a[40] = {"px-network-inspection", "--format=details"};
	
	//char *a[40] = {"", "-f", "details", "--family", "inet", "--scope", "link", "--table", "main", "-d", "default"};
	//get_route_trees(11, a, NULL);

	//char *c[40] = {"", "-f", "details", "--family", "inet", "--scope", "global"};
	//get_route_trees(7, c, NULL);
	//return 0;
	// **************************************************************************************************************

	struct arguments arguments;

	/* Default values. */
	arguments.output_file = NULL;
	arguments.format = JSON;

	/* Parse our arguments; every option seen by parse_opt will
	   be reflected in arguments. */
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	// Get objects from list.
	get_routes();

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
		NetDevice *dev = NETDEVICE(node->data);
		json_object_array_add(jarray, dev->jobj);
		// TODO: handle other routes
		if (root == primary_if_index)
		{
			g_node_traverse(node, G_LEVEL_ORDER, G_TRAVERSE_ALL, -1, traverse_json_array_func, jarray);	// TODO: user data.
		}
		char root_str[20];
		sprintf(root_str, "%s", dev->phy_index == primary_if_index ? "primary" : "others");
		json_object_object_add(jobj, root_str, jarray);
		//elem = roots[++root];
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
