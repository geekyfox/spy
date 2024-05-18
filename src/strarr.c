#include <string.h>
#include <strings.h>
#include <time.h>

#include "spy.h"

void strarr_add(struct strarr* arr, const char* s)
{
	if (arr->alc == arr->count) {
		arr->alc = arr->alc * 2 + 8;
		arr->data = realloc(arr->data, sizeof(char*) * arr->alc);
	}
	arr->data[arr->count++] = strdup(s);
}

void strarr_set(struct strarr* dst, struct strarr* src)
{
	for (int i = 0; i < dst->count; i++)
		free(dst->data[i]);

	dst->count = 0;

	for (int i = 0; i < src->count; i++)
		strarr_add(dst, src->data[i]);
}

void strarr_move(struct strarr* dst, struct strarr* src)
{
	for (int i = 0; i < dst->count; i++)
		free(dst->data[i]);
	free(dst->data);
	*dst = *src;
	bzero(src, sizeof(*src));
}

void strarr_clear(struct strarr* arr)
{
	for (int i = 0; i < arr->count; i++)
		free(arr->data[i]);
	free(arr->data);
	bzero(arr, sizeof(*arr));
}

bool strarr_has(struct strarr* arr, const char* s)
{
	return strarr_seek(arr, s) >= 0;
}

int strarr_seek(struct strarr* arr, const char* s)
{
	for (int i = 0; i < arr->count; i++)
		if (strcmp(arr->data[i], s) == 0)
			return i;

	return -1;
}

void strarr_split(struct strarr* ret, const char* text, const char* sep)
{
	char *temp, *scan, *next;
	size_t seplen;

	bzero(ret, sizeof(*ret));
	temp = strdup(text);
	scan = temp;
	seplen = strlen(sep);

	while ((next = strstr(scan, sep))) {
		*next = '\0';
		strarr_add(ret, scan);
		scan = next + seplen;
	}

	strarr_add(ret, scan);

	free(temp);
}

static void __swap(struct strarr* arr, int a, int b)
{
	char* t = arr->data[a];
	arr->data[a] = arr->data[b];
	arr->data[b] = t;
}

void strarr_shift(struct strarr* arr, int from, int to)
{
	while (from > to) {
		__swap(arr, from, from - 1);
		from--;
	}
	while (from < to) {
		__swap(arr, from, from + 1);
		from++;
	}
}

void strarr_shuffle(struct strarr* arr, int picks)
{
	srandom(time(NULL));

	if (picks <= 0)
		picks = arr->count - 1;

	for (int i = 0; i < picks; i++) {
		int avail = arr->count - i;
		int pick = random() % avail;
		if (pick != 0)
			strarr_shift(arr, pick + i, i);
	}
}

static int __compare(const void* px, const void* py)
{
	return strcmp(*(const char**)px, *(const char**)py);
}

void strarr_sort(struct strarr* arr)
{
	qsort(arr->data, arr->count, sizeof(char*), __compare);
}
