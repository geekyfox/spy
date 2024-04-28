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
	"        Set up the credentials for accessing Spotify API\n"
	"    spy list\n"
	"        Retrieves and prints the full list of user's playlist\n"
	"    spy fetch <playlist_id> <filename>\n"
	"        Retrieves the playlist and stores into a file\n";

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
	else
		printf("%s", HELP);
}
