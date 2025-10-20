#include <string.h>
#include <strings.h>

#include "spy.h"

struct context {
	bool add_new;
	bool force;
	playlist_t local;
	playlist_t remote;
};

static void __push(struct context*, const char* filename);

int cmd_push(char** args)
{
	struct context ctx;
	bzero(&ctx, sizeof(ctx));

	while (*args) {
		char* arg = *args;
		args++;

		if (! strcmp(arg, "--add-new")) {
			ctx.add_new = true;
		} else if (! strcmp(arg, "--force")) {
			ctx.add_new = true;
			ctx.force = true;
		} else {
			__push(&ctx, arg);
		}
	}

	return 0;
}

static void __add(struct context*);
static void __remove(struct context*);
static void __reorder(struct context*);
static void __reindex(playlist_t);

static void __push(struct context* ctx, const char* filename)
{
	ctx->local = playlist_read(filename, VF_PLAYLIST_ID);
	ctx->remote = api_get_playlist(ctx->local->playlist_id);

	__add(ctx);
	__remove(ctx);
	__reorder(ctx);
	__reindex(ctx->local);

	fs_write_playlist(ctx->local, filename);

	playlist_free(ctx->local);
	playlist_free(ctx->remote);
}

static void __add(struct context* ctx)
{
	struct strarr tids;
	bzero(&tids, sizeof(tids));

	track_t lt = NULL;
	while (playlist_iterate(&lt, ctx->local)) {
		bool should_add = track_remove_tag(lt, "add!") || ctx->add_new;
		if (! should_add)
			continue;

		if (playlist_lookup(ctx->remote, lt->id))
			continue;

		strarr_add(&tids, lt->id);

		struct track tmp;
		bzero(&tmp, sizeof(tmp));
		tmp.id = strdup(lt->id);
		playlist_add(ctx->remote, &tmp);
	}

	if (tids.count)
		api_add_tracks(ctx->local->playlist_id, &tids);

	strarr_clear(&tids);
}

static void __grab_tagged_remove(struct context*, struct strarr*);
static void __grab_locally_missing(struct context*, struct strarr*);

static void __remove(struct context* ctx)
{
	struct strarr tids;
	bzero(&tids, sizeof(tids));

	__grab_tagged_remove(ctx, &tids);
	if (ctx->force)
		__grab_locally_missing(ctx, &tids);

	if (tids.count)
		api_remove_tracks(ctx->local->playlist_id, &tids);

	strarr_clear(&tids);
}

static void __grab_tagged_remove(struct context* ctx, struct strarr* tids)
{
	track_t lt = NULL;
	while (playlist_iterate(&lt, ctx->local)) {
		if (! track_remove_tag(lt, "remove!"))
			continue;

		track_t rt = playlist_lookup(ctx->remote, lt->id);
		if (rt) {
			strarr_add(tids, lt->id);
			track_clear(rt);
		}

		track_clear(lt);
	}
}

static void __grab_locally_missing(struct context* ctx, struct strarr* tids)
{
	track_t rt = NULL;
	while (playlist_iterate(&rt, ctx->remote)) {
		if (playlist_lookup(ctx->local, rt->id))
			continue;

		strarr_add(tids, rt->id);
		track_clear(rt);
	}
}

static int* __gather_mapping(struct context*);
static bool __plan(struct reorder_move*, int*, int);

void __reorder(struct context* ctx)
{
	int* mapping = __gather_mapping(ctx);
	int count = ctx->local->count;
	int moves = 0;
	struct reorder_move move;

	while (__plan(&move, mapping, count)) {
		fputc('.', stdout);
		fflush(stdout);
		api_reorder(ctx->local->playlist_id, move);

		moves++;
		if (moves > count)
			DIE("Something is not right");
	}

	if (moves > 0) {
		fputc('\n', stdout);
		fflush(stdout);
	}

	free(mapping);
}

static void __eprintln_track(const char* issue, track_t t)
{
	fputs(issue, stderr);
	fprintf(stderr, "%s (%s by %s)\n", t->id, t->name, t->artists.data[0]);
}

static int* __gather_mapping(struct context* ctx)
{
	playlist_pack(ctx->local);
	playlist_pack(ctx->remote);

	int* mapping = malloc(ctx->remote->count * sizeof(int));

	bool issues = false;

	for (int i = 0; i < ctx->remote->count; i++) {
		mapping[i] = -1;
		track_t t = &ctx->remote->tracks[i];

		for (int j = 0; j < ctx->local->count; j++) {
			if (! strcmp(t->id, ctx->local->tracks[j].id)) {
				mapping[i] = j;
				break;
			}
		}

		if (mapping[i] < 0) {
			__eprintln_track("In remote but not in local: ", t);
			issues = true;
		}
	}

	track_t t = NULL;
	while (playlist_iterate(&t, ctx->local)) {
		if (! playlist_lookup(ctx->remote, t->id)) {
			__eprintln_track("In local but not in remote: ", t);
			issues = true;
		}
	}

	if (issues)
		exit(1);

	return mapping;
}

static void __apply(int* indices, int count, struct reorder_move* m)
{
	int tmp[count];
	int fill = 0;

	for (int i = 0; i < m->insert_before; i++)
		tmp[fill++] = indices[i];

	for (int i = 0; i < m->range_length; i++)
		tmp[fill++] = indices[m->range_start + i];

	for (int i = m->insert_before; i < m->range_start; i++)
		tmp[fill++] = indices[i];

	for (int i = fill; i < count; i++)
		tmp[fill++] = indices[i];

	for (int i = 0; i < count; i++)
		indices[i] = tmp[i];
}

static bool __plan(struct reorder_move* m, int* indices, int count)
{
	// Let's say, numbers go like this: [0 1 2 7 3 5 4 8 6]
	//
	// Step one is we search for the first misordered pair,
	// i.e. 2/7 in the example above.
	//
	// Here we "pretend" that array has a -1 at index -1, so
	// if there's a non-zero N at index 0, then the first
	// misordered pair is -1/N.

	m->insert_before = -1;

	for (int i = 0; i < count; i++) {
		if (indices[i] != i) {
			m->insert_before = i;
			break;
		}
	}

	if (m->insert_before < 0) {
		// We didn't find any. It means that array is fully
		// sorted and there's nothing else to do.
		return false;
	}

	// Step two is we search for the first element of the
	// slice to move into the gap. In the example above this
	// would be 3.

	m->range_start = -1;

	for (int i = 0; i < count; i++)
		if (indices[i] == m->insert_before) {
			m->range_start = i;
			break;
		}

	if (m->range_start < 0)
		DIE("This is very weird");

	int lookup_value = indices[m->insert_before] - 1;

	int range_max = count - m->range_start;
	if (range_max > 100)
		range_max = 100;

	m->range_length = 1;

	// Step three is we search for the last element of the slice
	// to move into the gap. Ideally we'd want it to be 6, so
	// we connect both 2-3 and 6-7. But we're also fine with
	// a mixture of numbers between 3 and 7 (tbd: explain why).

	for (int i = 1; i < range_max; i++) {
		int value = indices[m->range_start + i];
		if (value <= lookup_value)
			m->range_length = i + 1;
		if (value == lookup_value)
			break;
	}

	__apply(indices, count, m);

	return true;
}

static void __reindex(struct playlist* p)
{
	for (int i = 0; i < p->count; i++)
		p->tracks[i].remote_index = i + 1;
}
