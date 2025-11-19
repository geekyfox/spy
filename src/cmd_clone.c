#include "spy.h"

#include <string.h>
#include <strings.h>

const char CMD_CLONE_USAGE[] = "FILENAME";

static void __patch(playlist_t playlist, const char* playlist_id);

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

	__patch(playlist, playlist_id);
	fs_write_playlist(playlist, filename);

	free(playlist_id);
	strarr_clear(&tids);
	playlist_free(playlist);
}

static void __patch(playlist_t playlist, const char* playlist_id)
{
	struct strarr* header = &playlist->header;
	int idx = -1;
	for (int i = 0; i < header->count; i++) {
		if (! strncmp(header->data[i], "playlist_id ", 12)) {
			idx = i;
			break;
		}
	}

	char tmp[10240];
	sprintf(tmp, "playlist_id %s", playlist_id);

	if (idx < 0) {
		idx = header->count;
		strarr_add(header, tmp);
	} else {
		free(header->data[idx]);
		header->data[idx] = strdup(tmp);
	}

	if (idx != 0)
		strarr_shift(header, idx, 0);
}
