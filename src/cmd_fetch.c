#include "spy.h"

const char CMD_FETCH_USAGE[] = "API_ID FILENAME";

void cmd_fetch(void)
{
	const char* api_id = args_pop();
	const char* filename = args_poplast();

	playlist_t p = api_get_playlist(api_id);

	for (int i = 0; i < p->count; i++)
		track_add_tag(&p->tracks[i], "new?");

	fs_write_playlist(p, filename);
	playlist_free(p);
}
