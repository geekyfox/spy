#include "spy.h"

void cmd_sync_tags(const char* src_filename, const char* dst_filename)
{
	playlist_t src = playlist_read(src_filename, 0);
	playlist_t dst = playlist_read(dst_filename, 0);

	for (int i = 0; i < src->count; i++) {
		track_t src_track = &src->tracks[i];
		track_t dst_track = playlist_lookup(dst, src_track->id);
		if (dst_track)
			strarr_move(&dst_track->tags, &src_track->tags);
	}

	fs_write_playlist(dst, dst_filename);

	playlist_free(src);
	playlist_free(dst);
}
