
/*
 * See COPYRIGHTS file.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "event.h"
#include "journal.h"
#include "utils.h"

static int journal_daemonized =  false;

void journal_set_lvl(int lvl)
{
	journal_ftrace(__func__);

	if (lvl == 1)
		journal_daemonized = true;
	else
		journal_daemonized = false;
}

void journal_failure(int err_flag, char *msg, ...)
{
	journal_ftrace(__func__);

	va_list ap;
	va_start(ap, msg);

	if (journal_daemonized) {
		openlog("XIAD", LOG_PID, LOG_DAEMON);
		vsyslog(LOG_ERR, msg, ap);
	}
	else
		vfprintf(stderr, msg, ap);

	va_end(ap);

	/*_exit(err_flag); must now be explicitely called
	 * after journal_failure().
	 */
}

void journal_notice(char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);

	if (journal_daemonized) {
		openlog("XIAD", LOG_PID, LOG_DAEMON);
		vsyslog(LOG_ERR, msg, ap);
	}
	else
		vfprintf(stdout, msg, ap);

	va_end(ap);
}

#define FTRACE_MAX_ENTRY 20
static int strace_idx = 0;
static char *strace_logs[FTRACE_MAX_ENTRY] = {NULL};

void journal_ftrace(const char *fname)
{
	/* XXX journal_ftrace() should never be called
	 * inside itself, otherwise a infinite 
	 * recursivity will be created.
	 */

	free(strace_logs[strace_idx]);
	strace_logs[strace_idx] = NULL;

	strace_logs[strace_idx++] = strdup(fname);

	if (strace_idx == FTRACE_MAX_ENTRY)
		strace_idx = 0;
}

void journal_ftrace_dump()
{
	int idx = strace_idx;

	/* 
	 * XXX journal_ftrace() should never be called
	 * inside this function, otherwise a racecondition
	 * may occur with the strace index.
	 */

	if ((idx < FTRACE_MAX_ENTRY && strace_logs[idx + 1] == NULL) || idx == FTRACE_MAX_ENTRY)
		idx = 0;

	do {
		/* XXX control the stream here */
		printf("strace]> (%02i) => %s\n", idx, strace_logs[idx++]);

		if (idx == FTRACE_MAX_ENTRY)
			idx = 0;
	} while (idx != strace_idx);
}

int journal_init()
{
	journal_ftrace(__func__);
	event_register(EVENT_EXIT, "journal:journal_ftrace_dump()", journal_ftrace_dump, PRIO_LOW);

	return 0;
}
