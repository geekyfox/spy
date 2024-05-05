#include <ctype.h>
#include <string.h>

#include "spy.h"

enum json_type {
	JSON_OBJECT = 42001,
	JSON_STRING = 42002,
	JSON_NUMBER = 42003,
	JSON_ARRAY = 42004,
	JSON_BOOLEAN = 42005,
	JSON_NULL = 42006
};

struct json_value {
	enum json_type typecode;
	union {
		struct json_object* object;
		struct json_array* array;
		char* string;
		double number;
		bool boolean;
	} value;
};

struct json_object {
	char* key;
	struct json_value* value;
	struct json_object* next;
};

struct json_array {
	json_t* data;
	size_t alc;
	size_t count;
};

#define ASSERT_TYPE(var, code)                                                 \
	do {                                                                   \
		if (var->typecode != code)                                     \
			DIE("Expected %s, got %d", #code, var->typecode);      \
	} while (0)

static json_t __make(int typecode)
{
	json_t ret = malloc(sizeof(struct json_value));
	bzero(ret, sizeof(*ret));
	ret->typecode = typecode;
	return ret;
}

json_t jsobj_make()
{
	return __make(JSON_OBJECT);
}

void jsobj_put(json_t obj, char* key, json_t value)
{
	ASSERT_TYPE(obj, JSON_OBJECT);

	struct json_object* new_obj = malloc(sizeof(*new_obj));

	new_obj->next = obj->value.object;
	new_obj->key = key;
	new_obj->value = value;

	obj->value.object = new_obj;
}

json_t jsobj_pop(json_t value, const char* key, bool* flagptr)
{
	ASSERT_TYPE(value, JSON_OBJECT);

	struct json_object* obj = value->value.object;
	struct json_object* prev = NULL;

	while (obj) {
		if (strcmp(obj->key, key) != 0) {
			prev = obj;
			obj = obj->next;
			continue;
		}

		if (prev == NULL)
			value->value.object = obj->next;
		else
			prev->next = obj->next;

		json_t ret = obj->value;
		free(obj->key);
		free(obj);

		if (flagptr)
			*flagptr = true;

		return ret;
	}

	if (flagptr) {
		*flagptr = false;
		return NULL;
	}
	DIE("Mandatory field '%s' not found", key);
}

char* jsobj_popstr(json_t value, const char* key, bool* flagptr)
{
	json_t item = jsobj_pop(value, key, flagptr);
	if (! item)
		return NULL;

	char* ret = json_uwstr(item);
	free(item);
	return ret;
}

double jsobj_popnum(json_t value, const char* key, bool* flagptr)
{
	json_t item = jsobj_pop(value, key, flagptr);
	if (! item)
		return 0;

	ASSERT_TYPE(item, JSON_NUMBER);

	double ret = item->value.number;
	free(item);
	return ret;
}

json_t jsobj_get(json_t value, const char* key, bool* flagptr)
{
	ASSERT_TYPE(value, JSON_OBJECT);

	struct json_object* obj = value->value.object;

	while (obj) {
		if (! strcmp(obj->key, key))
			return obj->value;
		obj = obj->next;
	}

	if (flagptr) {
		*flagptr = false;
		return NULL;
	}

	DIE("Mandatory field '%s' not found", key);
}

char* jsobj_getstr(json_t value, const char* key, bool* flagptr)
{
	json_t item = jsobj_get(value, key, flagptr);
	if (! item)
		return NULL;

	ASSERT_TYPE(item, JSON_STRING);

	return item->value.string;
}

json_t jsarr_make(void)
{
	json_t ret = malloc(sizeof(struct json_value));
	ret->typecode = JSON_ARRAY;
	ret->value.array = malloc(sizeof(struct json_array));
	bzero(ret->value.array, sizeof(struct json_array));
	return ret;
}

void jsarr_push(json_t value, json_t obj)
{
	ASSERT_TYPE(value, JSON_ARRAY);

	struct json_array* arr = value->value.array;

	if (arr->alc == arr->count) {
		arr->alc = arr->alc * 2 + 8;
		arr->data = realloc(arr->data, sizeof(json_t) * arr->alc);
	}
	arr->data[arr->count++] = obj;
}

size_t jsarr_len(json_t arr)
{
	ASSERT_TYPE(arr, JSON_ARRAY);
	return arr->value.array->count;
}

json_t jsarr_get(json_t arr, size_t index)
{
	ASSERT_TYPE(arr, JSON_ARRAY);
	return arr->value.array->data[index];
}

json_t json_wstr(char* string)
{
	json_t ret = __make(JSON_STRING);
	ret->value.string = string;
	return ret;
}

char* json_uwstr(json_t x)
{
	ASSERT_TYPE(x, JSON_STRING);

	char* result = x->value.string;

	x->typecode = JSON_NULL;
	x->value.string = NULL;

	return result;
}

json_t json_wnum(double n)
{
	json_t ret = __make(JSON_NUMBER);
	ret->value.number = n;
	return ret;
}

json_t json_wbool(bool x)
{
	json_t ret = __make(JSON_BOOLEAN);
	ret->value.boolean = x;
	return ret;
}

json_t json_null()
{
	return __make(JSON_NULL);
}

bool json_isnull(json_t x)
{
	return x->typecode == JSON_NULL;
}

static void __reset_array(struct json_array* arr)
{
	if (arr->data) {
		for (int i = 0; i < arr->count; i++)
			json_free(arr->data[i]);
		free(arr->data);
	}
	free(arr);
}

static void __reset(json_t x)
{
	switch (x->typecode) {
	case JSON_ARRAY:
		__reset_array(x->value.array);
		x->value.array = NULL;
		break;
	case JSON_OBJECT:
		while (x->value.object) {
			struct json_object* obj = x->value.object;
			free(obj->key);
			json_free(obj->value);
			x->value.object = obj->next;
			free(obj);
		}
		break;
	case JSON_STRING:
		free(x->value.string);
		x->value.string = NULL;
		break;
	case JSON_NUMBER:
	case JSON_BOOLEAN:
	case JSON_NULL:
		break;
	default:
		DIE("Unexpected %d", x->typecode);
	}
	x->typecode = JSON_NULL;
}

void json_free(json_t json)
{
	__reset(json);
	free(json);
}

void json_merge(json_t dst, json_t src)
{
	if ((dst->typecode == JSON_ARRAY) && (src->typecode == JSON_ARRAY)) {
		struct json_array* src_arr = src->value.array;

		for (int i = 0; i < src_arr->count; i++)
			jsarr_push(dst, src_arr->data[i]);
		src_arr->count = 0;

		__reset(src);
	} else {
		DIE("Don't know how to merge %d into %d",
		    src->typecode,
		    dst->typecode);
	}
}
