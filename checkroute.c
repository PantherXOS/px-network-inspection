#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>


// ETHTOOL internal.h includes

//#include <stdbool.h>
//#include <stdio.h>
//#include <stdint.h>
//#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
//#include <unistd.h>
//#include <endian.h>
#include <sys/ioctl.h>
#include <net/if.h>

// ETHTOOL ethtool.c includes

//#include <stddef.h>
//#include <errno.h>
//#include <sys/utsname.h>
//#include <limits.h>
//#include <ctype.h>
//#include <assert.h>
#include <sys/fcntl.h>
//#include <netinet/in.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

/* Context for sub-commands */
struct cmd_context {
    const char *devname;    /* net device name */
    int fd;         /* socket suitable for ethtool ioctl */
    struct ifreq ifr;   /* ifreq suitable for ethtool ioctl */
    int argc;       /* number of arguments to the sub-command */
    char **argp;        /* arguments to the sub-command */
};

int send_ioctl(struct cmd_context *ctx, void *cmd)
{
	ctx->ifr.ifr_data = cmd;
	return ioctl(ctx->fd, SIOCETHTOOL, &ctx->ifr);
}

static int dump_drvinfo(struct ethtool_drvinfo *info)
{
	fprintf(stdout,
		"driver: %s\n"
		"version: %s\n"
		"firmware-version: %s\n"
		"bus-info: %s\n"
		"supports-statistics: %s\n"
		"supports-test: %s\n"
		"supports-eeprom-access: %s\n"
		"supports-register-dump: %s\n"
		"supports-priv-flags: %s\n",
		info->driver,
		info->version,
		info->fw_version,
		info->bus_info,
		info->n_stats ? "yes" : "no",
		info->testinfo_len ? "yes" : "no",
		info->eedump_len ? "yes" : "no",
		info->regdump_len ? "yes" : "no",
		info->n_priv_flags ? "yes" : "no");
	return 0;
}

static int do_gdrv(struct cmd_context *ctx)
{
	int err;
	struct ethtool_drvinfo drvinfo;
	if (ctx->argc != 0)
	{
		printf("bard args error\n");
		exit(1);
	}
	drvinfo.cmd = ETHTOOL_GDRVINFO;
	err = send_ioctl(ctx, &drvinfo);
	if (err < 0) {
		perror("Cannot get driver information");
		return 71;
	}
	return dump_drvinfo(&drvinfo);
}

int main(int argc, char *argv[])
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST], dest[NI_MAXHOST];

	struct rtnl_link *link;
	struct nl_sock *sk;
	int err = 0;
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	   can free list later */

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		/* Display interface name and family (including symbolic
		   form of the latter for the common families) */

		printf("%-8s %s (%d)\n",
				ifa->ifa_name,
				(family == AF_PACKET) ? "AF_PACKET" :
				(family == AF_INET) ? "AF_INET" :
				(family == AF_INET6) ? "AF_INET6" : "???",
				family);

		/* For an AF_INET* interface address, display the address */

		if (family == AF_INET || family == AF_INET6) {
			s = getnameinfo(ifa->ifa_addr,
					(family == AF_INET) ? sizeof(struct sockaddr_in) :
					sizeof(struct sockaddr_in6),
					host, NI_MAXHOST,
					NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}

			s = getnameinfo(ifa->ifa_dstaddr,
					(family == AF_INET) ? sizeof(struct sockaddr_in) :
					sizeof(struct sockaddr_in6),
					dest, NI_MAXHOST,
					NULL, 0, NI_NUMERICHOST);
			printf("\t\taddress: <%s> \t\taddress: <%s>\n", host, dest);

		} else if (family == AF_PACKET && ifa->ifa_data != NULL) {
			struct rtnl_link_stats *stats = ifa->ifa_data;

			printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
					"\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
					stats->tx_packets, stats->rx_packets,
					stats->tx_bytes, stats->rx_bytes);
		}

		sk = nl_socket_alloc();
		err = nl_connect(sk, NETLINK_ROUTE);
		if (err < 0) {
			nl_perror(err, "nl_connect");
			continue;
		}
		if ((err = rtnl_link_get_kernel(sk , 0, ifa->ifa_name, &link)) < 0)
		{
			nl_perror(err, "");
			goto CLEANUP_SOCKET;
		}
		else
		{
			printf("Found device: %s and type: %s - arptype: %d\n", rtnl_link_get_name(link), rtnl_link_get_type(link), rtnl_link_get_arptype(link));
		}
CLEANUP_SOCKET:
		nl_close(sk);

		struct cmd_context ctx;
		ctx.devname = ifa->ifa_name;
		memset(&ctx.ifr, 0, sizeof(ctx.ifr));
		strcpy(ctx.ifr.ifr_name, ctx.devname);
		ctx.fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (ctx.fd < 0) {
			perror("Cannot get control socket");
			return 70;
		}
		ctx.argc = 0;
		ctx.argp = NULL;
		do_gdrv(&ctx);
	}

	freeifaddrs(ifaddr);
	exit(EXIT_SUCCESS);



//	struct rtnl_link *link;
//	struct nl_sock *sk;
//	int err = 0;
//
//
//	//link = rtnl_link_alloc();
//	//if (!link) {
//	//	nl_perror(err, "rtnl_link_alloc");
//	//	goto OUT;
//	//}
//	//rtnl_link_set_name(link, "wg0");
//	//rtnl_link_set_type(link, "wireguard");
//
//	sk = nl_socket_alloc();
//	err = nl_connect(sk, NETLINK_ROUTE);
//	if (err < 0) {
//		nl_perror(err, "nl_connect");
//		goto CLEANUP_LINK;
//	}
//	printf("1\n");
//	if ((err = rtnl_link_get_kernel(sk , 0, "eno1", &link)) < 0)
//	{
//		nl_perror(err, "");
//		goto CLEANUP_SOCKET;
//	}
//	else
//	{
//		printf("Found device\n");
//	}
//	//err = rtnl_link_add(sk, link, NLM_F_CREATE);
//	//if (err < 0) {
//	//	nl_perror(err, "");
//	//	goto CLEANUP_SOCKET;
//	//}
//
//CLEANUP_SOCKET:
//	nl_close(sk);
//CLEANUP_LINK:
//	//rtnl_link_put(link);
//OUT:
//	return err;
}
