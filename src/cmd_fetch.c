#include "spy.h"

void cmd_fetch(const char* playlist_id, const char* filename)
{
	playlist_t p = api_get_playlist(playlist_id);
	fs_write_playlist(p, filename);
	playlist_free(p);
}
