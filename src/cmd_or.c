#include <string.h>
#include <strings.h>

#include "spy.h"

const char CMD_OR_HELP[] =
	"spy or [ { --top | --bottom} ] <target filename> <source filename>";

enum mode {
	MODE_DEFAULT = 46001,
	MODE_TOP = 46002,
	MODE_BOTTOM = 46003,
};

struct context {
	char* this_name;
	char* other_name;
	enum mode mode;
	playlist_t this;
	playlist_t other;
	playlist_t result;
	struct strarr this_tids;
};

static void __parse_args(struct context* ctx, char** args);
static void __read(struct context* ctx);
static void __wipe(playlist_t dst, playlist_t src);
static void __push(struct context* ctx, playlist_t p);

int cmd_or(char** args)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	__parse_args(&ctx, args);
	__read(&ctx);

	switch (ctx.mode) {
	case MODE_DEFAULT:
		__push(&ctx, ctx.this);
		__push(&ctx, ctx.other);
		break;
	case MODE_TOP:
		__push(&ctx, ctx.other);
		__push(&ctx, ctx.this);
		break;
	case MODE_BOTTOM:
		__wipe(ctx.this, ctx.other);
		__push(&ctx, ctx.this);
		__push(&ctx, ctx.other);
		break;
	default:
		DIE("Invalid mode %d", ctx.mode);
	}

	fs_write_playlist(ctx.result, ctx.this_name);

	playlist_free(ctx.this);
	playlist_free(ctx.other);
	playlist_free(ctx.result);
	strarr_clear(&ctx.this_tids);

	return 0;
}

static void __parse_args(struct context* ctx, char** args)
{
	if (! args[0])
		goto need_help;

	if (! strcmp(args[0], "--help"))
		goto need_help;

	if (! args[1])
		goto need_help;

	if (! args[2]) {
		ctx->this_name = args[0];
		ctx->other_name = args[1];
		ctx->mode = MODE_DEFAULT;
		return;
	}

	if (args[3])
		goto need_help;

	if (! strcmp(args[0], "--top")) {
		ctx->this_name = args[1];
		ctx->other_name = args[2];
		ctx->mode = MODE_TOP;
		return;
	}

	if (! strcmp(args[0], "--bottom")) {
		ctx->this_name = args[1];
		ctx->other_name = args[2];
		ctx->mode = MODE_BOTTOM;
		return;
	}

need_help:
	fprintf(stderr, "Usage: %s\n", CMD_OR_HELP);
	exit(1);
}

static void __read(struct context* ctx)
{
	ctx->this = playlist_read(ctx->this_name, 0);
	ctx->other = playlist_read(ctx->other_name, 0);
	ctx->result = playlist_init(ctx->this);

	track_t tt = NULL;
	while (playlist_iterate(&tt, ctx->this)) {
		if (! track_has_tag(tt, "add!"))
			strarr_add(&ctx->this_tids, tt->id);
	}

	track_t ot = NULL;
	while (playlist_iterate(&ot, ctx->other)) {
		tt = playlist_lookup(ctx->this, ot->id);
		ot->remote_index = tt ? tt->remote_index : 0;
	}
}

static void __wipe(playlist_t dst, playlist_t src)
{
	track_t t = NULL;
	while (playlist_iterate(&t, dst)) {
		if (playlist_lookup(src, t->id))
			track_clear(t);
	}
}

static void __push(struct context* ctx, playlist_t p)
{
	track_t t = NULL;
	while (playlist_iterate(&t, p)) {
		if (playlist_lookup(ctx->result, t->id))
			continue;

		if (strarr_has(&ctx->this_tids, t->id))
			track_remove_tag(t, "add!");
		else
			track_add_tag(t, "add!");

		playlist_add(ctx->result, t);
	}
}
