#ifndef PUBLIC_IP_H_
#define PUBLIC_IP_H_

#include <stdlib.h>

struct url_data {
    size_t size;
    char* data;
};

char *handle_url(char* url);

#endif
