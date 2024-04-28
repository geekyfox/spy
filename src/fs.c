#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "spy.h"

struct strbuff fs_read(const char* pathname)
{
	FILE* f = fopen(pathname, "r");
	if (! f)
		DIE("Error opening file %s: %m", pathname);

	struct strbuff ret;
	bzero(&ret, sizeof(ret));

	int ch;
	while ((ch = fgetc(f)) != EOF)
		strbuff_addch(&ret, ch);

	strbuff_addch(&ret, '\0');

	fclose(f);
	return ret;
}

json_t fs_read_json(const char* filename)
{
	struct strbuff buff = fs_read(filename);
	json_t ret = json_parse(&buff);
	free(buff.data);
	return ret;
}

static void __write_track(FILE* f, struct track* tr, int index)
{
	fprintf(f, "%d", index);

	if (tr->remote_index != index)
		fprintf(f, " (%d)", tr->remote_index);

	fprintf(f, " :: %s :: ", tr->name);
	for (int i = 0; i < tr->artists.count; i++) {
		if (i != 0)
			fprintf(f, ", ");
		fprintf(f, "%s", tr->artists.data[i]);
	}
	fprintf(f, "\n");
	fprintf(f, "# %s\n", tr->id);
	fprintf(f, "> ");
	for (int i = 0; i < tr->tags.count; i++) {
		if (i != 0)
			fprintf(f, " ");
		fprintf(f, "%s", tr->tags.data[i]);
	}
	fprintf(f, "\n\n");
}

void fs_write_playlist(playlist_t p, const char* filename)
{
	char tmpfile[10240];
	sprintf(tmpfile, "%s.temp", filename);

	FILE* f = fopen(tmpfile, "w");
	if (! f)
		DIE("Error opening file %s: %m", tmpfile);

	for (int i = 0; i < p->header.count; i++)
		fprintf(f, "### %s\n", p->header.data[i]);

	if ((p->header.count == 0) && p->playlist_id)
		fprintf(f, "### playlist_id %s\n", p->playlist_id);

	fprintf(f, "\n");

	for (int i = 0; i < p->count; i++) {
		if (p->separator == i)
			fprintf(f, "----\n\n");
		__write_track(f, &p->tracks[i], i + 1);
	}

	fclose(f);
	rename(tmpfile, filename);
}
