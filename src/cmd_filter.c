#include <string.h>

#include "spy.h"

struct params {
	char* tag;
	bool invert;
	char* filename;
};

static bool __parse_args(struct params* p, char** args)
{
	if (! args[0])
		return false;

	if (! strcmp(args[0], "--help"))
		return false;

	if (! args[1])
		return false;

	if (args[2])
		return false;

	if (args[0][0] == '-') {
		p->tag = args[0] + 1;
		p->invert = true;
	} else {
		p->tag = args[0];
		p->invert = false;
	}

	p->filename = args[1];

	return true;
}

int cmd_filter(char** args)
{
	struct params p;

	if (! __parse_args(&p, args)) {
		fprintf(stderr,
			"Usage: spy filter { <tag> | -<tag> } <filename>\n");
		return 1;
	}

	playlist_t input = playlist_read(p.filename, 0);
	playlist_t output = playlist_init(input);

	bool remove_tag = p.invert;
	bool keep_tag = ! remove_tag;

	for (int i = 0; i < input->count; i++) {
		track_t track = &input->tracks[i];
		bool has_tag = track_has_tag(track, p.tag);
		bool keep_track = has_tag ? keep_tag : remove_tag;
		if (keep_track)
			playlist_add(output, track);
	}

	fs_write_playlist(output, p.filename);
	playlist_free(input);
	playlist_free(output);

	return 0;
}
