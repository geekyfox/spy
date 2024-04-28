#include "spy.h"

void cmd_list(void)
{
	json_t resp = api_get_paginated("/me/playlists");

	int count = json_len(resp);

	for (int i = 0; i < count; i++) {
		json_t entry = json_get(resp, i);
		const char* name = json_popstr(entry, "name", NULL);
		const char* id = json_popstr(entry, "id", NULL);
		printf("%s    %s\n", id, name);
	}

	json_free(resp);
}
