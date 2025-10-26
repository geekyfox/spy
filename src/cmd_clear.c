#include "spy.h"

const char CMD_CLEAR_USAGE[] = "FILENAME...";

void cmd_clear(void)
{
	const char* filename = NULL;

	while (args_popnext(&filename)) {
		playlist_t playlist = playlist_read(filename, 0);

		int count = playlist->count;
		playlist->count = 0;
		fs_write_playlist(playlist, filename);

		playlist->count = count;
		playlist_free(playlist);
	}
}
