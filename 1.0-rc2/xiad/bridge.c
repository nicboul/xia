
/*
 * See COPYRIGHTS file.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <xia/xiap.h>
#include <xia/bridge_msg.h>

#include "../lib/event.h"
#include "../lib/journal.h"
#include "../lib/utils.h"

#include "dbal.h"
#include "muxia.h"

void bridge_msg_print(struct bridge_msg *bm)
{
	journal_ftrace(__func__);

	journal_notice("bridge msg version : %u\n", bm->version);
	journal_notice("bridge msg name    : %s\n", bm->name);
	journal_notice("bridge msg owned by: %u\n", bm->owned_by);
}

struct muxia_info *bridge_demux(void *msgp)
{
	journal_ftrace(__func__);

	static struct muxia_info mux;
	struct bridge_msg *bm = msgp;

	unsigned int context = 0;

	int_vector *vector;

	memset(&mux, 0, sizeof(struct muxia_info));
	mux.msgp_size = sizeof(struct bridge_msg);

	if (bm == NULL) {
		mux.ret = XIAP_STATUS_UNEXPECTED;
		return &mux;
	}

	bridge_msg_print(bm);

	vector = dbal_get_bridge_owner_context(bm->name);
	context = vector[VECTOR_IDX_BEGIN];
	free(vector);

	if (context == 0)
		journal_notice("%s %s :: %s:%i\n",
			bm->name, "is orphaned!", __FILE__, __LINE__);

	bm->owned_by = context;
	bm->version = BRIDGE_MSG_VERSION;

	mux.msgp = (void *)bm;
	return &mux;
}

void bridge_fini()
{
	journal_ftrace(__func__);
}

int bridge_init()
{
	journal_ftrace(__func__);

	muxia_register(bridge_demux, XIAMSG_BRIDGE);
	event_register(EVENT_EXIT, "bridge:bridge_fini()", bridge_fini, PRIO_AGNOSTIC);	

	return 0;
}


