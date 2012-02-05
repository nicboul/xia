/*
 * See COPYRIGHTS file.
 */
#ifndef XIA_BRIDGE_H
#define XIA_BRIDGE_H

#include "netbus.h"

/* Bridge Device */

typedef struct bridge {
	char *devname;
} bridge_t;

int bridge_destroy(bridge_t *);
int bridge_add(bridge_t *, iface_t *);
bridge_t *bridge_create();

#endif /* XIA_BRIDGE_H */
