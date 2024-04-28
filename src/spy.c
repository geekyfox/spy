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
	"        Set up the credentials for accessing Spotify API\n";

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
	else
		printf("%s", HELP);
}
