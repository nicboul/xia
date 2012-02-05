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

extern int pingring_add(reslist_t *);
extern int pingring_del(reslist_t *);
extern void pingring_show();
extern void pingring_ping();
extern void pingring_set_replied_callback(void (*)(reslist_t *));
extern void pingring_set_timeout_callback(void (*)(reslist_t *));
extern int pingring_init();

#endif /* PINGRING_H */
