#include "spy.h"
#include <string.h>

const char CMD_XOR_USAGE[] =
	"[--only-us] TARGET SOURCE"
	"\tcompares two playlists and make TARGET contain all tracks that are "
	"in either TARGET or SOURCE but not both.";

struct context {
	const char* target;
	const char* source;
	bool only_us;
	playlist_t us;
	playlist_t them;
	playlist_t res;
};

static void __digest_args(struct context*);
static void __init(struct context*);
static void __add_us(struct context*);
static void __add_them(struct context*);
static void __flush(struct context*);

void cmd_xor(void)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	__digest_args(&ctx);

	__init(&ctx);
	__add_us(&ctx);

	if (! ctx.only_us)
		__add_them(&ctx);

	__flush(&ctx);
}

static void __digest_args(struct context* ctx)
{
	const char* flags[] = {"only-us"};

	switch (args_flagx(flags, 1)) {
	case 0:
		ctx->only_us = true;
		break;
	};

	ctx->target = args_pop();
	ctx->source = args_poplast();
}

static void __init(struct context* ctx)
{
	ctx->us = playlist_read(ctx->target, 0);
	ctx->them = playlist_read(ctx->source, 0);
	ctx->res = playlist_init(ctx->us);
}

static void __add_us(struct context* ctx)
{
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->us)) {
		if (playlist_lookup(ctx->them, t->id))
			continue;

		if (! ctx->only_us)
			track_add_tag(t, "us");

		track_remove_tag(t, "them");
		playlist_add(ctx->res, t);
	}
}

static void __add_them(struct context* ctx)
{
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->them)) {
		if (playlist_lookup(ctx->us, t->id))
			continue;

		track_add_tag(t, "them");
		track_remove_tag(t, "us");
		playlist_add(ctx->res, t);
	}
}

static void __flush(struct context* ctx)
{
	fs_write_playlist(ctx->res, ctx->target);
	playlist_free(ctx->us);
	playlist_free(ctx->them);
	playlist_free(ctx->res);
}
