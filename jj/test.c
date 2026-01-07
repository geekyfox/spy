#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jj.h"

void test_happy_flow(void);
void test_in_memory_parsing(void);
void test_merge(void);

int main()
{
	test_happy_flow();
	test_in_memory_parsing();
	test_merge();
	printf("All good!\n");
}

#define ASSERT(a) __assert_impl(__FILE__, __LINE__, (a))

void __assert_impl(const char* file, int line, bool x)
{
	if (x)
		return;

	fprintf(stderr, "Assertion failed @ %s:%d\n", file, line);
	abort();
}

#define ASSERT_SAME_INT(a, b)                                                  \
	__assert_same_int_impl(__FILE__, __LINE__, (a), (b))

void __assert_same_int_impl(const char* file, int line, int x, int y)
{
	if (x == y)
		return;

	fprintf(stderr, "Assertion failed @ %s:%d", file, line);
	fprintf(stderr, " : %d vs %d\n", x, y);
	abort();
}

#define ASSERT_SAME_STRING(a, b)                                               \
	__assert_same_string_impl(__FILE__, __LINE__, (a), (b))

void __assert_same_string_impl(const char* file, int line, const char* x,
			       const char* y)
{
	if (! strcmp(x, y))
		return;

	fprintf(stderr, "Assertion failed @ %s:%d", file, line);
	fprintf(stderr, " : %s vs %s\n", x, y);
	abort();
}

void test_happy_flow(void)
{
	FILE* f = fopen("testdata/happy.json", "r");
	if (! f) {
		perror("Failed to fopen testdata/happy.json");
		abort();
	}

	jj_t ret = jj_fparse(f, NULL);

	if (fclose(f)) {
		perror("Failed to fclose testdata/happy.json");
		abort();
	}

	jj_t num = jj_pop(ret, "some_number", NULL);
	int v = jj_toint(num, NULL);
	ASSERT_SAME_INT(v, 42);

	jj_err_t errcode;
	jj_pop(ret, "some_number", &errcode);
	ASSERT_SAME_INT(errcode, JJ_ERR_NOTFOUND);

	jj_toint(ret, &errcode);
	ASSERT_SAME_INT(errcode, JJ_ERR_WRONGTYPE);

	jj_t stuff = jj_pop(ret, "stuff", NULL);
	ASSERT(jj_isnull(stuff));
	jj_free(stuff);

	jj_t obj = jj_pop(ret, "object", NULL);
	jj_t key = jj_pop(obj, "key", NULL);
	jj_free(obj);

	char* strkey = jj_tostr(key, NULL);
	ASSERT_SAME_STRING(strkey, "value");
	free(strkey);

	jj_free(ret);
}

void test_in_memory_parsing(void)
{
	const char* data = "[{\"foo\": 1}, 2, \"bar\", null]";
	jj_t ret = jj_parse(data, strlen(data), NULL);

	jj_t foo = jj_popl(ret, 1, NULL);
	int v = jj_toint(foo, NULL);
	ASSERT_SAME_INT(v, 2);

	jj_free(ret);
}

void test_merge(void)
{
	const char* data = "[1, 2, 3]";
	jj_t foo = jj_parse(data, strlen(data), NULL);
	data = "[4, 5, 6]";
	jj_t bar = jj_parse(data, strlen(data), NULL);

	jj_merge(foo, bar, NULL);
	int v = jj_len(foo, NULL);
	ASSERT_SAME_INT(v, 6);

	bar = jj_popl(foo, 4, NULL);
	v = jj_toint(bar, NULL);
	ASSERT_SAME_INT(v, 5);

	jj_free(foo);
}
