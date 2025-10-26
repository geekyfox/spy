#include <strings.h>

#include "spy.h"

const char CMD_TAKE_USAGE[] =
	"[N] FILENAME"
	"\twhen N is provided, keeps the first N tracks and removes the rest."
	"\twhen N is not provided, keeps tracks until (and including) the "
	"first track tagged `cutoff!`, and removes the rest."
	"\twhen N is not provided and no track is tagged `cutoff!` does "
	"nothing.";

struct args {
	const char* filename;
	int amount;
};

static struct args __parse_args(void)
{
	struct args args;
	const char* first = args_pop();
	const char* second = args_popopt();

	if (second) {
		args_finish();
		args.filename = second;
		args.amount = args_atoi(first);
	} else {
		args.filename = first;
		args.amount = -1;
	}

	return args;
}

void cmd_take(void)
{
	struct args args = __parse_args();

	playlist_t playlist = playlist_read(args.filename, 0);

	int count = playlist->count;

	if (args.amount < 0) {
		int cutoff = playlist_cutoff(playlist);
		args.amount = (cutoff < 0) ? count : cutoff + 1;
	}

	if (args.amount > count)
		args.amount = count;

	if (args.amount != count) {
		playlist->count = args.amount;
		fs_write_playlist(playlist, args.filename);
		playlist->count = count;
	}

	playlist_free(playlist);
}
