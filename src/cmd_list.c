#include "spy.h"

const char CMD_LIST_USAGE[] = "";

void cmd_list(void)
{
	jj_t resp = api_get_paginated("/me/playlists");

	int count = jj_len(resp, NULL);

	for (int i = 0; i < count; i++) {
		jj_t entry = jj_popl(resp, i, NULL);

		if (jj_isnull(entry))
			continue;

		jj_t tmp = jj_pop(entry, "name", NULL);
		char* name = jj_tostr(tmp, NULL);

		tmp = jj_pop(entry, "id", NULL);
		char* id = jj_tostr(tmp, NULL);

		printf("%s    %s\n", id, name);

		jj_free(entry);
		free(id);
		free(name);
	}

	jj_free(resp);
}
