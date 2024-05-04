#include <string.h>
#include <strings.h>

#include "spy.h"

struct stats {
	char* tags;
	int count;
	struct stats* next;
};

static void __select(struct strbuff* buff, struct strarr* tags, int option)
{
	for (int i = 0; i < tags->count; i++) {
		int mask = 1 << i;
		if (mask & option) {
			strbuff_addz(buff, tags->data[i]);
			strbuff_addch(buff, ' ');
		}
	}
	buff->wix--;
	strbuff_addch(buff, '\0');
}

static void __increment(struct stats** head_ptr, const char* tags)
{
	struct stats* prev = NULL;
	struct stats* curr = *head_ptr;

	while (curr != NULL) {
		int cmp = strcmp(curr->tags, tags);
		if (cmp == 0) {
			curr->count++;
			return;
		}

		if (cmp > 0)
			break;

		prev = curr;
		curr = curr->next;
	}

	struct stats* new = malloc(sizeof(struct stats));
	new->tags = strdup(tags);
	new->count = 1;
	new->next = curr;

	if (prev == NULL)
		*head_ptr = new;
	else
		prev->next = new;
}

static void __count(struct stats** stptr, track_t t)
{
	struct strarr* tags = &t->tags;
	// strarr_sort(tags);

	int options = 1 << tags->count;

	struct strbuff buff;
	bzero(&buff, sizeof(buff));

	for (int i = options - 1; i < options; i++) {
		__select(&buff, tags, i);
		__increment(stptr, buff.data);
		buff.wix = 0;
	}

	free(buff.data);
}

void cmd_stats(const char* filename)
{
	playlist_t playlist = playlist_read(filename, 0);

	struct stats* st = NULL;

	track_t t = NULL;

	while (playlist_iterate(&t, playlist))
		__count(&st, t);

	struct stats* iter = st;
	int maxlen = 0;

	iter = st;
	while (iter) {
		int len = strlen(iter->tags);
		if (len > maxlen)
			maxlen = len;
		iter = iter->next;
	}

	iter = st;
	while (iter) {
		int len = strlen(iter->tags);

		printf("%s", iter->tags);
		for (int i = len; i < maxlen; i++)
			printf(" ");
		printf(" : %d\n", iter->count);
		iter = iter->next;
	}

	playlist_free(playlist);
}
