
/*
 * See COPYRIGHTS file.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "../lib/journal.h"
#include "../lib/netbus.h"
#include "../lib/sched.h"

#include "pingring.h"
#include "reslist.h"

#ifndef PING_INTERVAL
# define PING_INTERVAL 15
#endif

/* Ring of ressources to ping */
pingring_t *pring;

void pingring_get_reply(peer_t *p)
{
	journal_ftrace(__func__);

	reslist_t *entry = (reslist_t *)p->udata;

	if (entry) {
		journal_notice("pingring]> got a ping reply from %s :: %s:%i\n", inet_ntoa(entry->addr), __FILE__, __LINE__);

		entry->lastseen = time(NULL);
		discovery_set_res_alive(entry);

	} else {
		journal_notice("pingring]> received NULL entry pointer :: %s:%i\n", __FILE__, __LINE__);
	}
}

int pingring_add(reslist_t *entry)
{
	journal_ftrace(__func__);

	pingring_t *pr;
	char *addr;

	pr = malloc(sizeof(pingring_t));
	if (pr == NULL) {
		journal_notice("pingring]> malloc failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	addr = inet_ntoa(entry->addr);

	pr->pinger = netbus_newping(addr, pingring_get_reply, entry);

	pr->entry = entry;
	pr->next = pring;
	pring = pr;

	return 0;
}

int pingring_del(reslist_t *entry)
{
	journal_ftrace(__func__);

	pingring_t *itr = pring,
		   *tmp = NULL;

	if (itr == NULL) {
		journal_notice("pingring]> head is null :: %s:%i\n", __FILE__, __LINE__);
		return 1;
	}

	do {
		if (itr->entry == entry) {

			if (tmp) {
				tmp->next = itr->next;
			} else {
				pring = itr->next;
			}

			itr->pinger->disconnect(itr->pinger);
			free(itr);
			break;
		}

		tmp = itr;
		itr = itr->next;

	} while (itr != NULL);

	return 0;
}

void pingring_show()
{
	journal_ftrace(__func__);

	pingring_t *itr;
	int num = 1;

	journal_notice("pingring]> dumping pingring entries :: %s:%i\n", __FILE__, __LINE__);

	for (itr = pring; itr != NULL; itr = itr->next, ++num)
		journal_notice("%i:%s:%s\n", num, inet_ntoa(itr->entry->addr),
		    itr->entry->hostname);
}

void pingring_ping(void *udata)
{
	journal_ftrace(__func__);

	pingring_t *itr = pring;
	time_t elapsed;

	if (itr == NULL)
		return;

	for (; itr != NULL; itr = itr->next) {

		if (itr->entry->lastseen == 0) {
			itr->entry->lastseen = time(NULL);
			continue;
		}

		elapsed = difftime(time(NULL), itr->entry->lastseen);

		if (elapsed > itr->entry->timeout)
			discovery_set_res_dead(itr->entry);

		itr->pinger->ping(itr->pinger);
	}
}

int pingring_init()
{
	journal_ftrace(__func__);

	sched_register(SCHED_PERIODIC, "pingring_ping", pingring_ping, PING_INTERVAL, NULL);

	return 0;
}
