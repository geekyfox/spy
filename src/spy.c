#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "spy.h"

struct command {
	const char* name;
	void (*func)(void);
	const char* purpose;
	const char* usage;
};

static const struct command* the_command;
static char** the_argv;
static int the_argc;

static const struct command CMDS[] = {
	{.name = "clear",
	 .func = cmd_clear,
	 .purpose = "Clear a local playlist",
	 .usage = CMD_CLEAR_USAGE},
	{.name = "drop",
	 .func = cmd_drop,
	 .purpose = "Remove tracks at the beginning of a local playlist",
	 .usage = CMD_DROP_USAGE},
	{.name = "fetch",
	 .func = cmd_fetch,
	 .purpose = "Fetch a remote playlist and store into a file",
	 .usage = CMD_FETCH_USAGE},
	{.name = "filter",
	 .func = cmd_filter,
	 .purpose = "Keep (or removes) tracks by tag from a local playlist",
	 .usage = CMD_FILTER_USAGE},
	{.name = "fix",
	 .func = cmd_fix,
	 .purpose = "Fix issues in a local playlist",
	 .usage = CMD_FIX_USAGE},
	{.name = "list",
	 .func = cmd_list,
	 .purpose = "Print the full list of user's remote playlists",
	 .usage = CMD_LIST_USAGE},
	{.name = "log",
	 .func = cmd_log,
	 .purpose = "Log a listening session",
	 .usage = CMD_LOG_USAGE},
	{.name = "login",
	 .func = cmd_login,
	 .purpose = "Set up the credentials for accessing Spotify API",
	 .usage = CMD_LOGIN_USAGE},
	{.name = "or",
	 .func = cmd_or,
	 .purpose = "Make a union of two local playlists",
	 .usage = CMD_OR_USAGE},
	{.name = "pull",
	 .func = cmd_pull,
	 .purpose = "Apply the remote changes to a local playlist",
	 .usage = CMD_PULL_USAGE},
	{.name = "push",
	 .func = cmd_push,
	 .purpose = "Apply the local changes to a remote playlist",
	 .usage = CMD_PUSH_USAGE},
	{.name = "reverse",
	 .func = cmd_reverse,
	 .purpose = "Reverse a local playlist",
	 .usage = CMD_REVERSE_USAGE},
	{.name = "shuffle",
	 .func = cmd_shuffle,
	 .purpose = "Randomize a local playlist",
	 .usage = CMD_SHUFFLE_USAGE},
	{.name = "sort",
	 .func = cmd_sort,
	 .purpose = "Rearrange a local playlist",
	 .usage = CMD_SORT_USAGE},
	{.name = "stats",
	 .func = cmd_stats,
	 .purpose = "Print stats of a local playlist",
	 .usage = CMD_STATS_USAGE},
	{.name = "tag",
	 .func = cmd_tag,
	 .purpose = "Add/remove tags in a local playlist",
	 .usage = CMD_TAG_USAGE},
	{.name = "take",
	 .func = cmd_take,
	 .purpose = "Remove tracks at the end of a local playlist",
	 .usage = CMD_TAKE_USAGE},
	{.name = "xor",
	 .func = cmd_xor,
	 .purpose = "Apply an eXclusive OR operation to two local playlists",
	 .usage = CMD_XOR_USAGE},
};

static void __overview(int status, const char* cmd)
{
	FILE* out = status ? stderr : stdout;

	fputs("Usage: spy COMMAND [ARGS]...", out);
	fputs("\n\nAvailable commands:\n", out);

	int maxlen = 0;
	int count = sizeof(CMDS) / sizeof(struct command);

	for (int i = 0; i < count; i++) {
		int len = strlen(CMDS[i].name);
		if (len > maxlen)
			maxlen = len;
	}

	for (int i = 0; i < count; i++) {
		fprintf(out, "  %s", CMDS[i].name);

		int spacing = maxlen - strlen(CMDS[i].name) + 4;
		while (spacing > 0) {
			fputc(' ', out);
			spacing--;
		}

		fprintf(out, "%s\n", CMDS[i].purpose);
	}

	fputs("\nSee 'spy <command> --help' for more information", out);
	fputs(" on a specific command\n", out);

	if (cmd)
		error(status, 0, "Invalid command: %s", cmd);

	exit(status);
}

