#include <ctype.h>

#include "spy.h"

struct parser {
	playlist_t result;
	const char* path;
	int line_index;
	struct track track;
	char buffer[10240];
};

static void __trim_right(char*);
static void __parse_line(struct parser*);

playlist_t parse_playlist(FILE* file, const char* path)
{
	struct parser p = {
		.result = playlist_init(NULL),
		.path = path,
		.line_index = 0,
	};

	bzero(&p.track, sizeof(p.track));

	while (fgets(p.buffer, 10000, file)) {
		p.line_index++;
		__trim_right(p.buffer);
		__parse_line(&p);
	}

	return p.result;
}

static void __trim_right(char* buff)
{
	int ix = strlen(buff) - 1;
	while ((ix >= 0) && isspace(buff[ix]))
		buff[ix--] = '\0';
}

static void __halt(struct parser* parser);
static bool __parse_header(struct parser* parser);
static bool __read_track(struct track* t, const char* line);
static char* __match_prefix(char* line, char* prefix);

static void __parse_line(struct parser* parser)
{
	playlist_t p = parser->result;
	track_t t = &parser->track;
	char* buf = parser->buffer;
	char* suf;

	if (buf[0] == '\0') {
		if (t->id)
			playlist_add(p, t);
		return;
	}

	if (__parse_header(parser))
		return;

	if ((buf[0] == '[') && __read_track(t, buf)) {
		return;
	}

	if ((suf = __match_prefix(buf, "~"))) {
		if (! t->id) {
			t->id = strdup(suf);
			return;
		}
		if (! t->tags.count) {
			strarr_split(&t->tags, suf, " ");
			return;
		}
	}

	__halt(parser);
}

static void __halt(struct parser* parser)
{
	fprintf(stderr,
		"Failed to parse %s: Line %d is invalid: '%s'\n",
		parser->path,
		parser->line_index,
		parser->buffer);
	exit(1);
}

static void __parse_aliases(playlist_t dst, const char* value);
static void __parse_tag_gap(struct parser* parser, const char* value);

static bool __parse_header(struct parser* parser)
{
	playlist_t p = parser->result;
	char* buf = parser->buffer;
	char* suf;

	if ((suf = __match_prefix(buf, "playlist_id ="))) {
		free(p->playlist_id);
		p->playlist_id = strdup(suf);
		return true;
	}

	if ((suf = __match_prefix(buf, "sort_order ="))) {
		free(p->sort_order);
		p->sort_order = strdup(suf);
		return true;
	}

	if ((suf = __match_prefix(buf, "spacing ="))) {
		p->spacing = atoi(suf);
		return true;
	}

	if ((suf = __match_prefix(buf, "bump_offset ="))) {
		p->bump_offset = atoi(suf);
		return true;
	}

	if ((suf = __match_prefix(buf, "bump_spacing ="))) {
		p->bump_spacing = atoi(suf);
		return true;
	}

	if ((suf = __match_prefix(buf, "aliases +="))) {
		__parse_aliases(p, suf);
		return true;
	}

	if ((suf = __match_prefix(buf, "tag_gaps +="))) {
		__parse_tag_gap(parser, suf);
		return true;
	}

	return false;
}

static char* __match_prefix(char* line, char* prefix)
{
	char* x = line;
	char* y = prefix;

	while (*y) {
		if (*y != *x)
			return NULL;
		x++;
		y++;
	}

	while (isspace(*x))
		x++;

	return x;
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

static void __parse_tag_gap(struct parser* parser, const char* value)
{
	char* sep = strstr(value, " ");
	if (! sep)
		__halt(parser);

	*sep = '\0';

	struct tag_spacing* new = malloc(sizeof(struct tag_spacing));
	new->tag = strdup(value);
	new->spacing = atoi(sep + 1);
	new->next = NULL;

	playlist_t p = parser->result;

	struct tag_spacing* ptr = p->tag_spacing;
	if (! ptr) {
		p->tag_spacing = new;
		return;
	}

	while (ptr->next != NULL)
		ptr = ptr->next;

	ptr->next = new;
}

static bool __read_track(struct track* t, const char* line)
{
	char* foo = strstr(line, "] ");
	char* bar = strstr(line, " - ");
	char* baz = strstr(line, " | ");

	if ((! foo) || (! bar) || (bar < foo))
		return false;

	if ((! baz) || (baz > foo)) {
		t->remote_index = atoi(line + 1);
	} else {
		t->remote_index = atoi(baz + 3);
	}

	t->name = strdup(bar + 3);
	bar[0] = '\0';
	strarr_split(&t->artists, foo + 2, ", ");
	return true;
}
