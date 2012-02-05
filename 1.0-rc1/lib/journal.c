
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "event.h"
#include "utils.h"

static int log_daemonized =  false;

void log_set_lvl(int lvl)
{
	journal_strace("log_set_lvl");

	if (lvl == 1)
		log_daemonized = true;
	else
		log_daemonized = false;
}

void journal_failure(int err_flag, char *msg, ...)
{
	journal_strace("journal_failure");

	va_list ap;
	va_start(ap, msg);

	if (log_daemonized) {
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

	if (log_daemonized) {
		openlog("XIAD", LOG_PID, LOG_DAEMON);
		vsyslog(LOG_ERR, msg, ap);
	}
	else
		vfprintf(stdout, msg, ap);

	va_end(ap);
}

#define STRACE_MAX_ENTRY 7
static int strace_idx = 0;
static char *strace_logs[STRACE_MAX_ENTRY] = {NULL};

void journal_strace(char *fname)
{
	/* XXX journal_strace() should never be called
	 * inside itself, otherwise a infinite 
	 * recursivity will be created.
	 */

	if (strace_logs[strace_idx] != NULL) {
		free(strace_logs[strace_idx]);
	}

	strace_logs[strace_idx++] = strdup(fname);

	if (strace_idx == STRACE_MAX_ENTRY)
		strace_idx = 0;
}

void journal_strace_dump()
{
	int idx = strace_idx;

	/* XXX journal_strace() should never be called
	 * inside this function, otherwise a racecondition
	 * may occur with the strace index.
	 */

	if ((idx < STRACE_MAX_ENTRY && strace_logs[idx + 1] == NULL) || idx == STRACE_MAX_ENTRY)
		idx = 0;

	do {
		/* XXX control the stream here */
		printf("strace]> (%02i) => %s\n", idx, strace_logs[idx++]);

		if (idx == STRACE_MAX_ENTRY)
			idx = 0;
	} while (idx != strace_idx);
}

int journal_init()
{
	journal_strace("journal_init");
	event_register(EVENT_EXIT, "journal:journal_strace_dump()", journal_strace_dump, PRIO_LOW);

	return 0;
}
