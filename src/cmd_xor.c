#include "spy.h"
#include <string.h>

struct context {
	int retcode;
	char* name_a;
	char* name_b;
	playlist_t us;
	playlist_t them;
	playlist_t res;
};

static void __digest_args(struct context*, char**);
static void __init(struct context*);
static void __add_us(struct context*);
static void __add_them(struct context*);
static void __flush(struct context*);

int cmd_xor(char** args)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	__digest_args(&ctx, args);
	__init(&ctx);
	__add_us(&ctx);
	__add_them(&ctx);
	__flush(&ctx);

	return 0;
}

static void __digest_args(struct context* ctx, char** args)
{
	if (! args[0])
		goto bad;

	if (! strcmp(args[0], "--help"))
		goto help;

	if (! args[1])
		goto bad;

	if (args[2])
		goto bad;

	ctx->name_a = args[0];
	ctx->name_b = args[1];

	return;

	static const char HELP[] =
		"Usage:\n"
		"    spy xor <file A> <file B>\n"
		"\n"
		"Compares two playlists and make <file A> contain all tracks\n"
		"that are in either <file A> or <file B> but not both.\n";

bad:
	fputs(HELP, stderr);
	exit(1);

help:
	fputs(HELP, stdout);
	exit(0);
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
