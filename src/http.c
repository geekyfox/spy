#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <curl/curl.h>

#include "spy.h"

static CURL* the_curl = NULL;

static void __cleanup(void)
{
	curl_easy_cleanup(the_curl);
}

void __ensure_inited(void)
{
	if (the_curl) {
		curl_easy_reset(the_curl);
	} else {
		the_curl = curl_easy_init();
		atexit(__cleanup);
	}
}

static void callback(char* ptr, size_t size, size_t nmemb, struct strbuff* buff)
{
	strbuff_add(buff, ptr, nmemb);
}

static void __abort_on_curl_error(CURLcode res, const char* errbuf)
{
	if (res == CURLE_OK)
		return;

	size_t len = strlen(errbuf);
	fprintf(stderr, "\nlibcurl: (%d) ", res);
	if (len)
		fprintf(stderr,
			"%s%s",
			errbuf,
			((errbuf[len - 1] != '\n') ? "\n" : ""));
	else
		fprintf(stderr, "%s\n", curl_easy_strerror(res));

	exit(1);
}

static void __abort_on_http_error(struct http_request req, const char* response)
{
	long http_code;
	curl_easy_getinfo(the_curl, CURLINFO_RESPONSE_CODE, &http_code);

	if ((http_code == 200) || (http_code == 201))
		return;

	fprintf(stderr, "Bad response\n");
	fprintf(stderr, "\thttp_code = %ld\n", http_code);
	fprintf(stderr, "\turl = %s\n", req.url);
	if (req.payload)
		fprintf(stderr, "\tpayload = %s\n", req.payload);
	fprintf(stderr, "\tresponse = %s\n", response);
	exit(1);
}

void http_submit(struct strbuff* ret, struct http_request req)
{
	struct strbuff tmp;
	struct strbuff* buf = ret ? ret : &tmp;
	bzero(buf, sizeof(*buf));

	char errbuf[CURL_ERROR_SIZE];

	__ensure_inited();

	curl_easy_setopt(the_curl, CURLOPT_URL, req.url);
	curl_easy_setopt(the_curl, CURLOPT_CUSTOMREQUEST, req.method);
	curl_easy_setopt(the_curl, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(the_curl, CURLOPT_WRITEDATA, buf);
	curl_easy_setopt(the_curl, CURLOPT_ERRORBUFFER, errbuf);

	if (req.payload)
		curl_easy_setopt(the_curl, CURLOPT_POSTFIELDS, req.payload);

	struct curl_slist* headers = NULL;
	if (req.auth)
		headers = curl_slist_append(headers, req.auth);
	if (req.payload_is_json)
		headers = curl_slist_append(headers,
					    "Content-Type: application/json");
	if (headers)
		curl_easy_setopt(the_curl, CURLOPT_HTTPHEADER, headers);

	CURLcode res = curl_easy_perform(the_curl);
	__abort_on_curl_error(res, errbuf);

	strbuff_addch(buf, '\0');
	__abort_on_http_error(req, buf->data);

	curl_slist_free_all(headers);

	if (! ret)
		free(buf->data);
}
