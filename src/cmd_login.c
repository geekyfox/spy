#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "spy.h"

struct context {
	struct sockaddr_in addr;
	int server_fd;
	int client_fd;
	char request[16000];
	char scratch[16000];
	char* method;
	char* url;
	char redirect_uri[1024];
	char* body;
	struct strbuff resp;
};

static void __init(struct context* ctx, int port)
{
	bzero(ctx, sizeof(*ctx));
	ctx->addr.sin_family = AF_INET;
	ctx->addr.sin_addr.s_addr = INADDR_ANY;
	ctx->addr.sin_port = htons(port);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		DIE("socket() failed: %m");

	if (bind(fd, (struct sockaddr*)&ctx->addr, sizeof(ctx->addr)))
		DIE("bind() failed: %m");

	if (listen(fd, 1))
		DIE("listen() failed: %m");

	ctx->server_fd = fd;
}

static void __accept(struct context* ctx)
{
	socklen_t slen = sizeof(ctx->addr);
	int fd = accept(ctx->server_fd, (struct sockaddr*)&ctx->addr, &slen);

	if (fd < 0)
		DIE("accept() failed: %m");

	ctx->client_fd = fd;

	ssize_t size = read(fd, ctx->request, 15000);

	if (size < 0)
		DIE("read() failed: %m");

	ctx->request[size] = '\0';
	if (size == 15000)
		DIE("Request is too big: %s", ctx->request);
}

void __parse_method_url(struct context* ctx, char* line)
{
	char* sep = strstr(line, " ");
	if (! sep)
		DIE("Bad request: %s", ctx->request);

	*sep = '\0';
	ctx->method = line;

	line = sep + 1;
	sep = strstr(line, " ");

	if (! sep)
		DIE("Bad request: %s", ctx->request);

	*sep = '\0';

	ctx->url = line;
}

void __parse_header(struct context* ctx, char* line)
{
	char* sep = strstr(line, ": ");
	if (! sep)
		return;

	*sep = '\0';
	char* value = sep + 2;

	if (! strcmp(line, "Host"))
		sprintf(ctx->redirect_uri, "http://%s/callback", value);
}

void __parse(struct context* ctx)
{
	char* s = strcpy(ctx->scratch, ctx->request);

	enum { METHOD_URL = 5, HEADERS = 6, BODY = 7 } state = METHOD_URL;

	while (state != BODY) {
		char* sep = strstr(s, "\r\n");

		if (sep) {
			*sep = '\0';
		} else {
			fprintf(stderr, "s = %s\n", s);
			DIE("Bad request: %s", ctx->request);
		}

		if (state == METHOD_URL) {
			__parse_method_url(ctx, s);
			state = HEADERS;
		} else if (s == sep) {
			state = BODY;
		} else {
			__parse_header(ctx, s);
		}

		s = sep + 2;
	}

	ctx->body = s;
}

#define S(s)                                                                   \
	do {                                                                   \
		strbuff_addz(&ctx->resp, s);                                   \
	} while (0)

#define ENCODE(s)                                                              \
	do {                                                                   \
		url_encode(&ctx->resp, s);                                     \
	} while (0)

static void __handle_form(struct context* ctx)
{
	S("HTTP/1.1 200 OK\r\n\r\n");
	S("<html><body style=\"font-family: helvetica\">"
	  "  <form method=\"post\""
	  "        action=\"/submit\""
	  "        style=\"margin-top:10%; margin-left:30%; margin-right:30%\">"
	  "    <div><b>Client ID</b></div> <br>"
	  "    <input type=\"text\" name=\"client_id\" size=\"40%\" />"
	  "    <br><br>"
	  "    <div><b>Client Secret</b></div>"
	  "    <br>"
	  "    <input type=\"text\" name=\"client_secret\" size=\"40%\" />"
	  "    <br><br>"
	  "    <input type=\"submit\" value=\"Let's go!\" />"
	  "    <br><br>"
	  "    <div>To find client_id and client_secret, go to"
	  "    <a href=\"https://developer.spotify.com/dashboard\">Spotify"
	  "    Development Dashboard</a>, go to your app, then click"
	  "    \"Settings\". If you don't have an app yet, then you'd have"
	  "    to register one. Keep in mind that (one of) configured"
	  "    Redirect URIs must be <b>exactly</b> <pre>");
	S(ctx->redirect_uri);
	S("</pre></div>"
	  "  </form>"
	  "</body></html>");
}

