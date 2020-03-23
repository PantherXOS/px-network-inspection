#ifndef APP_PROFILE_H_
#define APP_PROFILE_H_

#include <route-tree.h>

// Return value indicates the profile name type
int get_vpn_profile_name(enum VPN_METHODS vpn_methods, char profile_name[MAX_VPN_NAME]);

#endif
