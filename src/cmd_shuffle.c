#include <string.h>
#include <strings.h>

#include "spy.h"

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

int cmd_shuffle(char** args)
{
	if ((! args) || (! strcmp(*args, "--help"))) {
		fprintf(stderr, "Usage: spy shuffle <filename...>\n");
		return 1;
	}

	while (*args) {
		char* filename = *args;
		args++;

		playlist_t original = playlist_read(filename, 0);
		playlist_t shuffled = __shuffle_playlist(original);
		fs_write_playlist(shuffled, filename);

		playlist_free(original);
		playlist_free(shuffled);
	}

	return 0;
}
