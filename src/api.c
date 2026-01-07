#include <ctype.h>
#include <string.h>

#include "spy.h"

static void __make_url(char* url, const char* path)
{
	if (strncmp(path, "https://", 8) == 0)
		strcpy(url, path);
	else
		sprintf(url, "https://api.spotify.com/v1%s", path);
}

static void __submit(struct strbuff* ret, const char* method, const char* path,
		     const char* body)
{
	char url[10240], auth[10240];
	__make_url(url, path);
	sprintf(auth, "Authorization: Bearer %s", secrets_token());

	struct http_request req = {
		.method = method,
		.url = url,
		.payload = body,
		.payload_is_json = true,
		.auth = auth,
	};

	http_submit(ret, req);
}

static jj_t __get(const char* path)
{
	struct strbuff buff;
	__submit(&buff, "GET", path, NULL);
	jj_t ret = jj_parse(buff.data, buff.wix - 1, NULL);
	free(buff.data);
	return ret;
}

jj_t api_get_paginated(const char* path)
{
	jj_t ret = NULL;
	char* url = strdup(path);

	while (url) {
		jj_t resp = __get(url);
		free(url);

		jj_t page = jj_pop(resp, "items", NULL);
		jj_t next = jj_pop(resp, "next", NULL);
		jj_free(resp);

		if (! ret)
			ret = page;
		else
			jj_merge(ret, page, NULL);

		if (jj_isnull(next)) {
			jj_free(next);
			url = NULL;
		} else {
			url = jj_tostr(next, NULL);
		}
	}

	return ret;
}

static char* __pop_name(jj_t obj)
{
	jj_t tmp = jj_pop(obj, "name", NULL);
	char* name = jj_tostr(tmp, NULL);
	if (strcmp(name, ""))
		return name;

	free(name);
	return strdup("N/A");
}

void __digest_track(track_t ret, jj_t track)
{
	bzero(ret, sizeof(*ret));

	jj_t id = jj_pop(track, "id", NULL);
	ret->id = jj_tostr(id, NULL);
	ret->name = __pop_name(track);

	jj_t artists = jj_pop(track, "artists", NULL);

	int count = jj_len(artists, NULL);
	for (int i = 0; i < count; i++) {
		jj_t artist = jj_popl(artists, i, NULL);
		char* name = __pop_name(artist);
		jj_free(artist);

		strarr_add(&ret->artists, name);
		free(name);
	}

	jj_free(artists);
}

playlist_t api_get_playlist(const char* id)
{
	playlist_t ret = playlist_init(NULL);
	ret->playlist_id = strdup(id);

	char path[10240];
	sprintf(path, "/playlists/%s/tracks", id);
	jj_t resp = api_get_paginated(path);

	int count = jj_len(resp, NULL);

	for (int i = 0; i < count; i++) {
		jj_t blob = jj_popl(resp, i, NULL);
		jj_t track = jj_pop(blob, "track", NULL);
		jj_free(blob);

		struct track tr;
		__digest_track(&tr, track);
		jj_free(track);

		tr.remote_index = i + 1;
		playlist_add(ret, &tr);
	}

	jj_free(resp);

	validate_playlist(ret, path, 0);

	return ret;
}

static void __post(struct strbuff* res, const char* path, const char* body)
{
	__submit(res, "POST", path, body);
}

static void __delete(const char* path, const char* body)
{
	__submit(NULL, "DELETE", path, body);
}

static void __put(const char* path, const char* body)
{
	__submit(NULL, "PUT", path, body);
}

void api_reorder(const char* playlist_id, struct reorder_move move)
{
	char path[1024], content[10240];
	sprintf(path, "/playlists/%s/tracks", playlist_id);
	sprintf(content,
		"{\"range_start\":%d,\"insert_before\":%d,\"range_length\":%d}",
		move.range_start,
		move.insert_before,
		move.range_length);

	__put(path, content);
}

void api_add_tracks(const char* playlist_id, const struct strarr* tracks)
{
	char path[10240];
	sprintf(path, "/playlists/%s/tracks", playlist_id);

	int first = 0;

	struct strbuff req;
	bzero(&req, sizeof(req));

	while (first < tracks->count) {
		int last = first + 50;
		if (last > tracks->count)
			last = tracks->count;
		strbuff_addz(&req, "{\"uris\":[");

		for (int i = first; i < last; i++) {
			if (i != first)
				strbuff_addch(&req, ',');
			strbuff_addz(&req, "\"spotify:track:");
			strbuff_addz(&req, tracks->data[i]);
			strbuff_addch(&req, '"');
		}
		strbuff_addz(&req, "]}");
		strbuff_addch(&req, 0);
		__post(NULL, path, req.data);
		req.wix = 0;
		first = last;
	}

	free(req.data);
}

static void __make_remove_request(struct strbuff* req, struct strarr* tids)
{
	int count = 0;
	req->wix = 0;

	strbuff_addz(req, "{\"tracks\":[");

	while (count < 50) {
		const char* tid = strarr_pop(tids);
		if (! tid)
			break;

		if (count != 0)
			strbuff_addch(req, ',');
		count++;

		strbuff_addz(req, "{\"uri\":\"spotify:track:");
		strbuff_addz(req, tid);
		strbuff_addz(req, "\"}");
	}

	strbuff_addz(req, "]}\0");
}

void api_remove_tracks(const char* playlist_id, const struct strarr* track_ids)
{
	char path[10240];
	sprintf(path, "/playlists/%s/tracks", playlist_id);

	struct strbuff req;
	bzero(&req, sizeof(req));

	struct strarr tids = *track_ids;

	while (tids.count > 0) {
		__make_remove_request(&req, &tids);
		__delete(path, req.data);
	}

	free(req.data);
}

char* api_get_user_id(void)
{
	jj_t resp = __get("/me");
	jj_t user_id = jj_pop(resp, "id", NULL);
	jj_free(resp);

	return jj_tostr(user_id, NULL);
}

static char __sanitize(char c)
{
	if (c == '"')
		return '\'';
	if (c == '\\')
		return 0;
	if (isspace(c))
		return ' ';
	if (isalnum(c) || ispunct(c))
		return c;
	return 0;
}

static void __sanitize_name(struct strbuff* req, const char* filename)
{
	size_t written = 0;

	for (; *filename; filename++) {
		char c = __sanitize(*filename);
		if (c) {
			strbuff_addch(req, c);
			written++;
		}
	}

	if (! written)
		strbuff_addz(req, "Brand New Playlist");
}

char* api_create_playlist(const char* filename)
{
	char* user_id = api_get_user_id();

	char path[10240];
	sprintf(path, "/users/%s/playlists", user_id);

	struct strbuff req, resp;
	bzero(&req, sizeof(req));
	bzero(&resp, sizeof(resp));

	strbuff_addz(&req, "{\"name\": \"");
	__sanitize_name(&req, filename);
	strbuff_addz(&req, "\", \"description\": ");
	strbuff_addz(&req, "\"Made with https://github.com/geekyfox/spy\"");
	strbuff_addz(&req, ", \"public\": false}");
	strbuff_addch(&req, 0);

	__post(&resp, path, req.data);

	jj_t ret = jj_parse(resp.data, resp.wix - 1, NULL);
	jj_t tmp = jj_pop(ret, "id", NULL);
	char* playlist_id = jj_tostr(tmp, NULL);

	free(user_id);
	free(resp.data);
	free(req.data);
	jj_free(ret);

	return playlist_id;
}
