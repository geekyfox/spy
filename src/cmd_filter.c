#include <string.h>

#include "spy.h"

const char CMD_FILTER_USAGE[] = "{ TAG | -TAG } FILENAME";

struct args {
	const char* tag;
	bool invert;
	const char* filename;
};

static struct args __parse_args(void)
{
	struct args args;

	const char* tag = args_pop();

	if (tag[0] == '-') {
		args.tag = tag + 1;
		args.invert = true;
	} else {
		args.tag = tag;
		args.invert = false;
	}

	args.filename = args_poplast();

	return args;
}

void cmd_filter(void)
{
	struct args args = __parse_args();

	playlist_t input = playlist_read(args.filename, 0);
	playlist_t output = playlist_init(input);

	bool remove_tag = args.invert;
	bool keep_tag = ! remove_tag;

	for (int i = 0; i < input->count; i++) {
		track_t track = &input->tracks[i];
		bool has_tag = track_has_tag(track, args.tag);
		bool keep_track = has_tag ? keep_tag : remove_tag;
		if (keep_track)
			playlist_add(output, track);
	}

	fs_write_playlist(output, args.filename);
	playlist_free(input);
	playlist_free(output);
}
