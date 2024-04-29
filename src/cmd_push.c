#include <string.h>
#include <strings.h>

#include "spy.h"

static void __remove(playlist_t local, playlist_t remote, bool dryrun)
{
	struct strarr tids;
	bzero(&tids, sizeof(tids));

	for (int i = 0; i < local->count; i++) {
		track_t track = &local->tracks[i];

		if (! track_remove_tag(track, "remove!"))
			continue;

		track_t remote_track = playlist_lookup(remote, track->id);

		if (remote_track) {
			strarr_add(&tids, track->id);
			track_clear(remote_track);
		}

		track_clear(track);
	}

	if (dryrun) {
		for (int i = 0; i < tids.count; i++)
			printf("REMOVE %s\n", tids.data[i]);
	} else if (tids.count) {
		api_remove_tracks(local->playlist_id, &tids);
	}

	playlist_pack(local);
	playlist_pack(remote);
	strarr_clear(&tids);
}

static void __add(playlist_t local, playlist_t remote, bool dryrun)
{
	struct strarr tids;
	bzero(&tids, sizeof(tids));

	for (int i = 0; i < local->count; i++) {
		track_t track = &local->tracks[i];

		if (! track_remove_tag(track, "add!"))
			continue;

		track_t remote_track = playlist_lookup(remote, track->id);

		if (remote_track)
			continue;

		strarr_add(&tids, track->id);

		struct track tmp;
		bzero(&tmp, sizeof(tmp));
		tmp.id = strdup(track->id);
		playlist_add(remote, &tmp);
	}

	if (dryrun) {
		for (int i = 0; i < tids.count; i++)
			printf("ADD %s\n", tids.data[i]);
	} else if (tids.count) {
		api_add_tracks(local->playlist_id, &tids);
	}
	strarr_clear(&tids);
}

struct strarr __gather_local(struct playlist* local, struct playlist* remote,
			     bool* issues)
{
	struct strarr ret;
	bzero(&ret, sizeof(ret));

	for (int i = 0; i < local->count; i++) {
		struct track* track = &local->tracks[i];
		char* id = track->id;
		if (! playlist_lookup(remote, id)) {
			fprintf(stderr,
				"In local but not in remote: %s (%s by %s)\n",
				id,
				track->name,
				track->artists.data[0]);
			*issues = true;
		}
		strarr_add(&ret, id);
	}

	return ret;
}

struct strarr __gather_remote(struct playlist* local, struct playlist* remote,
			      bool* issues)
{
	struct strarr ret;
	bzero(&ret, sizeof(ret));

	for (int i = 0; i < remote->count; i++) {
		struct track* track = &remote->tracks[i];
		char* id = track->id;
		if (! playlist_lookup(local, id)) {
			fprintf(stderr,
				"In remote but not in local: %s (%s by %s)\n",
				id,
				track->name,
				track->artists.data[0]);
			*issues = true;
		}

		strarr_add(&ret, id);
	}

	return ret;
}

struct move {
	int range_start;
	int insert_before;
	int range_length;
};

static void __apply(int* indices, int count, struct move* m)
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

bool __analyze(struct move* m, int* indices, int count)
{
	int first_unordered_index = -1;

	for (int i = 0; i < count; i++)
		if (indices[i] != i) {
			first_unordered_index = i;
			break;
		}

	if (first_unordered_index < 0)
		return false;

	int first_move_value = first_unordered_index;
	int first_move_index = -1;

	for (int i = 0; i < count; i++)
		if (indices[i] == first_move_value) {
			first_move_index = i;
			break;
		}

	if (first_move_index == -1)
		DIE("This is very weird");

	int last_move_index = first_move_index;
	int first_unordered_value = indices[first_unordered_index];
	int lookup_value = first_unordered_value - 1;

	int move_index_limit = first_move_index + 100;
	if (move_index_limit > count)
		move_index_limit = count;

	for (int i = first_move_index + 1; i < move_index_limit; i++) {
		if (indices[i] == lookup_value) {
			last_move_index = i;
			break;
		}
		if (indices[i] < lookup_value) {
			last_move_index = i;
		}
	}

	m->range_start = first_move_index;
	m->insert_before = first_unordered_index;
	m->range_length = (last_move_index - first_move_index) + 1;

	__apply(indices, count, m);

	return true;
}

int __plan(struct move* plan, struct strarr* local_tids,
	   struct strarr* remote_tids)
{
	int count = local_tids->count;
	int indices[count];

	for (int i = 0; i < count; i++)
		indices[i] = strarr_seek(local_tids, remote_tids->data[i]);

	int fill = 0;

	while (__analyze(&plan[fill], indices, count)) {
		fill++;
		if (fill >= count)
			DIE("This is very weird");
	}

	return fill;
}

void __reorder(struct playlist* local, struct playlist* remote, bool dryrun)
{
	bool issues = false;

	struct strarr local_tids = __gather_local(local, remote, &issues);
	struct strarr remote_tids = __gather_remote(local, remote, &issues);

	if (issues)
		exit(1);

	struct move plan[local->count];
	int moves = __plan(plan, &local_tids, &remote_tids);

	if (dryrun) {
		for (int i = 0; i < moves; i++)
			printf("MOVE insert_before=%d range_start=%d "
			       "range_length=%d\n",
			       plan[i].insert_before,
			       plan[i].range_start,
			       plan[i].range_length);
	} else {
		for (int i = 0; i < moves; i++) {
			fputc('.', stdout);
			fflush(stdout);
			api_reorder(local->playlist_id,
				    plan[i].range_start,
				    plan[i].insert_before,
				    plan[i].range_length);
		}
		if (moves > 0) {
			fputc('\n', stdout);
			fflush(stdout);
		}
	}

	strarr_clear(&local_tids);
	strarr_clear(&remote_tids);
}

static void __reindex(struct playlist* p)
{
	for (int i = 0; i < p->count; i++)
		p->tracks[i].remote_index = i + 1;
}

void cmd_push(const char* filename, bool dryrun)
{
	playlist_t local = playlist_read(filename, VF_PLAYLIST_ID);
	playlist_t remote = api_get_playlist(local->playlist_id);

	__remove(local, remote, dryrun);
	__add(local, remote, dryrun);
	__reorder(local, remote, dryrun);
	__reindex(local);

	if (! dryrun)
		fs_write_playlist(local, filename);

	playlist_free(local);
	playlist_free(remote);
}
