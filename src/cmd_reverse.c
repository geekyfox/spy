#include "spy.h"

static void __reverse(const char* filename);

int cmd_reverse(char** args)
{
	if (! args[0]) {
		printf("Usage: spy reverse <filename.playlist ...>\n");
		return 1;
	}

	while (*args) {
		__reverse(*args);
		args++;
	}

	return 0;
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
