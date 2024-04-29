#include <string.h>
#include <strings.h>

#include "spy.h"

void track_add_tag(track_t track, const char* tag)
{
	if (! track_has_tag(track, tag))
		strarr_add(&track->tags, tag);
}

bool track_has_tag(track_t track, const char* tag)
{
	if (tag[0] == '!')
		return ! strarr_has(&track->tags, &tag[1]);
	return strarr_has(&track->tags, tag);
}

bool track_remove_tag(track_t track, const char* tag)
{
	struct strarr* tags = &track->tags;
	for (int i = 0; i < tags->count; i++) {
		if (! strcmp(tags->data[i], tag)) {
			free(tags->data[i]);
			tags->count--;
			tags->data[i] = tags->data[tags->count];
			return true;
		}
	}
	return false;
}

void track_clear(track_t tr)
{
	free(tr->id);
	free(tr->name);
	strarr_clear(&tr->artists);
	strarr_clear(&tr->tags);
	bzero(tr, sizeof(*tr));
}
