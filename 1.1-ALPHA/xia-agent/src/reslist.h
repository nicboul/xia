#ifndef RESLIST_H
#define RESLIST_H

#include <time.h>

#include <netinet/in.h>

#include <xia/xiap.h>
#include <xia/msg/discovery.h>

#include "dhcp.h" // XXX - tightly bound to the dhcp message

#ifndef RESLIST_SIZE
# define RESLIST_SIZE 256
#endif

typedef struct reslist {

	struct reslist *next, *prev;

	uint32_t xid;
	uint8_t chaddr[DHCP_HLEN];

	struct in_addr addr;
	char hostname[XIA_MSG_DISCOVERY_HOSTNAMESIZ];

	time_t lastseen;
	time_t timeout;

	uint8_t dhcp_state;	/* DHCP handshake */

	void *msg_queue;

} reslist_t;

extern reslist_t *reslist_add(const struct dhcp_msg *);
extern void reslist_del(reslist_t *);
extern int reslist_purge(const struct dhcp_msg *);


#endif /* RESLIST_H */
