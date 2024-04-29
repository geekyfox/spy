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

static json_t __get(const char* path)
{
	struct strbuff buff;
	__submit(&buff, "GET", path, NULL);
	json_t ret = json_parse(&buff);
	free(buff.data);
	return ret;
}

json_t api_get_paginated(const char* path)
{
	json_t ret = json_array();
	char* url = strdup(path);

	while (url) {
		json_t resp = __get(url);
		free(url);

		json_t page = json_pop(resp, "items", NULL);
		json_merge(ret, page);
		json_free(page);

		json_t next = json_pop(resp, "next", NULL);
		json_free(resp);

		if (json_isnull(next))
			url = NULL;
		else
			url = json_uwstr(next);
		json_free(next);
	}

	return ret;
}

void __digest_track(track_t ret, json_t blob)
{
	json_t track = json_pop(blob, "track", NULL);

	bzero(ret, sizeof(*ret));

	ret->id = json_popstr(track, "id", NULL);
	ret->name = json_popstr(track, "name", NULL);

	json_t artists = json_pop(track, "artists", NULL);

	int count = json_len(artists);
	for (int i = 0; i < count; i++) {
		json_t artist = json_get(artists, i);
		const char* name = json_popstr(artist, "name", NULL);
		strarr_add(&ret->artists, name);
	}

	strarr_add(&ret->tags, "new");
}

playlist_t api_get_playlist(const char* id)
{
	playlist_t ret = playlist_init(NULL);
	ret->playlist_id = strdup(id);

	char path[10240];
	sprintf(path, "/playlists/%s/tracks", id);
	json_t resp = api_get_paginated(path);

	int count = json_len(resp);

	for (int i = 0; i < count; i++) {
		json_t blob = json_get(resp, i);
		struct track tr;
		__digest_track(&tr, blob);
		tr.remote_index = i + 1;
		playlist_add(ret, &tr);
	}

	json_free(resp);

	validate_playlist(ret, path, 0);

	return ret;
}

static void __post(const char* path, const char* body)
{
	__submit(NULL, "POST", path, body);
}

static void __delete(const char* path, const char* body)
{
	__submit(NULL, "DELETE", path, body);
}

static void __put(const char* path, const char* body)
{
	__submit(NULL, "PUT", path, body);
}

void api_reorder(const char* playlist_id, int start, int before, int length)
{
	char path[1024], content[10240];
	sprintf(path, "/playlists/%s/tracks", playlist_id);
	sprintf(content,
		"{\"range_start\":%d,\"insert_before\":%d,\"range_length\":%d}",
		start,
		before,
		length);

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
		strbuff_addz(&req, "]}\0");
		__post(path, req.data);
		req.wix = 0;
		first = last;
	}

	free(req.data);
}

void api_remove_tracks(const char* playlist_id, const struct strarr* track_ids)
{
	char path[10240];
	sprintf(path, "/playlists/%s/tracks", playlist_id);

	struct strbuff req;
	bzero(&req, sizeof(req));
	strbuff_addz(&req, "{\n    \"tracks\":\n        [");

	for (int i = 0; i < track_ids->count; i++) {
		if (i != 0)
			strbuff_addch(&req, ',');
		strbuff_addz(&req, "\n        {\"uri\":\"spotify:track:");
		strbuff_addz(&req, track_ids->data[i]);
		strbuff_addz(&req, "\"}");
	}
	strbuff_addz(&req, "]}\0");

	__delete(path, req.data);
}
