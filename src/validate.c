#include <string.h>

#include "spy.h"

struct context {
	playlist_t playlist;
	int flags;
	const char* source;
	bool issues;
	bool duplicates;
};

static void __check_pair_unique(struct context* ctx, int i, int j)
{
	track_t ti = &ctx->playlist->tracks[i];
	track_t tj = &ctx->playlist->tracks[j];

	if (strcmp(ti->id, tj->id))
		return;

	if (! ctx->issues) {
		fprintf(stderr, "Issues detected in %s\n", ctx->source);
		ctx->issues = true;
	}

	if (! ctx->duplicates) {
		fputs("    Duplicate tracks:\n", stderr);
		ctx->duplicates = true;
	}

	fprintf(stderr,
		"      * %d and %d (%s by %s)\n",
		i + 1,
		j + 1,
		ti->name,
		ti->artists.data[0]);
}

static void __check_unique(struct context* ctx)
{
	int count = ctx->playlist->count;
	for (int i = 0; i < count; i++)
		for (int j = i + 1; j < count; j++)
			__check_pair_unique(ctx, i, j);
}

static void __check_playlist_id(struct context* ctx)
{
	if (ctx->playlist->playlist_id)
		return;

	if (! ctx->issues) {
		fprintf(stderr, "Issues detected in %s\n", ctx->source);
		ctx->issues = true;
	}

	fputs("    Playlist doesn't have playlist_id\n", stderr);
}

void validate_playlist(playlist_t p, const char* source, int flags)
{
	struct context ctx = {
		.playlist = p,
		.flags = flags,
		.source = source,
		.issues = false,
		.duplicates = false,
	};

	if (! (flags & VF_SKIP_UNIQUE))
		__check_unique(&ctx);

	if (flags & VF_PLAYLIST_ID)
		__check_playlist_id(&ctx);

	if (ctx.issues)
		exit(1);
}
