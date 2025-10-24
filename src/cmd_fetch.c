#include "spy.h"

void cmd_fetch(const char* playlist_id, const char* filename)
{
	playlist_t p = api_get_playlist(playlist_id);

	for (int i = 0; i < p->count; i++)
		track_add_tag(&p->tracks[i], "new?");

	fs_write_playlist(p, filename);
	playlist_free(p);
}
