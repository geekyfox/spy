#ifndef __JJ_HEADER_FILE__
#define __JJ_HEADER_FILE__

#include <stdbool.h>
#include <stdio.h>

struct jj;
typedef struct jj* jj_t;

enum jj_error_code {
	JJ_ALLGOOD = 0,
	JJ_ERR_WRONGTYPE = 31001,
	JJ_ERR_NOTFOUND = 31002,
	JJ_ERR_OUTBOUND = 31003,
	JJ_ERR_TAKEN = 31004,
	JJ_ERR_EARLYEOF = 31005,
	JJ_ERR_BADTOKEN = 31006,
	JJ_ERR_LONGSTRING = 31007,
	JJ_ERR_PASTEOF = 31008,
};

typedef enum jj_error_code jj_err_t;

struct jj_parse_error {
	jj_err_t perr_code;
	size_t perr_line;
	size_t perr_column;
};

jj_t jj_parse(const char* payload, size_t size, struct jj_parse_error*);
jj_t jj_fparse(FILE* stream, struct jj_parse_error*);
void jj_free(jj_t);

bool jj_isnull(jj_t);

int jj_toint(jj_t, jj_err_t*);
char* jj_tostr(jj_t, jj_err_t*);
double jj_tonum(jj_t, jj_err_t*);

jj_t jj_pop(jj_t, const char* key, jj_err_t*);

size_t jj_len(jj_t, jj_err_t*);
jj_t jj_popl(jj_t, size_t idx, jj_err_t*);
void jj_merge(jj_t, jj_t, jj_err_t*);

const char* jj_errmsg(jj_err_t);

#endif
