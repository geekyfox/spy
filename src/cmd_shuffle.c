#include <string.h>
#include <strings.h>

#include "spy.h"

const char CMD_SHUFFLE_USAGE[] = "FILENAME...";

static playlist_t __shuffle_playlist(struct playlist* p)
{
	struct strarr ids;
	bzero(&ids, sizeof(ids));

	for (int i = 0; i < p->count; i++)
		strarr_add(&ids, p->tracks[i].id);

	strarr_shuffle(&ids, 0);

	playlist_t ret = playlist_init(p);

	for (int i = 0; i < ids.count; i++) {
		track_t track = playlist_lookup(p, ids.data[i]);
		playlist_add(ret, track);
	}

	strarr_clear(&ids);

	return ret;
}

void cmd_shuffle(void)
{
	const char* filename = args_pop();

	do {
		playlist_t original = playlist_read(filename, 0);
		playlist_t shuffled = __shuffle_playlist(original);

		fs_write_playlist(shuffled, filename);

		playlist_free(original);
		playlist_free(shuffled);

		filename = args_popopt();
	} while (filename);
}
