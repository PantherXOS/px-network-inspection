#ifndef ETHTOOL_INFO_H_
#define ETHTOOL_INFO_H_

#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ethtool.h>

/* Context for sub-commands */
struct cmd_context {
    const char *devname;    /* net device name */
    int fd;         /* socket suitable for ethtool ioctl */
    struct ifreq ifr;   /* ifreq suitable for ethtool ioctl */
    int argc;       /* number of arguments to the sub-command */
    char **argp;        /* arguments to the sub-command */
};

int send_ioctl(struct cmd_context *ctx, void *cmd);

int dump_drvinfo(struct ethtool_drvinfo *info);

int do_gdrv(struct cmd_context *ctx, struct ethtool_drvinfo *drvinfo);
int get_drv_info(char *ifa_name, struct ethtool_drvinfo *drvinfo);

#endif
