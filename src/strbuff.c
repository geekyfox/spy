#include <stdlib.h>
#include <strings.h>

#include "spy.h"

static inline void __reserve(struct strbuff* buff, size_t wanted)
{
	size_t total = buff->wix + wanted;

	if (total <= buff->alc)
		return;

	if (buff->stack)
		DIE("Buffer overflow");

	buff->alc += total + 32;
	buff->data = realloc(buff->data, sizeof(char) * buff->alc);
}

struct strbuff strbuff_wrap(char* ptr, size_t size)
{
	struct strbuff result = {
		.data = ptr, .alc = size, .wix = 0, .rix = 0, .stack = true};

	return result;
}

void strbuff_addch(struct strbuff* buff, char ch)
{
	__reserve(buff, 1);
	buff->data[buff->wix++] = ch;
}

void strbuff_add(struct strbuff* buff, const char* ptr, size_t nmemb)
{
	__reserve(buff, nmemb);
	for (int i = 0; i < nmemb; i++)
		buff->data[buff->wix++] = ptr[i];
}

void strbuff_addz(struct strbuff* buff, const char* ptr)
{
	for (int i = 0; ptr[i]; i++)
		strbuff_addch(buff, ptr[i]);
}
