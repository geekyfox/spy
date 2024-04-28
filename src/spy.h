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

struct json_value;
typedef struct json_value* json_t;

struct strbuff {
	char* data;
	size_t alc;
	size_t wix;
	size_t rix;
	bool stack;
};

struct strarr {
	char** data;
	size_t alc;
	size_t count;
};

struct track {
	char* id;
	char* name;
	struct strarr artists;
	struct strarr tags;
	int remote_index;
};

typedef struct track* track_t;

struct playlist {
	char* playlist_id;
	char* sort_order;
	int same_artist_spacing;
	int bump_offset;
	int bump_spacing;
	struct strarr header;
	struct track* tracks;
	size_t count;
	size_t alc;
	int separator;
};

typedef struct playlist* playlist_t;

/* api.c */

json_t api_get_paginated(const char* path);
playlist_t api_get_playlist(const char* id);

/* fs.c */

struct strbuff fs_read(const char*);
json_t fs_read_json(const char*);
void fs_write_playlist(playlist_t, const char* filename);

/* playlist.c */

void playlist_add(playlist_t, track_t);
playlist_t playlist_init(playlist_t);
void playlist_validate(playlist_t, const char* source);
void playlist_free(playlist_t);

/* strarr.c */

void strarr_add(struct strarr*, const char*);
void strarr_clear(struct strarr*);
void strarr_set(struct strarr* dst, struct strarr* src);

/* strbuff.c : dynamic string buffer */

void strbuff_add(struct strbuff* buff, const char* ptr, size_t n);
void strbuff_addz(struct strbuff* buff, const char* ptr);
void strbuff_addch(struct strbuff* buff, const char ch);
struct strbuff strbuff_wrap(char*, size_t);
char* strbuff_export(struct strbuff*);

/* json.c */

bool json_isnull(json_t);

json_t json_object(void);
void json_put(json_t obj, char* key, json_t value);
json_t json_pop(json_t value, const char* key, bool* flagptr);
char* json_popstr(json_t json, const char* key, bool* flagptr);
double json_popnum(json_t value, const char* key, bool* flagptr);

json_t json_array(void);
void json_push(json_t arr, json_t value);
size_t json_len(json_t);
json_t json_get(json_t arr, size_t index);

json_t json_wstr(char* string);
char* json_uwstr(json_t);

json_t json_wnum(double n);

json_t json_wbool(bool);

json_t json_null(void);
bool json_isnull(json_t);

void json_merge(json_t, json_t);

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

const char* spy_access_token(void);
void secrets_configure(const char* client_id, const char* client_secret);
void secrets_login(const char* code, const char* redirect_url);
const char* secrets_token(void);

/* cmd_fetch.c */

void cmd_fetch(const char* playlist_id, const char* filename);

/* cmd_list.c */

void cmd_list(void);

/* cmd_login.c */

void cmd_login(void);

#endif
