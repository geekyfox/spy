#include <string.h>
#include <strings.h>

#include "spy.h"

struct context {
	playlist_t playlist;
	struct strarr tags;
	int* stack;
	int stack_ix;
};

static void __gather_tags(struct context* ctx)
{
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->playlist)) {
		for (int i = 0; i < t->tags.count; i++) {
			const char* tag = t->tags.data[i];
			if (strarr_has(&ctx->tags, tag))
				continue;
			strarr_add(&ctx->tags, tag);
		}
	}

	strarr_sort(&ctx->tags);

	if (! ctx->playlist->sort_order)
		return;

	struct strarr sort_order;
	strarr_split(&sort_order, ctx->playlist->sort_order, " ");

	int fill = 0;

	for (int i = 0; i < sort_order.count; i++) {
		int index = strarr_seek(&ctx->tags, sort_order.data[i]);
		if (index < fill)
			continue;
		strarr_shift(&ctx->tags, index, fill);
		fill++;
	}

	strarr_clear(&sort_order);
}

static bool __push(struct context* ctx, int tagix)
{
	for (int i = 0; i < ctx->stack_ix; i++)
		if (ctx->stack[i] == tagix)
			return false;
	ctx->stack[ctx->stack_ix++] = tagix;
	return true;
}

static void __pop(struct context* ctx, int tagix)
{
	if (ctx->stack_ix <= 0)
		DIE("Buffer underflow");
	ctx->stack_ix--;

	int popix = ctx->stack[ctx->stack_ix];
	if (popix != tagix)
		DIE("Expected to pop %d, got %d", tagix, popix);
}

static bool __match(struct context* ctx, track_t t)
{
	for (int i = 0; i < ctx->stack_ix; i++) {
		int tagix = ctx->stack[i];
		const char* tag = ctx->tags.data[tagix];
		if (! track_has_tag(t, tag))
			return false;
	}
	return true;
}

static bool __match_exact(struct context* ctx, track_t t)
{
	if (t->tags.count != ctx->stack_ix)
		return false;
	return __match(ctx, t);
}

static int __count_total(struct context* ctx)
{
	int result = 0;
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->playlist))
		if (__match(ctx, t))
			result++;
	return result;
}

static int __count_leaf(struct context* ctx)
{
	int result = 0;
	track_t t = NULL;
	while (playlist_iterate(&t, ctx->playlist))
		if (__match_exact(ctx, t))
			result++;
	return result;
}

static void __pad(int count)
{
	for (int i = count * 4; i > 0; i--)
		fputc(' ', stdout);
}

static void __say_total(struct context* ctx, int total)
{
	__pad(ctx->stack_ix - 1);
	int tagix = ctx->stack[ctx->stack_ix - 1];
	const char* tag = ctx->tags.data[tagix];
	printf("%s : %d\n", tag, total);
}

static void __say_leaf(struct context* ctx, int total)
{
	__pad(ctx->stack_ix);
	printf("* : %d\n", total);
}

static void __analyze(struct context* ctx)
{
	for (int i = 0; i < ctx->tags.count; i++) {
		if (! __push(ctx, i))
			continue;

		int total = __count_total(ctx);
		if (total == 0) {
			__pop(ctx, i);
			continue;
		}

		__say_total(ctx, total);

		int leaf = __count_leaf(ctx);

		if (leaf == total) {
			__pop(ctx, i);
			continue;
		}

		if (leaf > 0)
			__say_leaf(ctx, leaf);

		__analyze(ctx);
		__pop(ctx, i);
	}
}

void cmd_stats(const char* filename)
{
	playlist_t playlist = playlist_read(filename, 0);

	struct context ctx;
	bzero(&ctx, sizeof(ctx));
	ctx.playlist = playlist;
	__gather_tags(&ctx);

	ctx.stack = malloc(sizeof(int) * ctx.tags.count);

	__analyze(&ctx);

	free(ctx.stack);
	playlist_free(playlist);
	strarr_clear(&ctx.tags);
}
