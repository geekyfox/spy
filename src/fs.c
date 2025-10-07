#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

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

static char* __resolve(const char* filename)
{
	struct stat stat;
	if (lstat(filename, &stat) < 0) {
		if (errno == ENOENT)
			return strdup(filename);
		DIE("lstat() failed: %m");
	}

	if ((stat.st_mode & S_IFMT) != S_IFLNK)
		return strdup(filename);

	char* result = realpath(filename, NULL);
	if (! result)
		DIE("realpath() failed: %m");

	return result;
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

	for (int i = 0; i < p->count; i++)
		__write_track(f, &p->tracks[i], i + 1);

	fclose(f);

	char* real_filename = __resolve(filename);
	rename(tmpfile, real_filename);
	free(real_filename);
}

static void __trim_right(char* buff)
{
	int ix = strlen(buff) - 1;
	while ((ix >= 0) && isspace(buff[ix]))
		buff[ix--] = '\0';
}

static void __parse_aliases(playlist_t dst, const char* value)
{
	struct strarr tmp;
	strarr_split(&tmp, value, " == ");
	for (int i = 0; i < tmp.count; i++) {
		for (int j = i + 1; j < tmp.count; j++) {
			strarr_add(&dst->aliases, tmp.data[i]);
			strarr_add(&dst->aliases, tmp.data[j]);
		}
	}
	strarr_clear(&tmp);
}

static void __parse_tag_spacing(playlist_t dst, const char* value)
{
	struct tag_spacing* new = malloc(sizeof(struct tag_spacing));

	char* sep = strstr(value, " ");
	if (! sep)
		DIE("Malformed setting tag_spacing %s", value);

	*sep = '\0';

	new->tag = strdup(value);
	new->spacing = atoi(sep + 1);
	new->next = dst->tag_spacing;
	dst->tag_spacing = new;
}

static void __read_header(struct playlist* dst, const char* buff)
{
	char* sep = strstr(buff, " ");
	if (! sep)
		DIE("Malformed setting %s", buff);

	strarr_add(&dst->header, buff);

	if (buff[0] == '!')
		return;

	*sep = '\0';
	char* value = sep + 1;

	if (! strcmp(buff, "playlist_id"))
		dst->playlist_id = strdup(value);
	else if (! strcmp(buff, "sort_order"))
		dst->sort_order = strdup(value);
	else if (! strcmp(buff, "same_artist_spacing"))
		dst->same_artist_spacing = atoi(value);
	else if (! strcmp(buff, "bump_offset"))
		dst->bump_offset = atoi(value);
	else if (! strcmp(buff, "bump_spacing"))
		dst->bump_spacing = atoi(value);
	else if (! strcmp(buff, "alias"))
		__parse_aliases(dst, value);
	else if (! strcmp(buff, "tag_spacing"))
		__parse_tag_spacing(dst, value);
	else
		DIE("Unknown setting %s %s", buff, value);
}

static int __read_index(const char* buff)
{
	char* sep = strstr(buff, "(");
	if (sep)
		return atoi(sep + 1);
	else
		return atoi(buff);
}

static bool __read_index_name_artists(struct track* cur, const char* buff)
{
	char* sep = strstr(buff, " :: ");
	if (! sep)
		return false;

	*sep = '\0';

	cur->remote_index = __read_index(buff);

	char* name = sep + 4;
	sep = strstr(name, " :: ");
	if (! sep)
		return false;

	*sep = '\0';
	cur->name = strdup(name);
	strarr_split(&cur->artists, sep + 4, ", ");

	return true;
}

static void __parse_line(const char* line, int line_index, struct playlist* p,
			 struct track* t)
{
	if (line[0] == '\0') {
		if (t->id)
			playlist_add(p, t);
		return;
	}

	if (strncmp(line, "### ", 4) == 0) {
		__read_header(p, line + 4);
		return;
	}

	if (strncmp(line, "# ", 2) == 0) {
		if (t->id)
			fprintf(stderr, "Line %d is bad", line_index);
		else
			t->id = strdup(line + 2);
		return;
	}

	if (strncmp(line, "> ", 2) == 0) {
		if (t->tags.count)
			fprintf(stderr, "Line %d is bad", line_index);
		else
			strarr_split(&t->tags, line + 2, " ");
		return;
	}

	if (__read_index_name_artists(t, line))
		return;

	DIE("Unable to parse line %d: %s", line_index, line);
}

playlist_t fs_read_playlist(const char* filename)
{
	FILE* f = fopen(filename, "r");
	if (! f)
		DIE("Error opening file %s: %m", filename);

	playlist_t ret = playlist_init(NULL);

	char buffer[10240];

	struct track track;
	bzero(&track, sizeof(track));

	int line_count = 0;
	while (fgets(buffer, 10000, f)) {
		line_count++;
		__trim_right(buffer);
		__parse_line(buffer, line_count, ret, &track);
	}

	fclose(f);

	return ret;
}
