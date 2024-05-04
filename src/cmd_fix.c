#include <string.h>

#include "spy.h"

void __fix_duplicates(playlist_t p)
{
	track_t x = NULL;

	while (playlist_iterate(&x, p)) {
		track_t y = x;
		while (playlist_iterate(&y, p))
			if (strcmp(x->id, y->id) == 0) {
				free(y->id);
				y->id = NULL;
			}
	}

	playlist_pack(p);
}

void cmd_fix(const char* filename)
{
	playlist_t playlist = fs_read_playlist(filename);
	__fix_duplicates(playlist);
	validate_playlist(playlist, "after fixing", 0);
	fs_write_playlist(playlist, filename);
	playlist_free(playlist);
}
