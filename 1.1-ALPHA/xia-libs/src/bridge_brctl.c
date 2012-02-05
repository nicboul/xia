/*
 * See COPYRIGHTS file.
 */


#include <stdlib.h>

#include "bridge.h"
#include "netbus.h"

extern int bridge_add(bridge_t *bridge, iface_t *iface)
{

	journal_ftrace(__func__);

	char sys[128];
	int ret;

//	snprintf(sys, 128

	journal_notice("bridge]> sys> %s\n", sys);
	ret = system(sys);

	return ret;
}

extern bridge_t *bridge_create()
{
	journal_ftrace(__func__);

	char sys[128];
	int ret;
	bridge_t *bridge;

	bridge = malloc(sizeof(bridge_t));
	bridge->devname = strdup("bridge0");

	// snprintf(sys, 128, ...

	journal_notice("bridge]> sys> %s\n", sys);
	ret = system(sys);

	return bridge;
}