bool __urlparse(char** haystack, char** key, char** value)
{
	if (! *haystack)
		return false;

	char* sep = strstr(*haystack, "?");
	if (sep)
		*haystack = sep + 1;

	sep = strstr(*haystack, "=");
	if (! sep)
		return false;

	*key = *haystack;
	*sep = '\0';
	*value = sep + 1;

	sep = strstr(*value, "&");
	if (sep) {
		*sep = '\0';
		*haystack = sep + 1;
	} else {
		*haystack = NULL;
	}

	return true;
}

static void __handle_redirect(struct context* ctx)
{
	char *haystack = ctx->body, *key, *value;
	char *client_id = NULL, *client_secret = NULL;

	while (__urlparse(&haystack, &key, &value)) {
		if (! strcmp(key, "client_id")) {
			client_id = value;
		} else if (! strcmp(key, "client_secret")) {
			client_secret = value;
		} else {
			printf("Odd parameter: key='%s' value='%s'\n",
			       key,
			       value);
		}
	}

	if (! client_id || ! client_secret) {
		S("HTTP/1.1 400 Bad Request\r\n\r\n");
		if (! client_id)
			S("Bad request: client_id is missing");
		if (! client_secret)
			S("Bad request: client_secret is missing");
		return;
	}

	secrets_configure(client_id, client_secret);

	S("HTTP/1.1 302 Found\r\n");
	S("Location: https://accounts.spotify.com/authorize");
	S("?client_id=");
	S(client_id);
	S("&response_type=code");
	S("&redirect_uri=");
	ENCODE(ctx->redirect_uri);
	S("&scope=playlist-read-private+playlist-modify-private");
	S("+playlist-modify-public+user-library-read+user-library-modify");
	S("&show_dialog=true");
	S("\r\n\r\n");
}

static void __handle_callback(struct context* ctx)
{
	char *haystack = ctx->url, *key, *value, *code = NULL;

	while (__urlparse(&haystack, &key, &value)) {
		if (! strcmp(key, "code")) {
			code = value;
		} else {
			printf("Odd parameter: key='%s' value='%s'\n",
			       key,
			       value);
		}
	}

	secrets_login(code, ctx->redirect_uri);

	S("HTTP/1.1 200 OK\r\n\r\n");
	S("<html><body style=\"font-family: helvetica\">"
	  "  <div style=\"margin-top:10%; margin-left:30%; margin-right:30%\">"
	  "    <b>ALL SET!</b>"
	  "  </div>"
	  "</body></html>");
}

bool __match(struct context* ctx, const char* method, const char* url)
{
	if (strcmp(ctx->method, method))
		return false;

	char* sep = strstr(ctx->url, "?");
	if (sep) {
		int n = sep - ctx->url;
		return ! strncmp(ctx->url, url, n);
	}

	return ! strcmp(ctx->url, url);
}

void cmd_login(void)
{
	struct context ctx;
	__init(&ctx, 8888);

	printf("Now go to http://localhost:8888/setup and follow the "
	       "instructions\n");

	while (true) {
		__accept(&ctx);
		__parse(&ctx);

		ctx.resp.wix = 0;

		if (__match(&ctx, "GET", "/setup")) {
			__handle_form(&ctx);
		} else if (__match(&ctx, "POST", "/submit")) {
			__handle_redirect(&ctx);
		} else if (__match(&ctx, "GET", "/callback")) {
			__handle_callback(&ctx);
		} else {
			printf("method = %s ; url = %s ; body = %s\n",
			       ctx.method,
			       ctx.url,
			       ctx.body);

			strbuff_addz(&ctx.resp,
				     "HTTP/1.1 404 Not Found\r\n\r\n");
		}

		write(ctx.client_fd, ctx.resp.data, ctx.resp.wix);
		close(ctx.client_fd);
	}
}
