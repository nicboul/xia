/*
 * See COPYRIGHTS file.
 */


#include <stdlib.h>

#include "bridge.h"
#include "netbus.h"

extern int bridge_destroy(bridge_t *bridge)
{
	journal_ftrace(__func__);

	char sys[128];
	int ret;

	snprintf(sys, 128, "%s %s %s",
		"ifconfig",
		bridge->devname,
		"destroy");

	journal_notice("bridge]> sys:: %s\n", sys);
	ret = system(sys);

	return ret;
}

extern int bridge_add(bridge_t *bridge, iface_t *iface)
{
	journal_ftrace(__func__);
	
	char sys[128];
	int ret;

	snprintf(sys, 128, "%s %s %s %s",
		"brconfig",
		bridge->devname,
		"add",
		iface->devname);

	journal_notice("bridge]> sys:: %s\n", sys);
	ret = system(sys);

	return ret;
}

extern bridge_t *bridge_create(int idx)
{
	journal_ftrace(__func__);

	char sys[128];
	int ret;
	bridge_t *bridge;

	bridge = malloc(sizeof(bridge_t));
	bridge->devname = malloc(9);
	snprintf(bridge->devname, 9, "bridge%i", idx);

	snprintf(sys, 128, "%s %s %s",
		"ifconfig",
		bridge->devname,
		"up");

	journal_notice("bridge]> sys:: %s\n", sys);
	ret = system(sys);

	return bridge;
}
