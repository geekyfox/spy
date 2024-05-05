#include "spy.h"

void cmd_list(void)
{
	json_t resp = api_get_paginated("/me/playlists");

	int count = jsarr_len(resp);

	for (int i = 0; i < count; i++) {
		json_t entry = jsarr_get(resp, i);
		char* name = jsobj_getstr(entry, "name", NULL);
		char* id = jsobj_getstr(entry, "id", NULL);
		printf("%s    %s\n", id, name);
	}

	json_free(resp);
}
