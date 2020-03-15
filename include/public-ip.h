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
void get_public_ip(char if_names[MAX_PHYS_IFS][16], int if_number, char if_public_ips[MAX_PHYS_IFS][40]);

#endif
