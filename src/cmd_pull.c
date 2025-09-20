#include <string.h>

#include "spy.h"

enum mode {
	MODE_REMOTE_ORDER = 44001,
	MODE_LOCAL_ORDER = 44002,
};

struct context {
	const char* filename;
	enum mode mode;
	bool skip_gone;
	playlist_t combined;
	playlist_t local;
	playlist_t remote;
};

static void __pull(struct context*);
static void __gather_gone(struct context* ctx);
static void __gather_remote(struct context* ctx);
static void __gather_new(struct context* ctx);
static void __gather_local(struct context* ctx);

int cmd_pull(char** args)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	ctx.mode = MODE_REMOTE_ORDER;
	ctx.skip_gone = false;

	while (*args) {
		char* arg = *args;
		if (! strcmp(arg, "--keep-order")) {
			ctx.mode = MODE_LOCAL_ORDER;
		} else if (! strcmp(arg, "--skip-gone")) {
			ctx.skip_gone = true;
		} else {
			ctx.filename = arg;
			__pull(&ctx);
		}

		args++;
	}

	return 0;
}

void __pull(struct context* ctx)
{
	ctx->local = playlist_read(ctx->filename, VF_PLAYLIST_ID);
	ctx->remote = api_get_playlist(ctx->local->playlist_id);
	ctx->combined = playlist_init(ctx->local);

	switch (ctx->mode) {
	case MODE_REMOTE_ORDER:
		__gather_gone(ctx);
		__gather_remote(ctx);
		break;
	case MODE_LOCAL_ORDER:
		__gather_new(ctx);
		__gather_local(ctx);
		break;
	default:
		DIE("Unexpected pull_mode %d\n", ctx->mode);
	};

	fs_write_playlist(ctx->combined, ctx->filename);
	playlist_free(ctx->local);
	playlist_free(ctx->remote);
	playlist_free(ctx->combined);
}

static void __gather_gone(struct context* ctx)
{
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->local)) {
		if (playlist_lookup(ctx->remote, t->id))
			continue;

		if (! track_has_tag(t, "add!")) {
			if (ctx->skip_gone)
				continue;
			track_add_tag(t, "gone?");
		}

		playlist_add(ctx->combined, t);
	}
}

static void __gather_new(struct context* ctx)
{
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->remote)) {
		if (playlist_lookup(ctx->local, t->id))
			continue;
		track_add_tag(t, "new?");
		playlist_add(ctx->combined, t);
	}
}

static void __gather_remote(struct context* ctx)
{
	track_t remote = NULL;
	while (playlist_iterate(&remote, ctx->remote)) {
		track_t local = playlist_lookup(ctx->local, remote->id);
		if (local) {
			track_remove_tag(local, "add!");
			strarr_move(&remote->tags, &local->tags);
			track_clear(local);
		} else {
			track_add_tag(remote, "new?");
		}
		playlist_add(ctx->combined, remote);
	}
}

static void __gather_local(struct context* ctx)
{
	track_t local = NULL;
	while (playlist_iterate(&local, ctx->local)) {
		track_t remote = playlist_lookup(ctx->remote, local->id);
		if (remote) {
			track_remove_tag(local, "add!");
			strarr_move(&remote->tags, &local->tags);
			track_clear(local);
			playlist_add(ctx->combined, remote);
			continue;
		}

		if (ctx->skip_gone)
			continue;

		if (! track_has_tag(local, "add!"))
			track_add_tag(local, "gone?");
		playlist_add(ctx->combined, local);
	}
}
