#ifndef __SPY_HEADER_FILE__
#define __SPY_HEADER_FILE__

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../jj/jj.h"

#define DIE(fmt, ...)                                                          \
	do {                                                                   \
		fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__);               \
		fprintf(stderr, fmt, ##__VA_ARGS__);                           \
		fprintf(stderr, "\n");                                         \
		abort();                                                       \
	} while (0)

#define HALT(fmt, ...)                                                         \
	do {                                                                   \
		char* err = strerror(errno);                                   \
		fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__);               \
		fprintf(stderr, fmt, ##__VA_ARGS__);                           \
		fprintf(stderr, ": %s\n", err);                                \
		exit(1);                                                       \
	} while (0)

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

struct tag_spacing {
	char* tag;
	int spacing;
	struct tag_spacing* next;
};

struct playlist {
	char* playlist_id;
	char* sort_order;
	int spacing;
	int bump_offset;
	int bump_spacing;
	struct track* tracks;
	size_t count;
	size_t alc;
	struct strarr aliases;
	struct tag_spacing* tag_spacing;
};

typedef struct playlist* playlist_t;

/* api.c */

struct reorder_move {
	int range_start;
	int insert_before;
	int range_length;
};

jj_t api_get_paginated(const char* path);
playlist_t api_get_playlist(const char* id);
void api_reorder(const char* playlist_id, struct reorder_move);
void api_add_tracks(const char* playlist_id, const struct strarr* tracks);
void api_remove_tracks(const char* playlist_id, const struct strarr* track_ids);
char* api_create_playlist(const char* filename);

/* fs.c */

struct strbuff fs_read(const char*);
jj_t fs_read_json(const char*);
void fs_write_playlist(playlist_t, const char* filename);
playlist_t fs_read_playlist(const char* filename);

/* parse.c */

playlist_t parse_playlist(FILE*, const char* filename);

/* playlist.c */

void playlist_add(playlist_t, track_t);
playlist_t playlist_init(playlist_t);
bool playlist_iterate(track_t* tptr, playlist_t p);
track_t playlist_lookup(playlist_t p, const char* track_id);
playlist_t playlist_read(const char* filename, int flags);
void playlist_pack(playlist_t);
int playlist_cutoff(playlist_t);
void playlist_free(playlist_t);

/* strarr.c */

void strarr_add(struct strarr*, const char*);
void strarr_adopt(struct strarr*, char*);
void strarr_set(struct strarr* dst, struct strarr* src);
void strarr_move(struct strarr* dst, struct strarr* src);
bool strarr_has(struct strarr* arr, const char* s);
int strarr_seek(struct strarr* arr, const char* s);
void strarr_shuffle(struct strarr* arr, int picks);
void strarr_split(struct strarr* ret, const char* text, const char* sep);
void strarr_clear(struct strarr*);
void strarr_shift(struct strarr* arr, int from, int to);
void strarr_sort(struct strarr* arr);
char* strarr_pop(struct strarr* arr);

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
void track_move(track_t dst, track_t src);
void track_clear(track_t);

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

/* write.c */

void write_playlist(playlist_t p, FILE* f);

/* spy.c */

bool args_flag(const char*);
int args_flagx(const char**, size_t);
const char* args_popopt(void);
const char* args_pop(void);
const char* args_poplast(void);
bool args_popnext(const char**);
void args_finish(void);
int args_atoi(const char*);
void args_abort(void);

/* cmd_clear.c */

extern const char CMD_CLEAR_USAGE[];
void cmd_clear(void);

/* cmd_clone.c */

extern const char CMD_CLONE_USAGE[];
void cmd_clone(void);

/* cmd_drop.c */

extern const char CMD_DROP_USAGE[];
void cmd_drop(void);

/* cmd_fetch.c */

extern const char CMD_FETCH_USAGE[];
void cmd_fetch(void);

/* cmd_filter.c */

extern const char CMD_FILTER_USAGE[];
void cmd_filter(void);

/* cmd_fix.c */

extern const char CMD_FIX_USAGE[];
void cmd_fix(void);

/* cmd_list.c */

extern const char CMD_LIST_USAGE[];
void cmd_list(void);

/* cmd_log.c */

extern const char CMD_LOG_USAGE[];
void cmd_log(void);

/* cmd_login.c */

extern const char CMD_LOGIN_USAGE[];
void cmd_login(void);

/* cmd_or.c */

extern const char CMD_OR_USAGE[];
void cmd_or(void);

/* cmd_pull.c */

extern const char CMD_PULL_USAGE[];
void cmd_pull(void);

/* cmd_push.c */

extern const char CMD_PUSH_USAGE[];
void cmd_push(void);

/* cmd_reverse.c */

extern const char CMD_REVERSE_USAGE[];
void cmd_reverse(void);

/* cmd_shuffle.c */

extern const char CMD_SHUFFLE_USAGE[];
void cmd_shuffle(void);

/* cmd_sort.c */

extern const char CMD_SORT_USAGE[];
void cmd_sort(void);

/* cmd_stats.c */

extern const char CMD_STATS_USAGE[];
void cmd_stats(void);

/* cmd_tag.c */

extern const char CMD_TAG_USAGE[];
void cmd_tag(void);

/* cmd_take.c */

extern const char CMD_TAKE_USAGE[];
void cmd_take(void);

/* cmd_xor.c */

extern const char CMD_XOR_USAGE[];
void cmd_xor(void);

#endif
