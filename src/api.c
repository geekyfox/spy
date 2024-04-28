#include <string.h>

#include "spy.h"

static void __make_url(char* url, const char* path)
{
	if (strncmp(path, "https://", 8) == 0)
		strcpy(url, path);
	else
		sprintf(url, "https://api.spotify.com/v1%s", path);
}

static void __submit(struct strbuff* ret, const char* method, const char* path,
		     const char* body)
{
	char url[10240], auth[10240];
	__make_url(url, path);
	sprintf(auth, "Authorization: Bearer %s", secrets_token());

	struct http_request req = {
		.method = method,
		.url = url,
		.payload = body,
		.payload_is_json = true,
		.auth = auth,
	};

	http_submit(ret, req);
}

static json_t __get(const char* path)
{
	struct strbuff buff;
	__submit(&buff, "GET", path, NULL);
	json_t ret = json_parse(&buff);
	free(buff.data);
	return ret;
}

json_t api_get_paginated(const char* path)
{
	json_t ret = json_array();
	char* url = strdup(path);

	while (url) {
		json_t resp = __get(url);
		free(url);

		json_t page = json_pop(resp, "items", NULL);
		json_merge(ret, page);
		json_free(page);

		json_t next = json_pop(resp, "next", NULL);
		json_free(resp);

		if (json_isnull(next))
			url = NULL;
		else
			url = json_uwstr(next);
		json_free(next);
	}

	return ret;
}
