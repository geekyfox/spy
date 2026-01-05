#include "spy.h"

#include <string.h>
#include <strings.h>

const char CMD_CLONE_USAGE[] = "FILENAME";

void cmd_clone(void)
{
	const char* filename = args_poplast();
	playlist_t playlist = playlist_read(filename, 0);
	char* playlist_id = api_create_playlist(filename);

	struct strarr tids;
	bzero(&tids, sizeof(tids));

	track_t t = NULL;
	while (playlist_iterate(&t, playlist))
		strarr_add(&tids, t->id);

	if (tids.count)
		api_add_tracks(playlist_id, &tids);

	printf("Done: https://open.spotify.com/playlist/%s\n", playlist_id);

	free(playlist->playlist_id);
	playlist->playlist_id = playlist_id;
	fs_write_playlist(playlist, filename);

	strarr_clear(&tids);
	playlist_free(playlist);
}
