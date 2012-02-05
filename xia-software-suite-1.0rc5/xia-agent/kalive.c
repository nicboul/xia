
/*
 * See COPYRIGHTS file.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../lib/event.h"
#include "../lib/journal.h"
#include "../lib/netbus.h"
#include "../lib/sched.h"

#include "kalive.h"

#ifndef HEARTBEAT_INTERVAL
# define HEARTBEAT_INTERVAL 10
#endif

#ifndef HEARTBEAT_TIMEOUT
# define HEARTBEAT_TIMEOUT 60
#endif


static time_t heartbeat_lastseen;
peer_t *heartbeat;
char *kalive_addr;

static void heartbeat_check(void *udata)
{
	journal_ftrace(__func__);

	peer_t *remote = udata;

	if (difftime(time(NULL), heartbeat_lastseen) > HEARTBEAT_TIMEOUT) {
		journal_notice("kalive]> no heartbeat from server %s:%i\n", __FILE__, __LINE__);
		event_throw(EVENT_EXIT, NULL);
	} else {
		remote->ping(remote);
	}
}

static void heartbeat_reply()
{
	journal_ftrace(__func__);

	heartbeat_lastseen = time(NULL);
}


static void kalive_fini()
{
	journal_ftrace(__func__);

	free(kalive_addr);

	if (heartbeat)
		heartbeat->disconnect(heartbeat);
}

int kalive_init(char *addr)
{
	journal_ftrace(__func__);

	kalive_addr = strdup(addr);

	heartbeat_lastseen = time(NULL);
	heartbeat = netbus_newping(kalive_addr, heartbeat_reply, NULL);
	sched_register(SCHED_PERIODIC, "heartbeat_check", heartbeat_check, HEARTBEAT_INTERVAL, heartbeat);
	event_register(EVENT_EXIT, "kalive_fini", kalive_fini, PRIO_AGNOSTIC);

	return 0;
}
