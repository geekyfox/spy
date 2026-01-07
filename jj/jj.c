#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jj.h"

#define DIE(fmt, ...)                                                          \
	do {                                                                   \
		fprintf(stderr, "%s: ", __func__);                             \
		fprintf(stderr, fmt, ##__VA_ARGS__);                           \
		fprintf(stderr, "\n");                                         \
		abort();                                                       \
	} while (0)

#define STOP(errcode, fmt, ...)                                                \
	do {                                                                   \
		if (errptr) {                                                  \
			*errptr = errcode;                                     \
		} else {                                                       \
			DIE(fmt, ##__VA_ARGS__);                               \
		}                                                              \
	} while (0)

#define ERRMSG_SZ 20000

enum type {
	TYPE_OBJECT = 42000,
	TYPE_ARRAY = 42001,
	TYPE_STRING = 42002,
	TYPE_NUMBER = 42003,
	TYPE_BOOLEAN = 42004,
	TYPE_NULL = 42005,
	TYPE_TOMBSTONE = 42006,
};

struct object {
	char* key;
	struct jj* value;
	struct object* next;
};

struct array {
	jj_t* data;
	size_t alc;
	size_t count;
};

struct jj {
	enum type typecode;
	union {
		struct object* object;
		struct array* array;
		char* string;
		double number;
		bool boolean;
	} value;
};

struct parser {
	jj_err_t errcode;
	int line;
	int column;
	int prev_line;
	int prev_column;
	//
	FILE* file;
	//
	const char* payload;
	int pl_size;
	int pl_ix;
};

const char* jj_errmsg(jj_err_t code)
{
	switch (code) {
	case JJ_ERR_WRONGTYPE:
		return "Wrong type";
	case JJ_ERR_NOTFOUND:
		return "Not found";
	case JJ_ERR_OUTBOUND:
		return "Array index out of bounds";
	case JJ_ERR_TAKEN:
		return "Value was already taken";
	case JJ_ERR_EARLYEOF:
		return "Premature end of stream";
	case JJ_ERR_BADTOKEN:
		return "Unexpected character";
	case JJ_ERR_LONGSTRING:
		return "String value is too long";
	case JJ_ERR_PASTEOF:
		return "Tokens after the end of payload";
	default:
		return "Something is wrong";
	}
}

static jj_t __parse(struct parser*);
static void __report(struct parser*, struct jj_parse_error* error);

jj_t jj_parse(const char* payload, size_t size, struct jj_parse_error* error)
{
	struct parser p = {.line = 1, .payload = payload, .pl_size = size};

	jj_t ret = __parse(&p);

	if (! ret) {
		size_t x = strlen(payload);
		fprintf(stderr, "strlen = %ld ; size = %ld\n", x, size);
		__report(&p, error);
	}

	return ret;
}

void __report(struct parser* p, struct jj_parse_error* error)
{
	if (error) {
		error->perr_code = p->errcode;
		error->perr_line = p->line;
		error->perr_column = p->column;
		return;
	}

	fprintf(stderr,
		"Parsing failed at line %d, column %d: %s\n",
		p->line,
		p->column,
		jj_errmsg(p->errcode));
	abort();
}

jj_t jj_fparse(FILE* stream, struct jj_parse_error* error)
{
	struct parser p = {.line = 1, .file = stream};
	bzero(&p, sizeof(p));

	p.file = stream;

	jj_t ret = __parse(&p);
	if (! ret)
		__report(&p, error);

	return ret;
}

static void __free(jj_t x);

void jj_free(jj_t x)
{
	if (x)
		__free(x);
	free(x);
}

static void __free(jj_t x)
{
	switch (x->typecode) {
	case TYPE_OBJECT: {
		while (x->value.object) {
			struct object* obj = x->value.object;
			free(obj->key);
			jj_free(obj->value);
			x->value.object = obj->next;
			free(obj);
		}
		break;
	}
	case TYPE_ARRAY: {
		struct array* arr = x->value.array;
		for (int i = 0; i < arr->count; i++)
			jj_free(arr->data[i]);
		free(arr->data);
		free(arr);
		x->value.array = NULL;
		break;
	}
	case TYPE_STRING:
		free(x->value.string);
		x->value.string = NULL;
		break;
	case TYPE_NUMBER:
	case TYPE_BOOLEAN:
	case TYPE_NULL:
	case TYPE_TOMBSTONE:
		break;
	default:
		fprintf(stderr,
			"jj_free(): unexpected typecode=%d\n",
			x->typecode);
		abort();
	}
	x->typecode = TYPE_TOMBSTONE;
}

static jj_t __read(struct parser* p);
static int __next_token(struct parser* p);

static jj_t __parse(struct parser* p)
{
	jj_t ret = __read(p);
	if (! ret)
		return NULL;

	int token = __next_token(p);
	if (token == EOF)
		return ret;

	jj_free(ret);
	p->errcode = JJ_ERR_EARLYEOF;
	return NULL;
}

static jj_t __read_object(struct parser*);
static jj_t __read_array(struct parser*);
static char* __read_string(struct parser*);
static jj_t __read_number(struct parser*, char first);
static bool __expect(struct parser*, const char* exp);
static jj_t __alloc(enum type);
static void __rewind(struct parser*, int token);

static jj_t __read(struct parser* p)
{
	int token = __next_token(p);

	if (token == '{')
		return __read_object(p);

	if (token == '[')
		return __read_array(p);

	if (token == '"') {
		char* s = __read_string(p);
		if (! s)
			return NULL;
		jj_t ret = __alloc(TYPE_STRING);
		ret->value.string = s;
		return ret;
	}

	if (isdigit(token))
		return __read_number(p, token);

	if (token == 't') {
		__rewind(p, token);
		if (! __expect(p, "true"))
			return NULL;
		jj_t ret = __alloc(TYPE_BOOLEAN);
		ret->value.boolean = true;
		return ret;
	}

	if (token == 'f') {
		__rewind(p, token);
		if (! __expect(p, "false"))
			return NULL;
		jj_t ret = __alloc(TYPE_BOOLEAN);
		ret->value.boolean = false;
		return ret;
	}

	if (token == 'n') {
		__rewind(p, token);
		__expect(p, "null");
		return __alloc(TYPE_NULL);
	}

	p->errcode = JJ_ERR_BADTOKEN;
	return NULL;
}

static int __next_char(struct parser* p)
{
	int ch;

	if (p->file)
		ch = fgetc(p->file);
	else if (! p->payload)
		DIE("Invalid parser state");
	else if (p->pl_ix == p->pl_size)
		ch = EOF;
	else
		ch = p->payload[p->pl_ix++];

	p->prev_line = p->line;
	p->prev_column = p->column;

	if (ch == '\n') {
		p->line++;
		p->column = 1;
	} else {
		p->column++;
	}

	return ch;
}

static int __next_token(struct parser* p)
{
	int token;

	do {
		token = __next_char(p);
	} while (isspace(token));

	return token;
}

static jj_t __read_object(struct parser* p)
{
	jj_t ret = __alloc(TYPE_OBJECT);
	char* key = NULL;

	while (true) {
		int token = __next_token(p);

		if (token == EOF) {
			p->errcode = JJ_ERR_EARLYEOF;
			goto fail;
		}

		if (token == '}')
			break;

		if (token == ',')
			continue;

		if (token != '"') {
			p->errcode = JJ_ERR_BADTOKEN;
			goto fail;
		}

		key = __read_string(p);

		if (! key)
			goto fail;

		token = __next_token(p);

		if (token != ':') {
			p->errcode = JJ_ERR_BADTOKEN;
			goto fail;
		}

		jj_t value = __read(p);

		if (! value)
			goto fail;

		struct object* obj = malloc(sizeof(*obj));
		obj->key = key;
		obj->value = value;
		obj->next = ret->value.object;
		ret->value.object = obj;
		key = NULL;
	}

	return ret;

fail:
	jj_free(ret);
	if (key)
		free(key);
	return NULL;
}

static char* __read_string(struct parser* p)
{
	char buffer[10240];
	int offset = 0;
	bool escape = false;

	while (true) {
		if (offset >= 10240) {
			p->errcode = JJ_ERR_LONGSTRING;
			return NULL;
		}

		int ch = __next_char(p);

		if (ch == EOF) {
			p->errcode = JJ_ERR_EARLYEOF;
			return NULL;
		}

		if (escape) {
			if (ch == 'n')
				ch = '\n';
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

static jj_t __alloc(enum type type)
{
	jj_t ret = calloc(1, sizeof(*ret));
	ret->typecode = type;
	return ret;
}

static jj_t __read_number(struct parser* p, char first)
{
	char buffer[10240];
	buffer[0] = first;

	int offset = 1;

	int token = __next_char(p);

	while (isdigit(token) || (token == '.')) {
		buffer[offset++] = token;
		token = __next_char(p);
	}

	__rewind(p, token);
	buffer[offset] = '\0';

	jj_t ret = __alloc(TYPE_NUMBER);
	ret->value.number = atof(buffer);
	return ret;
}

static void __rewind(struct parser* p, int c)
{
	if (c == EOF)
		return;

	if (! p->prev_line)
		DIE("Can't rewind more than one character");

	if (p->file) {
		ungetc(c, p->file);
	} else if (p->pl_ix == 0) {
		DIE("Invalid parser state");
	} else if (p->payload[p->pl_ix - 1] != c) {
		DIE("Invalid argument");
	} else {
		p->pl_ix--;
	}

	p->line = p->prev_line;
	p->prev_line = 0;
	p->column = p->prev_column;
	p->prev_column = 0;
}

static jj_t __read_array(struct parser* p)
{
	struct array* arr = calloc(1, sizeof(*arr));
	jj_t ret = __alloc(TYPE_ARRAY);
	ret->value.array = arr;

	int token = __next_token(p);
	if (token == ']')
		return ret;

	__rewind(p, token);

	while (true) {
		jj_t next = __read(p);

		if (! next) {
			jj_free(ret);
			return NULL;
		}

		if (arr->alc == arr->count) {
			arr->alc += 64;
			arr->data = realloc(arr->data,
					    arr->alc * sizeof(*arr->data));
		}
		arr->data[arr->count] = next;
		arr->count++;

		token = __next_token(p);

		if (token == EOF) {
			jj_free(ret);
			p->errcode = JJ_ERR_EARLYEOF;
			return NULL;
		}

		if (token == ']')
			break;

		if (token != ',') {
			jj_free(ret);
			p->errcode = JJ_ERR_BADTOKEN;
			return NULL;
		}
	}
	return ret;
}

static bool __expect(struct parser* p, const char* exp)
{
	while (*exp) {
		int c = __next_char(p);

		if (c == EOF) {
			p->errcode = JJ_ERR_EARLYEOF;
			return false;
		}

		if (c != *exp) {
			p->errcode = JJ_ERR_BADTOKEN;
			return false;
		}

		exp++;
	}
	return true;
}

jj_t jj_pop(jj_t value, const char* key, jj_err_t* errptr)
{
	if (value->typecode != TYPE_OBJECT) {
		if (errptr) {
			*errptr = JJ_ERR_WRONGTYPE;
			return NULL;
		}
		fprintf(stderr, "jj_pop(): Value is not an object\n");
		abort();
	}

	struct object* obj = value->value.object;
	struct object* prev = NULL;
	struct object* found = NULL;

	while (obj) {
		if (! strcmp(obj->key, key)) {
			found = obj;
			break;
		}

		prev = obj;
		obj = obj->next;
	}

	if (! found) {
		if (errptr) {
			*errptr = JJ_ERR_NOTFOUND;
			return NULL;
		}
		fprintf(stderr,
			"jj_pop(): Entry not found for key '%s'\n",
			key);
		abort();
	}

	if (prev)
		prev->next = obj->next;
	else
		value->value.object = obj->next;

	jj_t ret = obj->value;
	free(obj->key);
	free(obj);

	if (errptr)
		*errptr = JJ_ALLGOOD;

	return ret;
}

jj_t jj_popl(jj_t value, size_t idx, jj_err_t* errptr)
{
	if (value->typecode != TYPE_ARRAY) {
		STOP(JJ_ERR_WRONGTYPE, "Value is not an array");
		return NULL;
	}

	struct array* arr = value->value.array;

	if (idx >= arr->count) {
		STOP(JJ_ERR_OUTBOUND, "Index out of bounds");
		return NULL;
	}

	jj_t result = arr->data[idx];

	if (! result) {
		STOP(JJ_ERR_TAKEN, "Value was already taken");
		return NULL;
	}

	arr->data[idx] = NULL;

	if (errptr)
		*errptr = JJ_ALLGOOD;

	return result;
}

int jj_toint(jj_t value, jj_err_t* errptr)
{
	if (value->typecode != TYPE_NUMBER) {
		STOP(JJ_ERR_WRONGTYPE, "Value is not a number");
		return 0;
	}

	int ret = value->value.number;
	jj_free(value);

	if (errptr)
		*errptr = JJ_ALLGOOD;

	return ret;
}

double jj_tonum(jj_t value, jj_err_t* errptr)
{
	if (value->typecode != TYPE_NUMBER) {
		STOP(JJ_ERR_WRONGTYPE, "Value is not a number");
		return 0;
	}

	double ret = value->value.number;
	jj_free(value);

	if (errptr)
		*errptr = JJ_ALLGOOD;

	return ret;
}

bool jj_isnull(jj_t value)
{
	return (value->typecode == TYPE_NULL);
}

char* jj_tostr(jj_t value, jj_err_t* errptr)
{
	if (value->typecode != TYPE_STRING) {
		STOP(JJ_ERR_WRONGTYPE, "Value is not a string");
		return NULL;
	}

	char* ret = value->value.string;
	free(value);

	if (errptr)
		*errptr = JJ_ALLGOOD;

	return ret;
}

void jj_merge(jj_t x, jj_t y, jj_err_t* errptr)
{
	if (x->typecode != TYPE_ARRAY) {
		STOP(JJ_ERR_WRONGTYPE, "First argument is not a string");
		return;
	}

	if (y->typecode != TYPE_ARRAY) {
		STOP(JJ_ERR_WRONGTYPE, "First argument is not a string");
		return;
	}

	struct array* foo = x->value.array;
	struct array* bar = y->value.array;

	size_t needed = foo->count + bar->count;
	if (foo->alc < needed) {
		foo->alc = needed;
		foo->data = realloc(foo->data, foo->alc * sizeof(*foo->data));
	}

	for (size_t i = 0; i < bar->count; i++)
		foo->data[foo->count++] = bar->data[i];

	free(bar->data);
	free(bar);
	free(y);

	if (errptr)
		*errptr = JJ_ALLGOOD;
}

size_t jj_len(jj_t x, jj_err_t* errptr)
{
	if (x->typecode != TYPE_ARRAY) {
		STOP(JJ_ERR_WRONGTYPE, "Argument is not an array");
		return 0;
	}

	return x->value.array->count;
}
