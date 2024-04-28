#ifndef __SPY_HEADER_FILE__
#define __SPY_HEADER_FILE__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DIE(fmt, ...)                                                          \
	do {                                                                   \
		fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__);               \
		fprintf(stderr, fmt, ##__VA_ARGS__);                           \
		fprintf(stderr, "\n");                                         \
		abort();                                                       \
	} while (0)

/* strbuff.c : dynamic string buffer */

struct strbuff {
	char* data;
	size_t alc;
	size_t wix;
	size_t rix;
	bool stack;
};

void strbuff_add(struct strbuff* buff, const char* ptr, size_t n);
void strbuff_addz(struct strbuff* buff, const char* ptr);
void strbuff_addch(struct strbuff* buff, const char ch);
struct strbuff strbuff_wrap(char*, size_t);

/* json.c */

struct json_value;
typedef struct json_value* json_t;

json_t json_object(void);
void json_put(json_t obj, char* key, json_t value);
json_t json_pop(json_t value, const char* key, bool* flagptr);
char* json_popstr(json_t json, const char* key, bool* flagptr);
double json_popnum(json_t value, const char* key, bool* flagptr);

json_t json_array(void);
void json_push(json_t arr, json_t value);

json_t json_wstr(char* string);
char* json_uwstr(json_t);

json_t json_wnum(double n);

json_t json_wbool(bool);

json_t json_null(void);

void json_free(json_t json);

/* json_parser.c */

json_t json_parse(struct strbuff*);

/* http.c */

struct http_request {
	const char* method;
	const char* url;
	const char* payload;
	bool payload_is_json;
	const char* auth;
};

void http_submit(struct strbuff*, struct http_request);

/* url.c */

void url_encode(struct strbuff*, const char* s);
void url_encode_pair(struct strbuff*, const char* key, const char* value);

/* secrets.c : credentials management */

void secrets_configure(const char* client_id, const char* client_secret);
void secrets_login(const char* code, const char* redirect_url);

/* cmd_login.c */

void cmd_login(void);

#endif
