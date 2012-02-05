/*
 * See COPYRIGHTS file.
 */

#include <errno.h>
#include <stdio.h>

#include <arpa/inet.h>

#include "../lib/journal.h"

#include "pingring.h"
#include "reslist.h"

static void (*replied_callback)(reslist_t *);
static void (*timeout_callback)(reslist_t *);

void pingring_get_reply(peer_t *p)
{
	reslist_t *entry = (reslist_t *)p->udata;

	journal_strace(__func__);

	if (entry) {
		journal_notice("%s %s :: %s:%i\n",
		    "got a ping reply from", inet_ntoa(entry->addr),
		    __FILE__, __LINE__);
		entry->lastseen = time(NULL);
		replied_callback(entry);
	} else {
		journal_notice("%s :: %s:%i\n",
		    "received NULL pointer", __FILE__, __LINE__);
	}
}

int pingring_add(pingring_t **pring, reslist_t *entry)
{
	pingring_t *pr;
	char *addr;

	pr = malloc(sizeof(pingring_t));
	if (pr == NULL) {
		journal_notice("%s :: %s:%i\n",
		    "malloc failed", __FILE__, __LINE__);
		return 1;
	}

	addr = inet_ntoa(entry->addr);

	pr->pinger = netbus_newping(addr, pingring_get_reply, entry);

	pr->entry = entry;
	pr->next = *pring;
	*pring = pr;

	return 0;
}

int pingring_del(pingring_t **pring, reslist_t *entry)
{
	pingring_t *itr = *pring,
		   *tmp = NULL;

	if (itr == NULL) {
		journal_notice("%s :: %s:%i\n",
		    "head is null", __FILE__, __LINE__);
		return 1;
	}

	do {
		if (itr->entry == entry) {

			if (tmp) {
				tmp->next = itr->next;
			} else {
				*pring = itr->next;
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

void pingring_show(pingring_t *pring)
{
	pingring_t *itr;
	int num = 1;

	journal_notice("dumping pingring entries...\n");

	for (itr = pring; itr != NULL; itr = itr->next, ++num)
		journal_notice("%i:%s:%s\n", num, inet_ntoa(itr->entry->addr),
		    itr->entry->hostname);
}

void pingring_ping(pingring_t **pring)
{
	pingring_t *itr = *pring;
	time_t elapsed;

	if (itr == NULL)
		return;

	for (; itr != NULL; itr = itr->next) {

		if (itr->entry->lastseen == 0) {
			itr->entry->lastseen = time(NULL);
			continue;
		}

		elapsed = difftime(time(NULL), itr->entry->lastseen);

		if (elapsed > itr->entry->timeout && timeout_callback)
			timeout_callback(itr->entry);

		itr->pinger->ping(itr->pinger);
	}
}

void pingring_set_replied_callback(void (*callback)(reslist_t *))
{
	replied_callback = callback;
}

void pingring_set_timeout_callback(void (*callback)(reslist_t *))
{
	timeout_callback = callback;
}
