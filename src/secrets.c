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
		DIE("Error opening file %s: %m", pathname);

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

static void __read(void)
{
	char pathname[1024];
	__pathname(pathname);

	json_t value = fs_read_json(pathname);
	bool flag;

	the_client_id = json_popstr(value, "client_id", &flag);
	if (! flag)
		DIE("'client_id' is missing in %s", pathname);

	the_client_secret = json_popstr(value, "client_secret", &flag);
	if (! flag)
		DIE("'client_secret' is missing in %s", pathname);

	the_access_token = json_popstr(value, "access_token", &flag);
	if (! flag)
		DIE("'access_token' is missing in %s", pathname);

	the_refresh_token = json_popstr(value, "refresh_token", &flag);
	if (! flag)
		DIE("'refresh_token' is missing in %s", pathname);

	the_expires_at = json_popnum(value, "expires_at", &flag);
	if (! flag)
		DIE("'expires_at' is missing in %s", pathname);

	json_free(value);
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
	json_t value = json_parse(buff);

	bool flag;
	char* new_access_token = json_popstr(value, "access_token", &flag);
	if (flag) {
		free(the_access_token);
		the_access_token = new_access_token;
	} else {
		DIE("'access_token' is missing in %s", buff->data);
	}

	double expires_in = json_popnum(value, "expires_in", &flag);
	if (flag) {
		the_expires_at = time(NULL) + expires_in;
	} else {
		DIE("'expires_in' is missing in %s", buff->data);
	}

	char* new_refresh_token = json_popstr(value, "refresh_token", &flag);
	if (flag) {
		free(the_refresh_token);
		the_refresh_token = new_refresh_token;
	}

	free(buff->data);
	json_free(value);
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
