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

#include <argp.h>
#include <stdbool.h>

#include <json-c/json.h>

const char *argp_program_version = "px-network-inspection";
const char *argp_program_bug_address = "<s.mahmood@pantherx.org>";

/* Program documentation. */
static char doc[] = "A utility to collect network routing details of local machine.";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
	{"format", 'f', "Format <JSON:CXX>", 0, "File Format which has to be one of JSON or CXX" },
	{"output", 'o', "File", 0, "Output to FILE instead of standard output" },
	{ 0 }
};

enum Format
{
	JSON,
	CXX
};

/* Used by main to communicate with parse_opt. */
struct arguments 
{
	enum Format format;
	char *output_file;
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key)
	{
		case 'f':
			if (!strcmp("JSON", arg))
				arguments->format = JSON;
			else if (!strcmp("CXX", arg))
				arguments->format = CXX;
			else
				argp_usage (state);
			break;
		case 'o':
				arguments->output_file = arg;
			break;

		case ARGP_KEY_ARG:
			break;
		case ARGP_KEY_END:
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc};

/* Network devices of a route, in order */
struct net_device
{
	uint32_t dev_pos;
	char dev_name[10];
	char dev_type[20];
	char dev_method[30];
	char dev_ip4[16];
	char dev_ip6[40];
	char dev_gateway[40];	// May be ipv6
	char dev_dns[40];
	char dev_active[10];
};

struct element
{
	struct element *forward;
	struct element *backward;
	struct net_device *net_dev;
};

static struct element *new_element(void)
{
	struct element *e;

	e = malloc(sizeof(struct element));
	if (e == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	return e;
}
/* End of network devices of a route */

// TODO: use all elemnts.
/* Root of each route */
static struct element *roots[5];

int check_wireless(const char* ifname, char* protocol)
{
	int sock = -1;
	struct iwreq pwrq;
	memset(&pwrq, 0, sizeof(pwrq));
	strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return 0;
	}

	if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1) {
		if (protocol) strncpy(protocol, pwrq.u.name, IFNAMSIZ);
		close(sock);
		return 1;
	}

	close(sock);
	return 0;
}

/* TODO: Add other devices too. */
void get_routes()
{
	// Read all NIC devices. Checks if a device is AF_PACKETS. Add it to the list.
	int root = 0;	
	struct element *prev = NULL;
	struct element *elem = NULL;

	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST];

	struct rtnl_link *link;
	struct nl_sock *sk;
	int err = 0;
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	   can free list later */

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		/* Display interface name and family (including symbolic
		   form of the latter for the common families) */

		printf("%-8s %s (%d)\n", ifa->ifa_name, (family == AF_PACKET) ? "AF_PACKET" : (family == AF_INET) ? "AF_INET" :
				(family == AF_INET6) ? "AF_INET6" : "???", family);

		// TODO: Do it for other interfaces other than AF_PACKET
		if (family == AF_PACKET && ifa->ifa_data != NULL)
		{
			struct rtnl_link_stats *stats = ifa->ifa_data;

			printf("\t\ttx_packets = %10u; rx_packets = %10u\n" "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
					stats->tx_packets, stats->rx_packets, stats->tx_bytes, stats->rx_bytes);

			sk = nl_socket_alloc();
			err = nl_connect(sk, NETLINK_ROUTE);
			if (err < 0) {
				nl_perror(err, "nl_connect");
				continue;
			}
			if ((err = rtnl_link_get_kernel(sk , 0, ifa->ifa_name, &link)) < 0)
			{
				nl_perror(err, "");
			}
			else
			{
				//TODO: trace to root. As now, there is only one element in each root
				if (rtnl_link_get_arptype(link) == 1)	// Means ethernet
				{
					struct net_device *new_device = (struct net_device*) malloc(sizeof(struct net_device));
					memcpy(new_device->dev_name, ifa->ifa_name, sizeof(ifa->ifa_name));
					memcpy(new_device->dev_type, "physical", sizeof("physical"));

					int fd;
					struct ifreq ifr;
					fd = socket(AF_INET, SOCK_DGRAM, 0);
					/* I want to get an IPv4 IP address: TODO: AF_INET6 */
					ifr.ifr_addr.sa_family = AF_INET;
					/* I want IP address attached to "eth0" */
					strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ-1);
					ioctl(fd, SIOCGIFADDR, &ifr);
					close(fd);

					char *ipv4 = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
					/* display result */
					//printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
					printf("%s\n", ipv4);

					if (rtnl_link_get_carrier(link))
						strcpy(new_device->dev_ip4, ipv4);
					else
						strcpy(new_device->dev_ip4, "");

					strcpy(new_device->dev_active, rtnl_link_get_carrier(link) ? "ACTIVE" : "NOACTIVE");

					new_device->dev_pos = 1;

					char protocol[IFNAMSIZ]  = {0};
					//TODO: check bluetooth as well.
					int is_wireless = check_wireless(ifa->ifa_name, protocol);
					strcpy(new_device->dev_method, is_wireless ? "wifi" : "lan");

					elem = new_element();
					elem->net_dev = new_device;
					roots[root++] = elem;
					insque(elem, prev);
					prev = elem;
				}

				printf("Found device: %s and type: %s arptype: %d carrier: %d\n", rtnl_link_get_name(link),
						rtnl_link_get_type(link), rtnl_link_get_arptype(link), rtnl_link_get_carrier(link));
			}
			nl_close(sk);
		}
	}

	freeifaddrs(ifaddr);
}

