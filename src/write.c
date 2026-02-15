#include "spy.h"

static void __write_track(FILE* f, struct track* tr, int index)
{
	if (tr->remote_index == index) {
		fprintf(f, "[%d] ", index);
	} else {
		fprintf(f, "[%d | %d] ", index, tr->remote_index);
	}

	for (int i = 0; i < tr->artists.count; i++) {
		if (i != 0)
			fprintf(f, ", ");
		fprintf(f, "%s", tr->artists.data[i]);
	}

	fprintf(f, " - %s\n", tr->name);
	fprintf(f, "~ %s\n", tr->id);
	fprintf(f, "~ ");
	for (int i = 0; i < tr->tags.count; i++) {
		if (i != 0)
			fprintf(f, " ");
		fprintf(f, "%s", tr->tags.data[i]);
	}
	fprintf(f, "\n\n");
}

void write_playlist(playlist_t p, FILE* f)
{
	if (p->playlist_id)
		fprintf(f, "playlist_id = %s\n", p->playlist_id);

	if (p->sort_order)
		fprintf(f, "sort_order = %s\n", p->sort_order);

	if (p->spacing)
		fprintf(f, "spacing = %d\n", p->spacing);

	if (p->bump_offset)
		fprintf(f, "bump_offset = %d\n", p->bump_offset);

	if (p->bump_spacing)
		fprintf(f, "bump_spacing = %d\n", p->bump_spacing);

	for (int i = 0; i < p->aliases.count; i += 2) {
		char* x = p->aliases.data[i];
		char* y = p->aliases.data[i + 1];
		fprintf(f, "aliases += %s == %s\n", x, y);
	}

	struct tag_spacing* ts = p->tag_spacing;
	while (ts) {
		fprintf(f, "tag_spacing += %s %d\n", ts->tag, ts->spacing);
		ts = ts->next;
	}

	fprintf(f, "\n");

	for (int i = 0; i < p->count; i++)
		__write_track(f, &p->tracks[i], i + 1);
}
