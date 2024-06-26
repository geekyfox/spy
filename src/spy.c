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
	"        Fetch and prints the full list of user's playlist\n"
	"    spy fetch <playlist_id> <filename>\n"
	"        Fetch remote playlist and store into a file\n"
	"    spy pull [--keep-order] <filename>\n"
	"        Fetch remote playlist and incorporate it with a local one\n"
	"    spy push [--dryrun] <filename>\n"
	"        Apply the local changes to remote playlist\n"
	"    spy sort [--race] <filename>\n"
	"        Rearrange the playlist\n"
	"    spy shuffle [<count>] <filename>\n"
	"        Randomize the playlist\n"
	"    spy log [--bump] <filename>\n"
	"        Logs a listening session\n"
	"    spy stats <filename>\n"
	"        Computes stats about remote playlist\n"
	"    spy sync-tags <source filename> <target filename>\n"
	"        Syncs tags between two playlists\n"
	"    spy add-all <source filename> <target filename>\n"
	"        Adds tracks from one playlist to another\n"
	"    spy filter <tag> <filename>\n"
	"        Keeps (or removes) tracks by tag\n"
	"    spy clear <filename>\n"
	"        Clears playlist\n"
	"    spy fix <filename>\n"
	"        Fixes duplicates\n";

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

	if (__match("add-all", 2))
		cmd_add_all(argv[2], argv[3]);
	else if (__match("clear", 1))
		cmd_clear(argv[2]);
	else if (__match("fetch", 2))
		cmd_fetch(argv[2], argv[3]);
	else if (__match("filter", 2))
		cmd_filter(argv[2], argv[3]);
	else if (__match("fix", 1))
		cmd_fix(argv[2]);
	else if (__match("log", 1))
		cmd_log(argv[2], LOG_MODE_DUMP_UNTAGGED);
	else if (__match_flag("log", "--bump", 1))
		cmd_log(argv[3], LOG_MODE_BUMP_UNTAGGED);
	else if (__match("login", 0))
		cmd_login();
	else if (__match("list", 0))
		cmd_list();
	else if (__match("pull", 1))
		cmd_pull(argv[2], PULL_MODE_REMOTE_ORDER);
	else if (__match_flag("pull", "--keep-order", 1))
		cmd_pull(argv[3], PULL_MODE_LOCAL_ORDER);
	else if (__match("push", 1))
		cmd_push(argv[2], false);
	else if (__match_flag("push", "--dryrun", 1))
		cmd_pull(argv[3], true);
	else if (__match("sort", 1))
		cmd_sort(argv[2], SORT_MODE_DEFAULT);
	else if (__match_flag("sort", "--race", 1))
		cmd_pull(argv[3], SORT_MODE_RACE);
	else if (__match("shuffle", 1))
		cmd_shuffle(argv[2], 0);
	else if (__match("shuffle", 2))
		cmd_shuffle(argv[3], atoi(argv[2]));
	else if (__match("stats", 1))
		cmd_stats(argv[2]);
	else if (__match("sync-tags", 2))
		cmd_sync_tags(argv[2], argv[3]);
	else
		printf("%s", HELP);
}
