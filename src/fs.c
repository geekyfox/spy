#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "spy.h"

struct strbuff fs_read(const char* pathname)
{
	FILE* f = fopen(pathname, "r");
	if (! f)
		DIE("Error opening file %s: %m", pathname);

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
