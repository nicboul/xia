/*
 * See COPYRIGHTS file.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <xia/event.h>
#include <xia/hooklet.h>
#include <xia/journal.h>
#include <xia/utils.h>

#include "acl.h"
#include "dbal.h"
#include "muxia.h"

void acl_fini()
{
	journal_ftrace(__func__);
}

int acl_init()
{
	journal_ftrace(__func__);

	/*mem_fun.hook = hooklet_inherit(HOOKLET_ACL);
	if (mem_fun.hook == NULL) {
		journal_notice("%s :: %s:%i\n",
			"No hooklet available to inherit "
			"the ACL subsystem", __FILE__, __LINE__);
		return -1;
	}

	if (hooklet_map_cb(mem_fun.hook, acl_cb))
		return -1;

	mem_fun.set_init();
	muxia_register(&acl_demux, XIAMSG_ACL);
	event_register(EVENT_EXIT, "acl:acl_fini()", acl_fini, PRIO_AGNOSTIC);
	*/

	return 0;
}
