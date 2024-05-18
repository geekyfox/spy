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
	struct strarr aliases;
};

typedef struct playlist* playlist_t;

/* api.c */

json_t api_get_paginated(const char* path);
playlist_t api_get_playlist(const char* id);
void api_reorder(const char* playlist_id, int start, int before, int length);
void api_add_tracks(const char* playlist_id, const struct strarr* tracks);
void api_remove_tracks(const char* playlist_id, const struct strarr* track_ids);

/* fs.c */

struct strbuff fs_read(const char*);
json_t fs_read_json(const char*);
void fs_write_playlist(playlist_t, const char* filename);
playlist_t fs_read_playlist(const char* filename);

/* playlist.c */

void playlist_add(playlist_t, track_t);
playlist_t playlist_init(playlist_t);
bool playlist_iterate(track_t* tptr, playlist_t p);
track_t playlist_lookup(playlist_t p, const char* track_id);
playlist_t playlist_read(const char* filename, int flags);
void playlist_pack(playlist_t);
void playlist_free(playlist_t);

/* strarr.c */

void strarr_add(struct strarr*, const char*);
void strarr_set(struct strarr* dst, struct strarr* src);
void strarr_move(struct strarr* dst, struct strarr* src);
bool strarr_has(struct strarr* arr, const char* s);
int strarr_seek(struct strarr* arr, const char* s);
void strarr_shuffle(struct strarr* arr, int picks);
void strarr_split(struct strarr* ret, const char* text, const char* sep);
void strarr_clear(struct strarr*);
void strarr_shift(struct strarr* arr, int from, int to);
void strarr_sort(struct strarr* arr);

/* strbuff.c : dynamic string buffer */

void strbuff_add(struct strbuff* buff, const char* ptr, size_t n);
void strbuff_addz(struct strbuff* buff, const char* ptr);
void strbuff_addch(struct strbuff* buff, const char ch);
struct strbuff strbuff_wrap(char*, size_t);
char* strbuff_export(struct strbuff*);

/* track.c */

void track_add_tag(track_t track, const char* tag);
bool track_has_tag(track_t track, const char* tag);
bool track_remove_tag(track_t track, const char* tag);
void track_clear(track_t);

/* json.c */

bool json_isnull(json_t);

json_t jsobj_make(void);
void jsobj_put(json_t obj, char* key, json_t value);
json_t jsobj_pop(json_t value, const char* key, bool* flagptr);
char* jsobj_popstr(json_t json, const char* key, bool* flagptr);
double jsobj_popnum(json_t value, const char* key, bool* flagptr);
json_t jsobj_get(json_t json, const char* key, bool* flagptr);
char* jsobj_getstr(json_t json, const char* key, bool* flagptr);

json_t jsarr_make(void);
void jsarr_push(json_t arr, json_t value);
size_t jsarr_len(json_t);
json_t jsarr_get(json_t arr, size_t index);

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

/* secrets.c : credentials management */

const char* spy_access_token(void);
void secrets_configure(const char* client_id, const char* client_secret);
void secrets_login(const char* code, const char* redirect_url);
const char* secrets_token(void);

/* url.c */

void url_encode(struct strbuff*, const char* s);
void url_encode_pair(struct strbuff*, const char* key, const char* value);

/* validate.c */

#define VF_DEFAULT 0
#define VF_SKIP_UNIQUE 1
#define VF_PLAYLIST_ID 2

void validate_playlist(playlist_t, const char* source, int flags);

/* cmd_add_all.c */

void cmd_add_all(const char* src_filename, const char* dst_filename);

/* cmd_clear.c */

void cmd_clear(const char* filename);

/* cmd_fetch.c */

void cmd_fetch(const char* playlist_id, const char* filename);

/* cmd_filter.c */

void cmd_filter(const char* tag, const char* filename);

/* cmd_fix.c */

void cmd_fix(const char* filename);

/* cmd_list.c */

void cmd_list(void);

/* cmd_log.c */

enum log_mode {
	LOG_MODE_DUMP_UNTAGGED = 43001,
	LOG_MODE_BUMP_UNTAGGED = 43002,
};

void cmd_log(const char* filename, enum log_mode);

/* cmd_login.c */

void cmd_login(void);

/* cmd_pull.c */

enum pull_mode {
	PULL_MODE_REMOTE_ORDER = 44001,
	PULL_MODE_LOCAL_ORDER = 44002,
};

void cmd_pull(const char* filename, enum pull_mode);

/* cmd_push.c */

void cmd_push(const char* filename, bool dryrun);

/* cmd_shuffle.c */

void cmd_shuffle(const char* filename, int limit);

/* cmd_sort.c */

enum sort_mode {
	SORT_MODE_DEFAULT = 45001,
	SORT_MODE_RACE = 45002,
};

void cmd_sort(const char* filename, enum sort_mode);

/* cmd_stats.c */

void cmd_stats(const char* filename);

/* cmd_sync_tags.c */

void cmd_sync_tags(const char* src, const char* dst);

#endif
