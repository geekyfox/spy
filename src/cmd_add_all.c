#include "spy.h"

void cmd_add_all(const char* src_filename, const char* dst_filename)
{
	playlist_t src = playlist_read(src_filename, 0);
	playlist_t dst = playlist_read(dst_filename, 0);

	for (int i = 0; i < src->count; i++) {
		track_t src_track = &src->tracks[i];
		track_t dst_track = playlist_lookup(dst, src_track->id);

		if (! dst_track) {
			track_add_tag(src_track, "add!");
			playlist_add(dst, src_track);
		}
	}

	fs_write_playlist(dst, dst_filename);
	playlist_free(src);
	playlist_free(dst);
}
