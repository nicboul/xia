#ifndef PINGRING_H
#define PINGRING_H

#include <time.h>

#include "../lib/netbus.h"

#include "reslist.h"

typedef struct pingring {

	struct pingring *next;

	peer_t *pinger;
	reslist_t *entry;

} pingring_t;

extern int pingring_add(pingring_t **, reslist_t *);
extern int pingring_del(pingring_t **, reslist_t *);
extern void pingring_show(pingring_t *);
extern void pingring_ping(pingring_t **);
extern void pingring_set_replied_callback(void (*)(reslist_t *));
extern void pingring_set_timeout_callback(void (*)(reslist_t *));

#endif /* PINGRING_H */
