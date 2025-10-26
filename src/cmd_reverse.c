#include "spy.h"

const char CMD_REVERSE_USAGE[] = "FILENAME...";

static void __reverse(const char* filename);

void cmd_reverse(void)
{
	const char* filename = NULL;

	while (args_popnext(&filename))
		__reverse(filename);
}

static void __reverse(const char* filename)
{
	playlist_t playlist = playlist_read(filename, 0);

	int i = 0, j = playlist->count - 1;

	while (i < j) {
		struct track t = playlist->tracks[i];
		playlist->tracks[i] = playlist->tracks[j];
		playlist->tracks[j] = t;
		i++;
		j--;
	}

	fs_write_playlist(playlist, filename);
	playlist_free(playlist);
}
