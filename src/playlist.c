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

playlist_t playlist_read(const char* filename, int flags)
{
	playlist_t ret = fs_read_playlist(filename);

	char buffer[10240];
	sprintf(buffer, "local playlist from file %s", filename);
	validate_playlist(ret, buffer, flags);

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

bool playlist_iterate(track_t* tptr, playlist_t p)
{
	int index = *tptr ? (*tptr - p->tracks + 1) : 0;

	while (index < p->count) {
		if (p->tracks[index].id == NULL) {
			index++;
		} else {
			*tptr = &p->tracks[index];
			return true;
		}
	}

	return false;
}

track_t playlist_lookup(playlist_t p, const char* track_id)
{
	if (track_id == NULL)
		DIE("That's unexpected");

	for (int i = 0; i < p->count; i++) {
		track_t t = &p->tracks[i];
		if ((t->id != NULL) && (strcmp(t->id, track_id) == 0))
			return t;
	}

	return NULL;
}

void playlist_free(playlist_t p)
{
	free(p->playlist_id);
	free(p->sort_order);

	for (int i = 0; i < p->count; i++)
		track_clear(&p->tracks[i]);
	free(p->tracks);

	strarr_clear(&p->header);
	free(p);
}

void playlist_pack(playlist_t p)
{
	int wix = -1;

	for (int i = 0; i < p->count; i++) {
		if (p->tracks[i].id == NULL) {
			wix = i;
			break;
		}
	}

	if (wix == -1)
		return;

	for (int i = wix; i < p->count; i++) {
		if (p->tracks[i].id == NULL)
			continue;
		p->tracks[wix++] = p->tracks[i];
	}

	p->count = wix;
}
