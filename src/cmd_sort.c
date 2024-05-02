#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "spy.h"

struct context {
	playlist_t source;
	playlist_t ret;
	int spacing;
	struct strarr* tags;
	int wanted_tag;
	int skipped_tags;
};

static bool __have_same_artists(struct context* ctx, track_t tx, track_t ty)
{
	struct strarr* xs = &tx->artists;
	struct strarr* ys = &ty->artists;

	for (int i = 0; i < xs->count; i++)
		if (strarr_has(ys, xs->data[i]))
			return true;

	for (int i = 0; i < ys->count; i++)
		if (strarr_has(xs, ys->data[i]))
			return true;

	struct strarr* as = &ctx->source->aliases;

	for (int i = 0; i < as->count; i += 2) {
		const char* p = as->data[i];
		const char* q = as->data[i + 1];

		if (strarr_has(xs, p) && strarr_has(ys, q))
			return true;
		if (strarr_has(xs, q) && strarr_has(xs, p))
			return true;
	}

	return false;
}

static bool __artists_overlap(struct context* ctx, track_t t)
{
	int index = ctx->ret->count - ctx->spacing;
	if (index < 0)
		index = 0;

	for (int i = index; i < ctx->ret->count; i++)
		if (__have_same_artists(ctx, t, &ctx->ret->tracks[i]))
			return true;

	return false;
}

static bool __names_overlap(struct context* ctx, track_t t)
{
	int index = ctx->ret->count - ctx->spacing;
	if (index < 0)
		index = 0;

	for (int i = index; i < ctx->ret->count; i++)
		if (! strcmp(t->name, ctx->ret->tracks[i].name))
			return true;

	return false;
}

static bool __tags_match(struct context* ctx, track_t t)
{
	if (track_has_tag(t, "new?"))
		return ctx->wanted_tag == -1;

	if (ctx->wanted_tag == -1)
		return false;

	if (! ctx->tags)
		return true;

	char* tag = ctx->tags->data[ctx->wanted_tag];
	return track_has_tag(t, tag);
}

static bool __track_fits(struct context* ctx, track_t t)
{
	if (! t->id)
		return false;

	if (__artists_overlap(ctx, t))
		return false;

	if (__names_overlap(ctx, t))
		return false;

	return __tags_match(ctx, t);
}

static track_t __pick_track_trial(struct context* ctx)
{
	int spacing = ctx->tags ? ctx->tags->count : 1;
	int i = ctx->ret->count - spacing;

	if (i < 0)
		return NULL;

	for (; i < ctx->ret->count; i++) {
		if (track_has_tag(&ctx->ret->tracks[i], "new?"))
			return false;
		if (track_has_tag(&ctx->ret->tracks[i], "maybe?"))
			return false;
	}

	track_t t = NULL;
	while (playlist_iterate(&t, ctx->source))
		if (track_has_tag(t, "new?") && __track_fits(ctx, t))
			return t;

	t = NULL;
	while (playlist_iterate(&t, ctx->source))
		if (track_has_tag(t, "maybe?") && __track_fits(ctx, t))
			return t;

	return NULL;
}

static track_t __pick_track_race(struct context* ctx)
{
	track_t t = NULL;

	while (playlist_iterate(&t, ctx->source)) {
		if (track_has_tag(t, "new?"))
			continue;
		if (track_has_tag(t, "maybe?"))
			continue;
		if (__track_fits(ctx, t))
			return t;
	}

	return NULL;
}

static track_t __pick_track(struct context* ctx, enum sort_mode mode)
{
	track_t t;
	switch (mode) {
	case SORT_MODE_DEFAULT:
		if ((t = __pick_track_trial(ctx)))
			return t;
	case SORT_MODE_RACE:
		if ((t = __pick_track_race(ctx)))
			return t;
		return NULL;
	default:
		DIE("Invalid sort mode %d", mode);
	}
}

static void __record_hit(struct context* ctx)
{
	ctx->skipped_tags = 0;
	ctx->wanted_tag++;

	int tag_count = ctx->tags ? ctx->tags->count : 1;
	if (ctx->wanted_tag == tag_count)
		ctx->wanted_tag = -1;
}

static bool __record_miss(struct context* ctx)
{
	if (ctx->wanted_tag == -1) {
		ctx->skipped_tags = 0;
		ctx->wanted_tag = 0;
		return true;
	}

	ctx->skipped_tags++;
	ctx->wanted_tag++;

	int tag_count = ctx->tags ? ctx->tags->count : 1;

	if (ctx->skipped_tags == tag_count)
		return false;

	if (ctx->wanted_tag == tag_count)
		ctx->wanted_tag = -1;

	return true;
}

void cmd_sort(const char* filename, enum sort_mode mode)
{
	playlist_t playlist = playlist_read(filename, 0);
	playlist_t tmp = playlist_init(playlist);

	struct context ctx = {
		.ret = tmp,
		.source = playlist,
		.tags = NULL,
		.spacing = playlist->same_artist_spacing,
		.wanted_tag = 0,
		.skipped_tags = 0,
	};

	struct strarr tags;
	if (playlist->sort_order) {
		strarr_split(&tags, playlist->sort_order, " ");
		ctx.tags = &tags;
	}

	bool keep_going = true;
	while (keep_going) {
		track_t t = __pick_track(&ctx, mode);
		if (t) {
			__record_hit(&ctx);
			playlist_add(tmp, t);
		} else {
			keep_going = __record_miss(&ctx);
		}
	}

	for (int i = 0; i < playlist->count; i++) {
		struct track* track = &playlist->tracks[i];
		if (track->id)
			playlist_add(tmp, track);
	}

	if (playlist->sort_order)
		strarr_clear(&tags);

	playlist_free(playlist);
	playlist = tmp;

	fs_write_playlist(playlist, filename);

	playlist_free(playlist);
}
