#include <ctype.h>
#include <string.h>

#include "spy.h"

#define FAIL(s)                                                                \
	do {                                                                   \
		char snippet[32];                                              \
		__chop(snippet, s);                                            \
		DIE("Malformed JSON: '%c' %s", s->data[s->rix], snippet);      \
	} while (0)

void __chop(char* snippet, struct strbuff* s)
{
	int a = s->rix - 10;
	if (a < 0)
		a = 0;

	int z = s->rix + 10;
	if (z > s->wix)
		z = s->wix;

	for (int i = a; i < z; i++)
		snippet[i - a] = s->data[i];

	snippet[z - a] = '\0';
}

static json_t __read(struct strbuff* s);
static int __next_token(struct strbuff* s);

json_t json_parse(struct strbuff* payload)
{
	json_t ret = __read(payload);

	int token = __next_token(payload);
	if (token != EOF)
		DIE("Malformed JSON");

	return ret;
}

static int __next_char(struct strbuff* s)
{
	if (s->rix < s->wix - 1)
		return s->data[s->rix++];
	return EOF;
}

static int __next_token(struct strbuff* s)
{
	int token;

	do {
		token = __next_char(s);
	} while (isspace(token));

	return token;
}

static json_t __read_object(struct strbuff* s);
static json_t __read_array(struct strbuff* s);
static json_t __read_number(struct strbuff* s);
static char* __read_string(struct strbuff* s);

static void __expect(struct strbuff* s, const char* exp);

static json_t __read(struct strbuff* s)
{
	int token = __next_token(s);

	if (token == '{') {
		return __read_object(s);
	} else if (token == '[') {
		return __read_array(s);
	} else if (token == '"') {
		char* str = __read_string(s);
		return json_wstr(str);
	} else if (isdigit(token)) {
		s->rix--;
		return __read_number(s);
	} else if (token == 't') {
		s->rix--;
		__expect(s, "true");
		return json_wbool(true);
	} else if (token == 'f') {
		s->rix--;
		__expect(s, "false");
		return json_wbool(false);
	} else if (token == 'n') {
		s->rix--;
		__expect(s, "null");
		return json_null();
	} else {
		FAIL(s);
	}
}

static json_t __read_object(struct strbuff* s)
{
	json_t ret = jsobj_make();

	while (true) {
		int token = __next_token(s);
		if (token == '}')
			break;

		if (token == ',')
			continue;

		if (token != '"')
			FAIL(s);

		char* key = __read_string(s);

		token = __next_token(s);

		if (token != ':')
			DIE("Malformed JSON");

		json_t value = __read(s);
		jsobj_put(ret, key, value);
	}

	return ret;
}

static json_t __read_array(struct strbuff* s)
{
	json_t ret = jsarr_make();

	int token = __next_token(s);
	if (token == ']')
		return ret;

	s->rix--;

	while (true) {
		json_t obj = __read(s);
		jsarr_push(ret, obj);

		token = __next_token(s);

		if (token == ']')
			break;

		if (token == ',')
			continue;

		DIE("Malformed JSON");
	}
	return ret;
}

static char* __read_string(struct strbuff* s)
{
	char buffer[10240];
	int offset = 0;
	bool escape = false;

	while (true) {
		if (offset >= 10240)
			DIE("Buffer overflow");

		int ch = __next_char(s);

		if (ch == EOF)
			DIE("Malformed JSON");

		if (escape) {
			buffer[offset++] = ch;
			escape = false;
			continue;
		}

		if (ch == '"') {
			buffer[offset++] = '\0';
			break;
		}

		if (ch == '\\') {
			escape = true;
			continue;
		}

		buffer[offset++] = ch;
	}

	return strdup(buffer);
}

static json_t __read_number(struct strbuff* s)
{
	char buffer[10240];
	int offset = 0;

	while (true) {
		int token = __next_char(s);
		if (isdigit(token) || (token == '.')) {
			buffer[offset++] = token;
			continue;
		} else {
			s->rix--;
			buffer[offset] = '\0';
			break;
		}
	}

	double n = atof(buffer);
	return json_wnum(n);
}

static void __expect(struct strbuff* s, const char* exp)
{
	while (*exp) {
		char c = s->data[s->rix++];
		if (c != *exp)
			DIE("Unexpected %c vs %c", c, *exp);
		exp++;
	}
}
