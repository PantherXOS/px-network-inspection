#ifndef PUBLIC_IP_H_
#define PUBLIC_IP_H_

#include <stdlib.h>
#include <route-tree.h>

struct url_data
{
    size_t size;
    char* data;
};

//char *handle_url(char* url, char* if_name);
void get_public_ip(char if_names[16][MAX_PHYS_IFS], int if_number, char if_public_ips[40][MAX_PHYS_IFS]);

#endif
