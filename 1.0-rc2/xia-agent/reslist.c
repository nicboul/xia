
/*
 * See COPYRIGHTS file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dhcp.h"	// DHCP_HLEN
#include "reslist.h"

#include "../lib/hash.h"
#include "../lib/journal.h"

#define RESLIST_HKEY_NUM 2
uint32_t reslist_hash(const uint8_t *chaddr)
{
	static uint32_t salt = 0;
	uint32_t result;
	uint32_t key[RESLIST_HKEY_NUM] = {0};

	journal_ftrace(__func__);

	memcpy(key, chaddr, DHCP_HLEN);

	if (salt == 0)
		salt = rand();

	result = hashword(key, RESLIST_HKEY_NUM, salt);

	return result % RESLIST_SIZE;
}

reslist_t *reslist_add(reslist_t **rlist, const struct dhcp_msg *dhcp)
{
	reslist_t *entry = NULL;
	uint32_t pos;

	journal_ftrace(__func__);
	
	pos = reslist_hash(dhcp->chaddr);

	for (entry = rlist[pos]; entry != NULL; entry = entry->next) {
		if (memcmp(entry->chaddr, dhcp->chaddr, DHCP_HLEN) == 0) {
			if (entry->xid == dhcp->xid) {
				journal_notice("%s %p :: %s:%i\n",
				    "found entry at", entry,
				    __FILE__, __LINE__);
				return entry;
			}
		}
	}

	entry = malloc(sizeof(reslist_t));
	if (entry == NULL)
		return NULL;

	journal_notice("%s %p :: %s:%i\n", "created entry at", entry,
	    __FILE__, __LINE__);

	memset(entry, 0, sizeof(reslist_t));

	entry->xid = dhcp->xid;
	memcpy(entry->chaddr, dhcp->chaddr, DHCP_HLEN);

	if (rlist[pos]) { 
		entry->next = rlist[pos];
		rlist[pos]->prev = entry;
	}

	rlist[pos] = entry; // we are the new head

	return entry;
}

void reslist_del(reslist_t **rlist, reslist_t *entry)
{
	uint32_t pos;

	journal_ftrace(__func__);

	if (entry->next)
		entry->next->prev = entry->prev;
	if (entry->prev) {
		entry->prev->next = entry->next;
	} else {
		pos = reslist_hash(entry->chaddr);
		rlist[pos] = entry->next;
	}

	free(entry);
}

int reslist_purge(reslist_t **rlist,
		  int (*purge)(reslist_t *),
		  const struct dhcp_msg *dhcp)
{
	reslist_t *itr, *head;
	int count = 0;

	journal_ftrace(__func__);

	head = rlist[reslist_hash(dhcp->chaddr)];

	for (itr = head; itr != NULL; itr = itr->next) {
		if (memcmp(itr->chaddr, dhcp->chaddr, DHCP_HLEN))
			continue;

		if (itr->xid != dhcp->xid) {
			journal_notice("%s %p %s :: %s:%i\n",
			    "entry at", itr, "is to purge",
			    __FILE__, __LINE__);

			purge(itr);

			itr = head;
			count++;
		}
	}

	return count;
}
