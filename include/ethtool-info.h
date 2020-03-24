#ifndef ETHTOOL_INFO_H_
#define ETHTOOL_INFO_H_

#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ethtool.h>

/// @brief	The struct represents the request message for ioctl system call.
struct cmd_context
{
	///	@brief	the queried device name
    const char *devname;
	/// @brief	the handling file descriptor
    int fd;
	///	@brief	the request suitable for ethtool ioctl
    struct ifreq ifr;
	/// @brief	the number of arguments to the sub-command
    int argc;
	///	@brief	arguments to the sub-command
    char **argp;
};

//int send_ioctl(struct cmd_context *ctx, void *cmd);

/**
 *  @brief dumps the retrieved driver information
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	info	the struct represents the driver information.
 *	@pre	the info must be retrieved beforehand.
 */
int dump_drvinfo(struct ethtool_drvinfo *info);

//int do_gdrv(struct cmd_context *ctx, struct ethtool_drvinfo *drvinfo);

/**
 *  @brief gets the bus and hardware related device information from the driver.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[in]	ifa_name	the name of the network interface device.
 * 	@param[out]	drvinfo	the resulting driver information.
 * 	@return	the successfulness of the request. 
 */
int get_drv_info(char *ifa_name, struct ethtool_drvinfo *drvinfo);

#endif
