#include "spy.h"

void cmd_clear(const char* filename)
{
	playlist_t playlist = playlist_read(filename, 0);

	int count = playlist->count;

	playlist->count = 0;
	fs_write_playlist(playlist, filename);

	playlist->count = count;
	playlist_free(playlist);
}
