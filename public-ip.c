#include <public-ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data)
{
	size_t index = data->size;
	size_t n = (size * nmemb);
	char* tmp;

	data->size += (size * nmemb);

	tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

	if(tmp) {
		data->data = tmp;
	} else {
		if(data->data) {
			free(data->data);
		}
		fprintf(stderr, "Failed to allocate memory.\n");
		return 0;
	}

	memcpy((data->data + index), ptr, n);
	data->data[data->size] = '\0';

	return size * nmemb;
}

struct url_data* handle_url(char* url, char *if_name)
{
	CURL *curl;

	struct url_data *data = (struct url_data*)malloc(sizeof(struct url_data));
	data->size = 0;
	data->data = malloc(4096); /* reasonable size initial buffer */
	if(NULL == data->data) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return NULL;
	}

	data->data[0] = '\0';

	CURLcode res;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_INTERFACE, if_name);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
		res = curl_easy_perform(curl);
		//if(res != CURLE_OK) {
		//	fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		//}

		curl_easy_cleanup(curl);

	}
	return data;
}

void get_public_ip(char if_names[MAX_PHYS_IFS][IFNAMSIZ], int if_number, char if_public_ips[MAX_PHYS_IFS][40])
{
	for (int i = 0; i < if_number; i++)
	{
		struct url_data *data = handle_url("https://my-ip.pantherx.org/ip", if_names[i]);

		bzero(if_public_ips[i], sizeof(if_public_ips[i]));
		if(data->size)
		{
			strncpy(if_public_ips[i], data->data, data->size - 1);
		}
		else
		{
			strncpy(if_public_ips[i], "", sizeof(""));
		}
	}
}
