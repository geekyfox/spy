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

void strarr_clear(struct strarr* arr)
{
	for (int i = 0; i < arr->count; i++)
		free(arr->data[i]);
	free(arr->data);
	bzero(arr, sizeof(*arr));
}
