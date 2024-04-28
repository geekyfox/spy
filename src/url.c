#include "spy.h"

static const char RESERVED[] = " !\"#$\%&'()*+,/:;=?@[]";

static bool __is_reserved(char ch)
{
	for (int i = sizeof(RESERVED) - 1; i >= 0; i--)
		if (ch == RESERVED[i])
			return true;
	return false;
}

void url_encode(struct strbuff* resp, const char* s)
{
	const char hex[] = "0123456789ABCDEF";

	for (; *s; s++) {
		if (__is_reserved(*s)) {
			strbuff_addch(resp, '%');
			strbuff_addch(resp, hex[*s >> 4]);
			strbuff_addch(resp, hex[*s & 0xF]);
		} else {
			strbuff_addch(resp, *s);
		}
	}
}

void url_encode_pair(struct strbuff* buff, const char* key, const char* value)
{
	if (key[0] == '&') {
		buff->wix--;
		strbuff_addch(buff, '&');
		key++;
	}

	url_encode(buff, key);
	strbuff_addch(buff, '=');
	url_encode(buff, value);
	strbuff_addch(buff, '\0');
}
