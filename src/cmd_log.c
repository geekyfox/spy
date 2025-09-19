#include <strings.h>

#include "spy.h"

struct context {
	playlist_t source;
	playlist_t result;
	enum log_mode mode;
	struct strarr bump;
	struct strarr dump;
	struct strarr bulk;
};

static void __init(struct context* ctx, const char* filename,
		   enum log_mode mode)
{
	bzero(ctx, sizeof(*ctx));
	ctx->source = playlist_read(filename, 0);
	ctx->result = playlist_init(ctx->source);
	ctx->mode = mode;
}

struct strarr* __pick_untagged(struct context* ctx)
{
	switch (ctx->mode) {
	case LOG_MODE_DUMP_UNTAGGED:
		return &ctx->dump;
	case LOG_MODE_BUMP_UNTAGGED:
		return &ctx->bump;
	default:
		DIE("Unexpected mode=%d", ctx->mode);
	}
}

void __cleave(struct context* ctx)
{
	struct strarr* untagged = __pick_untagged(ctx);
	playlist_t p = ctx->source;
	int cutoff = playlist_cutoff(p) + 1;

	for (int i = 0; i < p->count; i++) {
		track_t t = &p->tracks[i];

		struct strarr* accum;

		if (track_has_tag(t, "bump!"))
			accum = &ctx->bump;
		else if (track_has_tag(t, "dump!"))
			accum = &ctx->dump;
		else if (i < cutoff)
			accum = untagged;
		else
			accum = &ctx->bulk;

		strarr_add(accum, t->id);
		track_remove_tag(t, "bump!");
		track_remove_tag(t, "dump!");
	}
}

void __reverse(struct strarr* x)
{
	int a = 0, z = x->count - 1;

	while (a < z) {
		char* t = x->data[a];
		x->data[a] = x->data[z];
		x->data[z] = t;
		a++;
		z--;
	}
}

char* __pop(struct strarr* x)
{
	if (x->count == 0)
		return NULL;
	x->count--;
	return x->data[x->count];
}

void __flush(struct strarr* dst, struct strarr* src, int count)
{
	for (int i = 0; i < count; i++) {
		char* s = __pop(src);
		if (s == NULL)
			return;
		strarr_add(dst, s);
		free(s);
	}
}

void __splice(struct context* ctx)
{
	struct strarr tids;
	bzero(&tids, sizeof(tids));

	strarr_shuffle(&ctx->bump, 0);
	strarr_shuffle(&ctx->dump, 0);
	__reverse(&ctx->bulk);

	__flush(&tids, &ctx->bulk, ctx->source->bump_offset);

	while (ctx->bump.count > 0) {
		__flush(&tids, &ctx->bump, 1);
		__flush(&tids, &ctx->bulk, ctx->source->bump_spacing);
	}

	__flush(&tids, &ctx->bulk, ctx->bulk.count);
	__flush(&tids, &ctx->dump, ctx->dump.count);

	strarr_clear(&ctx->bump);
	strarr_clear(&ctx->dump);
	strarr_clear(&ctx->bulk);

	for (int i = 0; i < tids.count; i++) {
		track_t t = playlist_lookup(ctx->source, tids.data[i]);
		if (t == NULL)
			DIE("That's weird");
		playlist_add(ctx->result, t);
	}

	strarr_clear(&tids);
}

void cmd_log(const char* filename, enum log_mode mode)
{
	struct context ctx;

	__init(&ctx, filename, mode);
	__cleave(&ctx);
	__splice(&ctx);

	fs_write_playlist(ctx.result, filename);

	playlist_free(ctx.source);
	playlist_free(ctx.result);
}
