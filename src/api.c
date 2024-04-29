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
