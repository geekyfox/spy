#include <strings.h>

#include "spy.h"

const char CMD_DROP_USAGE[] =
	"[N] FILENAME"
	"\twhen N is provided, removes the first N tracks."
	"\twhen N is not provided, removes tracks until (and including) "
	"the first track tagged `cutoff!`."
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

void cmd_drop(void)
{
	struct args args = __parse_args();

	playlist_t playlist = playlist_read(args.filename, 0);

	int count = playlist->count;

	if (args.amount > count)
		args.amount = count;

	if (args.amount < 0)
		args.amount = playlist_cutoff(playlist) + 1;

	if (args.amount > 0) {
		playlist->tracks += args.amount;
		playlist->count -= args.amount;

		fs_write_playlist(playlist, args.filename);

		playlist->tracks -= args.amount;
		playlist->count += args.amount;
	}

	playlist_free(playlist);
}
