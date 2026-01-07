#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "spy.h"

static char* the_client_id = NULL;
static char* the_client_secret = NULL;
static char* the_access_token = NULL;
static char* the_refresh_token = NULL;
static double the_expires_at = 0;

static void __pathname(char* pathname)
{
	char* home = getenv("HOME");
	if (! home)
		DIE("$HOME environment variable is not set");

	sprintf(pathname, "%s/.spy.json", home);
}

static FILE* __fopen(const char* mode)
{
	char pathname[1024];
	__pathname(pathname);

	FILE* f = fopen(pathname, mode);
	if (! f)
		HALT("Error opening file %s", pathname);

	return f;
}

static void __write(void)
{
	FILE* f = __fopen("w");

	fprintf(f, "{\n");
	fprintf(f, "\t\"client_id\":     \"%s\", \n", the_client_id);
	fprintf(f, "\t\"client_secret\": \"%s\", \n", the_client_secret);
	fprintf(f, "\t\"access_token\":  \"%s\", \n", the_access_token);
	fprintf(f, "\t\"refresh_token\": \"%s\", \n", the_refresh_token);
	fprintf(f, "\t\"expires_at\":    %lf     \n", the_expires_at);
	fprintf(f, "}\n");

	fclose(f);
}

static char* __popstr(jj_t value, char* key, char* pathname)
{
	jj_err_t err;
	jj_t item = jj_pop(value, key, &err);

	if (err)
		goto nope;

	char* result = jj_tostr(item, &err);

	if (err)
		goto nope;

	return result;

nope:
	DIE("Failed to get '%s' from %s: %s", key, pathname, jj_errmsg(err));
}

static double __popnum(jj_t value, char* key, char* pathname)
{
	jj_err_t err;
	jj_t item = jj_pop(value, key, &err);

	if (err)
		goto nope;

	double result = jj_tonum(item, &err);

	if (err)
		goto nope;

	return result;

nope:
	DIE("Failed to get '%s' from %s: %s", key, pathname, jj_errmsg(err));
}

static void __read(void)
{
	char pathname[1024];
	__pathname(pathname);

	jj_t value = fs_read_json(pathname);

	the_client_id = __popstr(value, "client_id", pathname);
	the_client_secret = __popstr(value, "client_secret", pathname);
	the_access_token = __popstr(value, "access_token", pathname);
	the_refresh_token = __popstr(value, "refresh_token", pathname);
	the_expires_at = __popnum(value, "expires_at", pathname);

	jj_free(value);
}

static const char B64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz"
				"0123456789+/";

void b64_encode(char* out, const char* in)
{
	int rix = 0, wix = 0;
	int pad = 0;

	while (pad == 0) {
		char a = in[rix++];
		char b = in[rix++];
		char c = in[rix++];

		if (! a)
			break;
		if (! b)
			pad++;
		if (! c)
			pad++;

		int n = (a << 16) | (b << 8) | c;
		out[wix++] = B64_TABLE[(n >> 18) & 0x3F];
		out[wix++] = B64_TABLE[(n >> 12) & 0x3F];
		out[wix++] = pad >= 2 ? '=' : B64_TABLE[(n >> 6) & 0x3F];
		out[wix++] = pad >= 1 ? '=' : B64_TABLE[n & 0x3F];
	}
	out[wix++] = '\0';
}

void __make_login_params(char* dst, size_t size, const char* code,
			 const char* redirect_url)
{
	struct strbuff buff = strbuff_wrap(dst, size);
	url_encode_pair(&buff, "grant_type", "authorization_code");
	url_encode_pair(&buff, "&code", code);
	url_encode_pair(&buff, "&redirect_uri", redirect_url);
}

void __make_refresh_params(char* dst, size_t size)
{
	struct strbuff buff = strbuff_wrap(dst, size);
	url_encode_pair(&buff, "grant_type", "refresh_token");
	url_encode_pair(&buff, "&refresh_token", the_refresh_token);
}

void __make_auth_header(char* dst)
{
	char raw[256], encoded[512];
	sprintf(raw, "%s:%s", the_client_id, the_client_secret);
	b64_encode(encoded, raw);
	sprintf(dst, "Authorization: Basic %s", encoded);
}

void __apply_response(struct strbuff* buff)
{
	jj_t value = jj_parse(buff->data, buff->wix - 1, NULL);

	jj_t tmp = jj_pop(value, "access_token", NULL);
	free(the_access_token);
	the_access_token = jj_tostr(tmp, NULL);

	tmp = jj_pop(value, "expires_in", NULL);
	double expires_in = jj_tonum(tmp, NULL);
	the_expires_at = time(NULL) + expires_in;

	jj_err_t err;
	tmp = jj_pop(value, "refresh_token", &err);
	switch (err) {
	case JJ_ALLGOOD:
		free(the_refresh_token);
		the_refresh_token = jj_tostr(tmp, NULL);
	case JJ_ERR_NOTFOUND:
		break;
	default:
		DIE("Something is wrong with refresh_token: %s",
		    jj_errmsg(err));
	};

	free(buff->data);
	jj_free(value);
}

void __refresh()
{
	char params[1024], auth_header[1024];
	struct strbuff buff;

	__make_refresh_params(params, 1024);
	__make_auth_header(auth_header);

	struct http_request req = {
		.method = "POST",
		.url = "https://accounts.spotify.com/api/token",
		.payload = params,
		.auth = auth_header,
	};

	http_submit(&buff, req);

	__apply_response(&buff);
	__write();
}

void __cleanup(void)
{
	free(the_client_id);
	free(the_client_secret);
	free(the_access_token);
	free(the_refresh_token);
}

void secrets_configure(const char* client_id, const char* client_secret)
{
	if (! the_client_id)
		atexit(__cleanup);

	the_client_id = strdup(client_id);
	the_client_secret = strdup(client_secret);
}

void secrets_login(const char* code, const char* redirect_url)
{
	char params[1024], auth_header[1024];
	struct strbuff buff;

	__make_login_params(params, 1024, code, redirect_url);
	__make_auth_header(auth_header);

	struct http_request req = {
		.method = "POST",
		.url = "https://accounts.spotify.com/api/token",
		.payload = params,
		.auth = auth_header,
	};

	http_submit(&buff, req);

	__apply_response(&buff);
	__write();
}

const char* secrets_token()
{
	if (! the_client_id) {
		__read();
		atexit(__cleanup);
	}

	int until_expiry = the_expires_at - time(NULL);
	if (until_expiry < 60)
		__refresh();

	return the_access_token;
}
