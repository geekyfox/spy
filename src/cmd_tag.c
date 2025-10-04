#include <string.h>

#include "spy.h"

struct context {
	struct strarr add;
	struct strarr remove;
	struct strarr files;
	bool infer;
	bool clear;
	bool need_help;
	struct strarr sort_order;
};

static bool __nothing_to_do(struct context* ctx)
{
	if (ctx->files.count == 0)
		return true;

	if (ctx->add.count > 0)
		return false;

	if (ctx->remove.count > 0)
		return false;

	if (ctx->infer)
		return false;

	if (ctx->clear)
		return false;

	return true;
}

static void __parse_args(struct context* ctx, char** args)
{
	while (*args) {
		char* arg = *args;

		if (! strcmp(arg, "--help"))
			ctx->need_help = true;
		else if (! strcmp(arg, "--infer"))
			ctx->infer = true;
		else if (! strcmp(arg, "--clear"))
			ctx->clear = true;
		else if (arg[0] == '+')
			strarr_add(&ctx->add, arg + 1);
		else if (arg[0] == '-')
			strarr_add(&ctx->remove, arg + 1);
		else
			strarr_add(&ctx->files, arg);
		args++;
	}

	if (__nothing_to_do(ctx))
		ctx->need_help = true;
}

static void __prep_infer(struct context* ctx, playlist_t p, const char* fname)
{
	if (p->sort_order) {
		strarr_split(&ctx->sort_order, p->sort_order, " ");

		if (ctx->sort_order.count >= 1)
			return;
	}

	fprintf(stderr, "Playlist %s has no sort_order", fname);
	fprintf(stderr, ", --infer option will be ignored\n");

	strarr_clear(&ctx->sort_order);
}

static void __infer(struct context* ctx, track_t t, int track_index)
{
	int tag_index = track_index % ctx->sort_order.count;
	const char* tag = ctx->sort_order.data[tag_index];
	track_add_tag(t, tag);
}

static void __tag(struct context* ctx, const char* filename)
{
	playlist_t playlist = playlist_read(filename, 0);

	if (ctx->infer)
		__prep_infer(ctx, playlist, filename);

	for (int i = 0; i < playlist->count; i++) {
		track_t track = &playlist->tracks[i];

		if (ctx->clear)
			strarr_clear(&track->tags);

		for (int j = 0; j < ctx->remove.count; j++)
			track_remove_tag(track, ctx->remove.data[j]);

		for (int j = 0; j < ctx->add.count; j++)
			track_add_tag(track, ctx->add.data[j]);

		if (ctx->infer)
			__infer(ctx, track, i);
	}

	fs_write_playlist(playlist, filename);
	playlist_free(playlist);
	strarr_clear(&ctx->sort_order);
}

static void __cleanup(struct context* ctx)
{
	strarr_clear(&ctx->add);
	strarr_clear(&ctx->remove);
	strarr_clear(&ctx->files);
}

int cmd_tag(char** args)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	__parse_args(&ctx, args);
	if (ctx.need_help) {
		fprintf(stderr,
			"Usage: spy tag { +<tag> | -<tag> | --infer }+ "
			"<filename>\n");
		__cleanup(&ctx);
		return 1;
	}

	for (int i = 0; i < ctx.files.count; i++) {
		__tag(&ctx, ctx.files.data[i]);
	}

	__cleanup(&ctx);
	return 0;
}
