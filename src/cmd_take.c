#include <strings.h>

#include "spy.h"

struct context {
	int amount;
	char* filename;
	bool need_help;
};

static void __parse_args(struct context* ctx, char** args)
{
	if (! args[0]) {
		ctx->need_help = true;
		return;
	}

	if (! args[1]) {
		ctx->amount = -1;
		ctx->filename = args[0];
		return;
	}

	if (args[2]) {
		ctx->need_help = true;
	}

	ctx->amount = atoi(args[0]);
	ctx->filename = args[1];
}

int cmd_take(char** args)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	__parse_args(&ctx, args);
	if (ctx.need_help) {
		fprintf(stderr, "Usage: spy take [<amount>] <filename>\n");
		return 1;
	}

	playlist_t playlist = playlist_read(ctx.filename, 0);

	int count = playlist->count;

	if (ctx.amount > count)
		ctx.amount = count;

	if (ctx.amount < 0)
		ctx.amount = playlist_cutoff(playlist) + 1;

	playlist->count = ctx.amount;
	fs_write_playlist(playlist, ctx.filename);

	playlist->count = count;
	playlist_free(playlist);

	return 0;
}
