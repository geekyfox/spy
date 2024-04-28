#include <ctype.h>
#include <string.h>

#include "spy.h"

playlist_t playlist_init(playlist_t original)
{
	playlist_t ret = malloc(sizeof(*ret));
	bzero(ret, sizeof(*ret));
	ret->separator = -1;

	if (original) {
		if (original->playlist_id)
			ret->playlist_id = strdup(original->playlist_id);
		if (original->sort_order)
			ret->sort_order = strdup(original->sort_order);
		ret->same_artist_spacing = original->same_artist_spacing;
		ret->bump_offset = original->bump_offset;
		ret->bump_spacing = original->bump_spacing;
		strarr_set(&ret->header, &original->header);
	}
	return ret;
}

void playlist_add(playlist_t p, track_t t)
{
	if (p->alc == p->count) {
		p->alc = p->alc * 2 + 8;
		p->tracks = realloc(p->tracks, sizeof(struct track) * p->alc);
	}

	p->tracks[p->count++] = *t;
	bzero(t, sizeof(*t));
}

void playlist_validate(playlist_t p, const char* source)
{
	bool issues = false;
	char header[1024];
	sprintf(header, "Issues detected in %s:\n", source);

	bool duplicates = false;

	for (int i = 0; i < p->count; i++) {
		for (int j = i + 1; j < p->count; j++) {
			if (strcmp(p->tracks[i].id, p->tracks[j].id) != 0)
				continue;
			if (! issues) {
				fputs(header, stderr);
				issues = true;
			}
			if (! duplicates) {
				fputs("    Duplicate tracks:\n", stderr);
				duplicates = true;
			}
			fprintf(stderr,
				"      * %d and %d (%s by %s)\n",
				i + 1,
				j + 1,
				p->tracks[i].name,
				p->tracks[i].artists.data[0]);
		}
	}

	if (issues)
		exit(1);
}

static void __clear_track(track_t tr)
{
	free(tr->id);
	free(tr->name);
	strarr_clear(&tr->artists);
	strarr_clear(&tr->tags);
	bzero(tr, sizeof(*tr));
}

void playlist_free(playlist_t p)
{
	free(p->playlist_id);
	free(p->sort_order);

	for (int i = 0; i < p->count; i++)
		__clear_track(&p->tracks[i]);
	free(p->tracks);

	strarr_clear(&p->header);
	free(p);
}
