#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "spy.h"

struct strbuff fs_read(const char* pathname)
{
	FILE* f = fopen(pathname, "r");
	if (! f) {
		char* errmsg = strerror(errno);
		fprintf(stderr,
			"Error opening file %s: %s\n",
			pathname,
			errmsg);
		exit(1);
	}

	struct strbuff ret;
	bzero(&ret, sizeof(ret));

	int ch;
	while ((ch = fgetc(f)) != EOF)
		strbuff_addch(&ret, ch);

	strbuff_addch(&ret, '\0');

	fclose(f);
	return ret;
}

json_t fs_read_json(const char* filename)
{
	struct strbuff buff = fs_read(filename);
	json_t ret = json_parse(&buff);
	free(buff.data);
	return ret;
}

playlist_t fs_read_playlist(const char* filename)
{
	FILE* f = fopen(filename, "r");
	if (! f)
		HALT("Error opening file %s", filename);

	playlist_t ret = parse_playlist(f, filename);

	fclose(f);

	return ret;
}

static char* __resolve(const char* filename)
{
	struct stat stat;
	if (lstat(filename, &stat) < 0) {
		if (errno == ENOENT)
			return strdup(filename);
		char* err = strerror(errno);
		DIE("lstat() failed: %s", err);
	}

	if ((stat.st_mode & S_IFMT) != S_IFLNK)
		return strdup(filename);

	char* result = realpath(filename, NULL);
	if (! result) {
		char* err = strerror(errno);
		DIE("realpath() failed: %s", err);
	}

	return result;
}

void fs_write_playlist(playlist_t p, const char* filename)
{
	char tmpfile[10240];
	sprintf(tmpfile, "%s.temp", filename);

	FILE* f = fopen(tmpfile, "w");
	if (! f) {
		char* err = strerror(errno);
		DIE("Error opening file %s: %s", tmpfile, err);
	}

	write_playlist(p, f);

	fclose(f);

	char* real_filename = __resolve(filename);
	rename(tmpfile, real_filename);
	free(real_filename);
}
