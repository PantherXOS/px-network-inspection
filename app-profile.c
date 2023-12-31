#include <app-profile.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

/**
 *  @brief Eliminates the last extension from the profile address.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@param[out]	mystr	The address of the profile.
 * 	@return	indicates	The profile name.
 */
char *remove_ext(char* mystr)
{
    char *retstr;
    char *lastext;
    if (mystr == NULL) return NULL;
    if ((retstr = malloc (strlen (mystr) + 1)) == NULL) return NULL;
    strcpy (retstr, mystr);
    lastext = strrchr (retstr, '.');
    if (lastext != NULL)
        *lastext = '\0';
    return retstr;
}

/**
 *  @brief Finds the OpenVPN profile name.
 *
 *  @details
 *   The function finds the profile name of VPN used OpenVPN. It searches for the process cmd_line in /proc.
 *
 *  @see	https://wiki.pantherx.org
 *  @todo
 *  	TODO Support tap-based interfaces.
 *
 * 	@param[out]	profile_name	the found name of the profile if there is any.
 * 	@return	indicates that the profile_name contains a valid name. The value -1 means unsupported VPN method.
 * 			The value 0 means a default or no profile name. The value 1 means profile_name contains a valid value.
 */
int get_openvpn_profile_name(char profile_name[MAX_VPN_PROFILE_NAME])
{
	const char* directory = "/proc";
	size_t      task_name_size = 1024;
	char*       task_name = calloc(1, task_name_size);
	char*       option;
	char*       pn;

	DIR* dir = opendir(directory);

	if (dir)
	{
		struct dirent* de = 0;

		while ((de = readdir(dir)) != 0)
		{
			if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
				continue;

			int pid = -1;
			int res = sscanf(de->d_name, "%d", &pid);

			if (res == 1)
			{
				// we have a valid pid

				// open the cmdline file to determine what's the name of the process running
				char cmdline_file[1024] = {0};
				sprintf(cmdline_file, "%s/%d/cmdline", directory, pid);

				FILE* cmdline = fopen(cmdline_file, "r");

				// TODO: Better parameter parsing
				int n = 0;
				if ((n = getline(&task_name, &task_name_size, cmdline)) > 0)
				{
					if (!strncmp(task_name, "openvpn", sizeof("openvpn")))
					{
						size_t ts = strlen(task_name);
						option = &task_name[ts + 1];
						size_t os = strlen(option);
						if (!strncmp(option, "--config", sizeof("--config")))
						{
							if (n > (ts + os + 2))
							{
								pn = &task_name[ts + os + 2];
								char *pfn = basename(pn);
								char* apfn = remove_ext(pfn);
								size_t apfns = strlen(apfn);
								strncpy(profile_name, apfn, apfns + 1);
								closedir(dir);
								free(task_name);
								return 1;
							}
							else
							{
								bzero(profile_name, MAX_VPN_PROFILE_NAME);
								closedir(dir);
								free(task_name);
								return 0;
							}
						}
						else
						{
							if (n == (ts + os +2))
							{
								char *pfn = basename(option);
								char* apfn = remove_ext(pfn);
								size_t apfns = strlen(apfn);
								strncpy(profile_name, apfn, apfns + 1);
								closedir(dir);
								free(task_name);
								return 1;
							}
							else
							{
								bzero(profile_name, MAX_VPN_PROFILE_NAME);
								closedir(dir);
								free(task_name);
								return 0;
							}
						}
					}
				}
				fclose(cmdline);
			}
		}
	}

	bzero(profile_name, MAX_VPN_PROFILE_NAME);
	closedir(dir);
	free(task_name);
	return 0;
}

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
int get_vpn_profile_name(enum VPN_METHODS vpn_method, char profile_name[MAX_VPN_PROFILE_NAME])
{
	if (vpn_method == OPENVPN)
	{
		return get_openvpn_profile_name(profile_name);
	}
	else if (vpn_method == WIREGUARD)
	{
		return -1;	// Unsupported
	}
	else if (vpn_method == ANYCONNECT)
	{
		bzero(profile_name, MAX_VPN_PROFILE_NAME);
		strncpy(profile_name, "anyconnect", sizeof("anyconnect"));
		return 1;	// No profile
	}
	else
	{
		return 0;
	}
}
