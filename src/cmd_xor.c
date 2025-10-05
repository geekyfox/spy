#include "spy.h"
#include <string.h>

static const char HELP[] =
	"Usage:\n"
	"    spy xor <file A> <file B>\n"
	"\n"
	"Compares two playlists and make <file A> contain all tracks\n"
	"that are in either <file A> or <file B> but not both.\n";

struct context {
	char* name_a;
	char* name_b;
	bool only_us;
	playlist_t us;
	playlist_t them;
	playlist_t res;
};

static int __digest_args(struct context*, char**);
static void __show_help(int retcode);
static void __init(struct context*);
static void __add_us(struct context*);
static void __add_them(struct context*);
static void __flush(struct context*);

int cmd_xor(char** args)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	int retcode = __digest_args(&ctx, args);
	if (retcode >= 0)
		__show_help(retcode);

	__init(&ctx);
	__add_us(&ctx);

	if (! ctx.only_us)
		__add_them(&ctx);

	__flush(&ctx);

	return 0;
}

static int __digest_args(struct context* ctx, char** args)
{
	if (! *args)
		return 1;

	if (! strcmp(*args, "--help"))
		return 0;

	if (! strcmp(*args, "--only-us")) {
		ctx->only_us = true;
		args++;
	}

	if (! *args)
		return 1;

	ctx->name_a = *args;
	args++;

	if (! *args)
		return 1;

	ctx->name_b = *args;
	args++;

	if (*args)
		return 1;

	return -1;
}

static void __show_help(int retcode)
{
	if (retcode > 0)
		fputs(HELP, stderr);
	else
		fputs(HELP, stdout);

	exit(retcode);
}

static void __init(struct context* ctx)
{
	ctx->us = playlist_read(ctx->name_a, 0);
	ctx->them = playlist_read(ctx->name_b, 0);
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
	fs_write_playlist(ctx->res, ctx->name_a);
	playlist_free(ctx->us);
	playlist_free(ctx->them);
	playlist_free(ctx->res);
}