int main (int argc, char **argv)
{
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
	json_object *jobj = json_object_new_object();
	int root = 0;
	struct element *elem = roots[root];
	while (elem)
	{
		printf("******************net_dev: %s -- %s -- %s -- %s\n", elem->net_dev->dev_name, elem->net_dev->dev_type,
				elem->net_dev->dev_ip4, elem->net_dev->dev_active);
		json_object *jarray = json_object_new_array();
		json_object *dev = json_object_new_object();

		json_object *dev_pos = json_object_new_int(elem->net_dev->dev_pos);
		json_object_object_add(dev, "pos", dev_pos);

		json_object *dev_name = json_object_new_string(elem->net_dev->dev_name);
		json_object_object_add(dev, "adapter", dev_name);

		json_object *dev_method = json_object_new_string(elem->net_dev->dev_method);
		json_object_object_add(dev, "method", dev_method);

		json_object *dev_type = json_object_new_string(elem->net_dev->dev_type);
		json_object_object_add(dev, "type", dev_type);

		json_object *dev_ip4 = json_object_new_string(elem->net_dev->dev_ip4);
		json_object_object_add(dev, "ip4", dev_ip4);

		json_object *dev_ip6 = json_object_new_string(elem->net_dev->dev_ip6);
		json_object_object_add(dev, "ip6", dev_ip6);

		json_object *dev_dns = json_object_new_string(elem->net_dev->dev_dns);
		json_object_object_add(dev, "dns", dev_dns);

		json_object *dev_gateway = json_object_new_string(elem->net_dev->dev_gateway);
		json_object_object_add(dev, "gateway", dev_gateway);

		json_object *dev_active = json_object_new_string(elem->net_dev->dev_active);
		json_object_object_add(dev, "status", dev_active);

		json_object_array_add(jarray, dev);
		char root_str[20];
		sprintf(root_str, "%d",root);
		json_object_object_add(jobj, root_str, jarray);
		elem = roots[++root];
	}

	// Send it to output.

	/*Now printing the json object*/
	printf ("The json object created: %s\n",json_object_to_json_string(jobj));

	if (arguments.output_file)
	{
		FILE* output_file;
		output_file = fopen(arguments.output_file, "w");
		fprintf(output_file, "%s", json_object_to_json_string(jobj));
		fclose(output_file);
	}

	exit (0);
}
