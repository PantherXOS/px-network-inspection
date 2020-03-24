#include <ethtool-info.h> 
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 *  @brief the wrapper for sending SIOCETHTOOL request via ioctl.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	ctx	the request to be sent.
 * 	@param[out]	cmd	the result of the request.
 *	@pre	the fd must be opened.
 */
int send_ioctl(struct cmd_context *ctx, void *cmd)
{
	ctx->ifr.ifr_data = cmd;
	return ioctl(ctx->fd, SIOCETHTOOL, &ctx->ifr);
}

/**
 *  @brief dumps the retrieved driver information
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	info	the struct represents the driver information.
 *	@pre	the info must be retrieved beforehand.
 */
int dump_drvinfo(struct ethtool_drvinfo *info)
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

/**
 *  @brief gets driver information request form dump.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	ctx	the command context to be submitted to ioctl.
 * 	@param[out]	drvinfo	the resulting driver information.
 * 	@return	the successfulness of the request. 
 */
int do_gdrv(struct cmd_context *ctx, struct ethtool_drvinfo *drvinfo)
{
	int err;
	//struct ethtool_drvinfo drvinfo;
	if (ctx->argc != 0)
	{
		printf("bard args error\n");
		return -1;
	}
	drvinfo->cmd = ETHTOOL_GDRVINFO;
	err = send_ioctl(ctx, drvinfo);
	if (err < 0)
	{
		perror("Cannot get driver information");
		return 71;
	}
	//return dump_drvinfo(drvinfo);
	return 0;
}

/**
 *  @brief gets the bus and hardware related device information from the driver.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	ifa_name	the name of the network interface device.
 * 	@param[out]	drvinfo	the resulting driver information.
 * 	@return	the successfulness of the request. 
 */
int get_drv_info(char *ifa_name, struct ethtool_drvinfo *drvinfo)
{
	struct cmd_context ctx;
	ctx.devname = ifa_name;
	memset(&ctx.ifr, 0, sizeof(ctx.ifr));
	strcpy(ctx.ifr.ifr_name, ctx.devname);
	ctx.fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (ctx.fd < 0)
	{
		perror("Cannot get control socket");
		return 70;
	}
	ctx.argc = 0;
	ctx.argp = NULL;
	return do_gdrv(&ctx, drvinfo);
}
