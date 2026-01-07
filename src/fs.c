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
	if (! f)
		HALT("Error opening file %s", pathname);

	struct strbuff ret;
	bzero(&ret, sizeof(ret));

	int ch;
	while ((ch = fgetc(f)) != EOF)
		strbuff_addch(&ret, ch);

	strbuff_addch(&ret, '\0');

	fclose(f);
	return ret;
}

jj_t fs_read_json(const char* filename)
{
	FILE* f = fopen(filename, "r");
	if (! f)
		HALT("Error opening file %s", filename);

	struct jj_parse_error err;

	jj_t ret = jj_fparse(f, &err);
	if (! ret) {
		DIE("Failed to parse %s: %s @ line %ld column %ld",
		    filename,
		    jj_errmsg(err.perr_code),
		    err.perr_line,
		    err.perr_column);
	}

	fclose(f);

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
		HALT("lstat() failed");
	}

	if ((stat.st_mode & S_IFMT) != S_IFLNK)
		return strdup(filename);

	char* result = realpath(filename, NULL);
	if (! result)
		HALT("realpath() failed");

	return result;
}

void fs_write_playlist(playlist_t p, const char* filename)
{
	char tmpfile[10240];
	sprintf(tmpfile, "%s.temp", filename);

	FILE* f = fopen(tmpfile, "w");
	if (! f)
		HALT("Error opening file %s", tmpfile);

	write_playlist(p, f);

	fclose(f);

	char* real_filename = __resolve(filename);
	rename(tmpfile, real_filename);
	free(real_filename);
}