static void __help(int status)
{
	FILE* out = status ? stderr : stdout;

	if (! status)
		fprintf(out, "%s\n\n", the_command->purpose);

	fprintf(out, "Usage: spy %s ", the_command->name);

	int lines = 0, chars = 0;
	char buf[10240];

	for (const char* read = the_command->usage; *read; read++) {
		if (*read == '\t') {
			buf[chars++] = '\n';

			if (lines == 0)
				buf[chars++] = '\n';

			buf[chars] = '\0';
			fputs(buf, out);
			chars = 0;
			lines++;

			buf[chars++] = ' ';
			buf[chars++] = ' ';
			buf[chars++] = '*';
			buf[chars++] = ' ';
			continue;
		}

		if ((chars > 72) && (lines > 0)) {
			while (buf[chars] != ' ') {
				chars--;
				read--;
			}

			buf[chars++] = '\n';
			buf[chars] = '\0';
			fputs(buf, out);
			chars = 0;

			buf[chars++] = ' ';
			buf[chars++] = ' ';
			buf[chars++] = ' ';
		}

		buf[chars++] = *read;
	}

	buf[chars++] = '\n';
	buf[chars] = '\0';
	fputs(buf, out);
	exit(status);
}

int main(int argc, char** argv)
{
	if (argc == 1)
		__overview(1, NULL);

	if (! strcmp(argv[1], "--help"))
		__overview(0, NULL);

	int count = sizeof(CMDS) / sizeof(struct command);

	the_command = NULL;
	for (int i = 0; i < count; i++) {
		if (! strcmp(argv[1], CMDS[i].name)) {
			the_command = &CMDS[i];
			break;
		}
	}

	if (! the_command)
		__overview(1, argv[1]);

	if ((argc == 3) && (! strcmp(argv[2], "--help")))
		__help(0);

	the_argv = argv + 2;
	the_argc = argc - 2;

	the_command->func();
	return 0;
}

void args_abort(void)
{
	__help(1);
}

int args_flagx(const char** flags, size_t count)
{
	if (the_argc == 0)
		return -1;

	if (strncmp(the_argv[0], "--", 2))
		return -1;

	const char* flag = the_argv[0] + 2;

	the_argv++;
	the_argc--;

	for (size_t i = 0; i < count; i++) {
		if (! strcmp(flags[i], flag))
			return i;
	}

	__help(1);

	return -1;
}

bool args_flag(const char* flag)
{
	if (the_argc == 0)
		return false;

	if (strncmp(the_argv[0], "--", 2))
		return false;

	if (strcmp(the_argv[0] + 2, flag))
		return false;

	the_argv++;
	the_argc--;

	return true;
}

const char* args_popopt(void)
{
	if (the_argc == 0)
		return NULL;

	const char* arg = the_argv[0];
	the_argv++;
	the_argc--;

	return arg;
}

const char* args_pop(void)
{
	const char* arg = args_popopt();
	if (! arg)
		__help(1);
	return arg;
}

const char* args_poplast(void)
{
	const char* arg = args_pop();
	args_finish();
	return arg;
}

bool args_popnext(const char** dst)
{
	const char* arg = *dst ? args_popopt() : args_pop();
	*dst = arg;
	return arg != NULL;
}

void args_finish(void)
{
	if (the_argc > 0)
		__help(1);
}

int args_atoi(const char* num)
{
	char* endptr = NULL;
	int result = strtol(num, &endptr, 10);

	if ((errno == EINVAL) || (errno == ERANGE) || (*endptr))
		__help(1);

	return result;
}
