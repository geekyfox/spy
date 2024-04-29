#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <curl/curl.h>

#include "spy.h"

const char HELP[] =
	"Usage:\n"
	"    spy login\n"
	"        Sets up the credentials for accessing Spotify API\n"
	"    spy list\n"
	"        Retrieves and prints the full list of user's playlist\n"
	"    spy fetch <playlist_id> <filename>\n"
	"        Retrieves the playlist and stores into a file\n"
	"    spy pull [--keep-order] <filename>\n"
	"        Retrieves the playlist and combines it with one stored in a local file\n";

static int __argc;
static char** __argv;

static bool __match(const char* name, int argct)
{
	if (__argc != argct + 2)
		return false;

	if (strcmp(__argv[1], name))
		return false;

	return true;
}

static bool __match_flag(const char* name, const char* flag, int argct)
{
	if (! __match(name, argct + 1))
		return false;

	if (strcmp(__argv[2], flag) != 0)
		return false;

	return true;
}

int main(int argc, char** argv)
{
	__argv = argv;
	__argc = argc;

	if (__match("login", 0))
		cmd_login();
	else if (__match("list", 0))
		cmd_list();
	else if (__match("fetch", 2))
		cmd_fetch(argv[2], argv[3]);
	else if (__match("pull", 1))
		cmd_pull(argv[2], PULL_MODE_REMOTE_ORDER);
	else if (__match_flag("pull", "--keep-order", 1))
		cmd_pull(argv[3], PULL_MODE_LOCAL_ORDER);
	else
		printf("%s", HELP);
}
