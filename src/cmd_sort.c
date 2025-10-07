#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "spy.h"

enum sort_mode {
	SORT_MODE_DEFAULT = 45001,
	SORT_MODE_RACE = 45002,
	SORT_MODE_TRIAL = 45003,
};

struct context {
	enum sort_mode mode;
	playlist_t source;
	playlist_t ret;
	int spacing;
	struct strarr* tags;
	int tag_count;
	int wanted_tag;
	int skipped_tags;
	bool relaxed_fit;
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
		if (strarr_has(xs, q) && strarr_has(ys, p))
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
	if (! ctx->tags)
		return true;

	char* tag = ctx->tags->data[ctx->wanted_tag];
	return track_has_tag(t, tag);
}

static bool __spaced_tags_overlap(struct context* ctx, track_t t)
{
	struct tag_spacing* ts = ctx->source->tag_spacing;

	while (ts) {
		if (! track_has_tag(t, ts->tag)) {
			ts = ts->next;
			continue;
		}

		int i = ctx->ret->count - ts->spacing;
		if (i < 0)
			i = 0;

		for (; i < ctx->ret->count; i++) {
			if (track_has_tag(&ctx->ret->tracks[i], ts->tag))
				return true;
		}

		ts = ts->next;
	}

	return false;
}

static bool __track_fits(struct context* ctx, track_t t)
{
	if (! t->id)
		return false;

	if (! __tags_match(ctx, t))
		return false;

	if (ctx->relaxed_fit)
		return true;

	if (__artists_overlap(ctx, t))
		return false;

	if (__names_overlap(ctx, t))
		return false;

	if (__spaced_tags_overlap(ctx, t))
		return false;

	return true;
}

static track_t __pick_track_default(struct context* ctx)
{
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->source)) {
		if (__track_fits(ctx, t))
			return t;
	}
	return NULL;
}

static track_t __pick_track_race(struct context* ctx)
{
	track_t not_yes = NULL, t = NULL;

	while (playlist_iterate(&t, ctx->source)) {
		if (! __track_fits(ctx, t))
			continue;

		if (track_has_tag(t, "yes"))
			return t;

		if (! not_yes)
			not_yes = t;
	}

	return not_yes;
}

static bool __trial_wants_yes(struct context* ctx)
{
	track_t tracks = ctx->ret->tracks;
	int count = ctx->ret->count;

	if (count < 2)
		return true;
	if (! track_has_tag(&tracks[count - 1], "yes"))
		return true;
	if (! track_has_tag(&tracks[count - 2], "yes"))
		return true;

	if ((! ctx->tags) || (ctx->tags->count < 2))
		return false;

	const char* tag = ctx->tags->data[ctx->wanted_tag];
	int tag_reps = 0;

	for (int i = ctx->tags->count - 1; i >= 0; i--)
		if (! strcmp(tag, ctx->tags->data[i]))
			tag_reps++;

	for (int i = count - 1; i >= 0; i--) {
		if (! track_has_tag(&tracks[i], tag))
			continue;
		if (! track_has_tag(&tracks[i], "yes"))
			return true;
		tag_reps--;
		if (tag_reps == 0)
			return false;
	}

	return false;
}

static track_t __pick_track_trial(struct context* ctx)
{
	bool wants_yes = __trial_wants_yes(ctx);

	track_t t = NULL, result = NULL;

	while (playlist_iterate(&t, ctx->source)) {
		if (! __track_fits(ctx, t))
			continue;
		if (track_has_tag(t, "yes") == wants_yes)
			return t;
		if (! result)
			result = t;
	}

	return result;
}

static track_t __pick_track(struct context* ctx)
{
	switch (ctx->mode) {
	case SORT_MODE_DEFAULT:
		return __pick_track_default(ctx);
	case SORT_MODE_RACE:
		return __pick_track_race(ctx);
	case SORT_MODE_TRIAL:
		return __pick_track_trial(ctx);
	default:
		DIE("Invalid sort mode %d", ctx->mode);
	}
}

static void __increment_wanted_tag(struct context* ctx)
{
	ctx->wanted_tag = (ctx->wanted_tag + 1) % ctx->tag_count;
}

static void __record_hit(struct context* ctx)
{
	ctx->skipped_tags = 0;
	__increment_wanted_tag(ctx);
}

static bool __record_miss(struct context* ctx)
{
	ctx->skipped_tags++;
	if (ctx->skipped_tags == ctx->tag_count)
		return false;

	__increment_wanted_tag(ctx);
	return true;
}

int cmd_sort(char** args)
{
	enum sort_mode mode = SORT_MODE_DEFAULT;
	const char* filename = NULL;

	while (! filename) {
		if (! *args)
			return 1;

		if (! strcmp(*args, "--race"))
			mode = SORT_MODE_RACE;
		else if (! strcmp(*args, "--trial"))
			mode = SORT_MODE_TRIAL;
		else
			filename = *args;

		args++;
	}

	playlist_t playlist = playlist_read(filename, 0);
	playlist_t tmp = playlist_init(playlist);

	struct context ctx = {
		.mode = mode,
		.source = playlist,
		.ret = tmp,
		.spacing = playlist->same_artist_spacing,
		.tags = NULL,
		.tag_count = 1,
		.wanted_tag = 0,
		.skipped_tags = 0,
		.relaxed_fit = false,
	};

	struct strarr tags;
	if (playlist->sort_order) {
		strarr_split(&tags, playlist->sort_order, " ");
		ctx.tags = &tags;
		ctx.tag_count = tags.count;
	}

	bool keep_going = true;
	while (keep_going) {
		ctx.relaxed_fit = false;
		track_t t = __pick_track(&ctx);
		if (t) {
			playlist_add(tmp, t);
			__record_hit(&ctx);
			continue;
		}

		ctx.relaxed_fit = true;
		t = __pick_track(&ctx);
		if (t) {
			playlist_add(tmp, t);
			__record_hit(&ctx);
			continue;
		}
		keep_going = __record_miss(&ctx);
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

	return 0;
}
