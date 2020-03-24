#ifndef APP_PROFILE_H_
#define APP_PROFILE_H_

#include <route-tree.h>

/**
 *  @brief Finds the VPN's profile name.
 *
 *  @details
 *   The function finds the profile name of the VPN if there is any.
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO Support tap-based interfaces.
 *
 * 	@param[in]	vpn_method	the method used for VPN i.e. OPENVPN.
 * 	@param[out]	profile_name	the found name of the profile if there is any.
 * 	@return	indicates that the profile_name contains a valid name. The value -1 means unsupported VPN method.
 * 			The value 0 means a default or no profile name. The value 1 means profile_name contains a valid value.
 *	@pre	The vpn_method must be retrieved first.
 */
int get_vpn_profile_name(enum VPN_METHODS vpn_method, char profile_name[MAX_VPN_NAME]);

#endif
