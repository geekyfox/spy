#include "spy.h"

void cmd_filter(const char* tag, const char* filename)
{
	playlist_t input = playlist_read(filename, 0);
	playlist_t output = playlist_init(input);

	for (int i = 0; i < input->count; i++) {
		track_t track = &input->tracks[i];

		bool remove;
		if (tag[0] == '-')
			remove = track_has_tag(track, tag + 1);
		else
			remove = ! track_has_tag(track, tag);

		if (remove) {
			if (track_has_tag(track, "add!") ||
			    track_has_tag(track, "gone!"))
				continue;
			track_add_tag(track, "remove!");
		} else {
			track_remove_tag(track, "remove!");
		}

		playlist_add(output, track);
	}

	fs_write_playlist(output, filename);
	playlist_free(input);
	playlist_free(output);
}
