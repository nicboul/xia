
/*
 * See COPYRIGHTS file.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include <xia/xiap.h>
#include <xia/discovery_msg.h>

#include "../lib/event.h"
#include "../lib/journal.h"
#include "../lib/utils.h"

#include "dbal.h"
#include "discovery.h"
#include "muxia.h"

void discovery_msg_print(struct discovery_msg *msg)
{
	journal_ftrace(__func__);

	journal_notice("discovery msg version : %u\n", msg->version);
	journal_notice("discovery msg type    : %u\n", msg->type);
	journal_notice("discovery msg name    : %s\n", msg->name);
	journal_notice("discovery msg haddr   : %s\n", msg->haddr);
	journal_notice("discovery msg addr    : %s\n", inet_ntoa(msg->addr));
	journal_notice("discovery msg hostname: %s\n", msg->hostname);
}

struct muxia_info *discovery_demux(void *msgp)
{
	journal_ftrace(__func__);

	static struct muxia_info mux;
	struct discovery_msg *msg = msgp;
	char *addr;

	muxia_reply_init(mux, sizeof(struct discovery_msg));

	if (msg == NULL) {
		mux.ret = XIAP_STATUS_UNEXPECTED;
		return &mux;
	}

	discovery_msg_print(msg);

	switch (msg->type) {

		case DISCOVERY_MSG_SET_RES_ADDR:
			dbal_set_res_addr_mapping(msg->name, msg->haddr,
			    inet_ntoa(msg->addr));
			break;

		case DISCOVERY_MSG_SET_RES_HOSTNAME:
			if (strnlen(msg->hostname, sizeof(msg->hostname)) == 0)
				snprintf(msg->hostname, sizeof(msg->hostname),
				    DISCOVERY_SET_RES_HOSTNAME_DEFAULT);
			dbal_set_res_hostname_mapping(msg->name,
			    msg->haddr, msg->hostname);
			break;

		case DISCOVERY_MSG_SET_RES_HOST_DEAD:
			dbal_set_res_status(msg->name, msg->haddr, 0);
			break;

		case DISCOVERY_MSG_SET_RES_HOST_ALIVE:
			dbal_set_res_status(msg->name, msg->haddr, 1);
			break;
	}

	msg->version = DISCOVERY_MSG_VERSION;
	mux.msgp = (void *)msg;
	return &mux;
}

void discovery_fini()
{
	journal_ftrace(__func__);
}

int discovery_init()
{
	journal_ftrace(__func__);

	muxia_register(discovery_demux, XIAMSG_DISCOVERY);
	event_register(EVENT_EXIT, "discovery:discovery_fini()", discovery_fini, PRIO_AGNOSTIC);	

	return 0;
}
