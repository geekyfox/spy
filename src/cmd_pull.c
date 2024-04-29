#include <string.h>

#include "spy.h"

struct context {
	playlist_t combined;
	playlist_t local;
	playlist_t remote;
};

static void __gather_gone(struct context* ctx);
static void __gather_remote(struct context* ctx);
static void __gather_new(struct context* ctx);
static void __gather_local(struct context* ctx);

void cmd_pull(const char* filename, enum pull_mode mode)
{
	struct context ctx;
	ctx.local = playlist_read(filename, VF_PLAYLIST_ID);
	ctx.remote = api_get_playlist(ctx.local->playlist_id);
	ctx.combined = playlist_init(ctx.local);

	switch (mode) {
	case PULL_MODE_REMOTE_ORDER:
		__gather_gone(&ctx);
		__gather_remote(&ctx);
		break;
	case PULL_MODE_LOCAL_ORDER:
		__gather_new(&ctx);
		__gather_local(&ctx);
		break;
	};

	fs_write_playlist(ctx.combined, filename);
	playlist_free(ctx.local);
	playlist_free(ctx.remote);
	playlist_free(ctx.combined);
}

static void __gather_gone(struct context* ctx)
{
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->local)) {
		if (playlist_lookup(ctx->remote, t->id))
			continue;
		if (! track_has_tag(t, "add!"))
			track_add_tag(t, "gone?");
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
		} else {
			track_add_tag(local, "gone?");
			playlist_add(ctx->combined, local);
		}
	}
}
