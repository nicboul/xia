/*
 * Copyright 2008 Mind4Networks Technologies INC.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include <xia/xiap.h>
#include <xia/acl_msg.h>
#include <xia/session_msg.h>

#include "../lib/event.h"
#include "../lib/journal.h"
#include "../lib/utils.h"

#include "dbal.h"
#include "muxia.h"

static uint8_t session_acl_exec(const struct session_msg *sm, uint8_t type)
{
	journal_strace("session_acl_exec");

	struct acl_msg aclm;
	struct muxia_info *mux;

	memset(&aclm, 0, sizeof(struct acl_msg));

	aclm.type = type;
	aclm.version = ACL_MSG_VERSION;
	aclm.addr.s_addr = sm->addr.s_addr;
	strncpy((char *)&aclm.name, sm->name, XIAP_NAMESIZ);

	mux = muxia_exec(XIAMSG_ACL, (void *)&aclm);

	return mux->ret;
}

static void session_msg_print(struct session_msg *sm)
{
	journal_strace("session_msg_print");

	journal_notice("session msg version : %u\n", sm->version);
	journal_notice("session msg type    : %u\n", sm->type);
	journal_notice("session msg name    : %s\n", sm->name);
}

static struct session_msg *session_reply(struct session_msg *sm, uint8_t type)
{
	journal_strace("session_reply");

	/* XXX - using the old session message */
	sm->version = SESSION_MSG_VERSION;
	sm->type = type;
	return sm;
}

static uint8_t session_down(struct session_msg *sm, struct muxia_info *mux)
{
	journal_strace("session_down");

	/*
	 * Phase 1 - unset_token_addr_mapping()
	 */
	dbal_unset_token_addr_mapping(inet_ntoa(sm->addr));

	/*
	 * Final - notify ACL subsystem
	 */
	return session_acl_exec(sm, ACL_MSG_DEL);
}

static uint8_t session_up(struct session_msg *sm, struct muxia_info *mux)
{
	journal_strace("session_up");

	int_vector *status;
	uint8_t ret;

	/*
	 * Phase 1 - check token status
	 */
	status = dbal_get_token_status(sm->name);

	if (status[VECTOR_IDX_BEGIN] != SESSION_STATUS_ENABLED) {
		journal_notice("user %s, access denied :: %s:%i\n",
			sm->name, __FILE__, __LINE__);

		mux->msgp = (void *)session_reply(sm, SESSION_MSG_DOWN);
		return XIAP_STATUS_OK;
	}

	/*
	 * Phase 2 - set_token_addr_mapping()
	 */
	dbal_set_token_addr_mapping(sm->name, inet_ntoa(sm->addr));

	/*
	 * Final - notify ACL subsystem
	 */
	ret = session_acl_exec(sm, ACL_MSG_ADD);
		
	if (ret != XIAP_STATUS_OK) {
		session_down(sm, mux);
		return ret;
	}

	mux->msgp = (void *)session_reply(sm, SESSION_MSG_UP);
	return XIAP_STATUS_OK;
}

static struct muxia_info *session_demux(void *msgp)
{
	journal_strace("session_demux");

	static struct muxia_info mux;
	struct session_msg *sm = msgp;

	memset(&mux, 0, sizeof(struct muxia_info));
	mux.msgp_size = sizeof(struct session_msg);

	if (sm == NULL) {
		mux.ret = XIAP_STATUS_UNEXPECTED;
		return &mux;
	}

	switch (sm->type) {
		case SESSION_MSG_UP:
			mux.ret = session_up(sm, &mux);
			break;

		case SESSION_MSG_DOWN:
			mux.ret = session_down(sm, &mux);
			break;

		default:
			journal_notice("%s :: %s:%i\n",
				"wrong msg type", __FILE__, __LINE__);

			mux.ret = XIAP_STATUS_FAILED;
			break;
	}

	return &mux;
}

static void session_fini()
{
	journal_strace("session_fini");
}

int session_init()
{
	journal_strace("session_init");

	muxia_register(&session_demux, XIAMSG_SESSION);
	event_register(EVENT_EXIT, "session:session_fini()", session_fini, PRIO_AGNOSTIC);	

	return 0;
}
