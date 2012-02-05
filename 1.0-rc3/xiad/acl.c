
/*
 * See COPYRIGHTS file.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <xia/xiap.h>
#include <xia/acl_msg.h>
#include <xia/bridge_msg.h>

#include "../lib/event.h"
#include "../lib/hooklet.h"
#include "../lib/journal.h"
#include "../lib/utils.h"

#include "acl.h"
#include "dbal.h"
#include "muxia.h"

enum {
	ACL_CATEGORY_USR,
	ACL_CATEGORY_RES,
	ACL_CATEGORY_ROOT
};

/* Use an 8 bits-vector table to know the state of each set.
 * Related bv_* functions can be found inside xiad/utils.c
 *
 * Change this value to support more than 65 536 (8192 * 8) state bits.
 */
#define ACLSETSIZ 8192
uint8_t set_states[ACLSETSIZ];
int acl_set_create(const unsigned int);

/* Hooklets with HOOKLET_ACL scope must implement these functions */

static struct {

	int (*usr_add)(const char *, const unsigned int);
	int (*usr_del)(const char *, const unsigned int);

	int (*res_add)(const char *, const unsigned int);
	int (*res_del)(const char *, const unsigned int);

	int (*set_create)(const unsigned int);
	int (*set_destroy)(const unsigned int);

	int (*set_init)(void);
	int (*set_fini)(void);

	/* We inherit the parent class */
	hooklet_t *hook;

} mem_fun;

hooklet_cb_t acl_cb[] = {

	{ "acl_usr_add",	CB(mem_fun.usr_add) },
	{ "acl_usr_del",	CB(mem_fun.usr_del) },
	{ "acl_res_add",	CB(mem_fun.res_add) },
	{ "acl_res_del",	CB(mem_fun.res_del) },
	{ "acl_set_create",	CB(mem_fun.set_create) },
	{ "acl_set_destroy",	CB(mem_fun.set_destroy) },
	{ "acl_set_init", 	CB(mem_fun.set_init) },
	{ "acl_set_fini", 	CB(mem_fun.set_fini) },
	
	{ NULL }
};


unsigned int acl_get_context(const char *name)
{
	journal_ftrace(__func__);

	struct bridge_msg bm, *reply;
	struct muxia_info *mux;

	memset(&bm, 0, sizeof(bm));

	bm.version = BRIDGE_MSG_VERSION;
	strncpy((char *)&bm.name, name, XIAP_NAMESIZ);

	mux = muxia_exec(XIAMSG_BRIDGE, (void *)&bm);

	if (mux->ret != XIAP_STATUS_OK)
		return 0;

	reply = (struct bridge_msg *)mux->msgp;

	return reply->owned_by;
}

int acl_swap_context(char *name)
{
	journal_ftrace(__func__);
	return swap_context(name, acl_get_context(name));
}

uint8_t acl_exec(struct acl_msg *aclm, uint8_t category)
{
	journal_ftrace(__func__);

	int (*exec)(const char *, const unsigned int);
	int_vector *set_list;
	uint8_t ret = XIAP_STATUS_OK;

	unsigned int i;
	char *addr;

	addr = inet_ntoa(aclm->addr);

	switch (category) {
		case ACL_CATEGORY_USR:
			set_list = dbal_get_usr_membership(aclm->name);
			if (set_list == NULL) {
				journal_notice("acl]> dbal_get_usr_membership(%s) failed :: %s:%i\n", aclm->name, __FILE__, __LINE__);
				return XIAP_STATUS_FAILED;
			}

			exec = (aclm->type == ACL_MSG_ADD) ?
			    mem_fun.usr_add : mem_fun.usr_del;

			break;

		case ACL_CATEGORY_RES:
			set_list = dbal_get_res_membership(aclm->name);
			if (set_list == NULL) {
				journal_notice("acl]> dbal_get_res_membership(%s) failed :: %s:%i\n", aclm->name, __FILE__, __LINE__);
				return XIAP_STATUS_FAILED;
			}

			exec = (aclm->type == ACL_MSG_ADD) ?
			    mem_fun.res_add : mem_fun.res_del;

			break;

		case ACL_CATEGORY_ROOT:
			ret = (aclm->type == ACL_MSG_ADD) ?
			    mem_fun.usr_add(addr, 0) : mem_fun.usr_del(addr, 0);

			return (ret) ? XIAP_STATUS_RETRY : XIAP_STATUS_OK;

		default:
			journal_notice("%s :: %s:%i\n",
			    "invalid category given in switch statement",
			    __FILE__, __LINE__);
			return XIAP_STATUS_FAILED;
	}

	if (set_list == NULL) {
		journal_notice("%s :: %s:%i\n",
		    "set_list is NULL. normal?", __FILE__, __LINE__);
		return XIAP_STATUS_OK;
	}

	for (i = VECTOR_IDX_BEGIN; i < set_list[VECTOR_IDX_SIZE]; ++i) {

		if (acl_set_create(set_list[i])) {
			ret = XIAP_STATUS_RETRY;
			break;
		}

		if (exec(addr, set_list[i])) {
			journal_notice("%s :: %s:%i\n",
				"hooklet failed", __FILE__, __LINE__);

			ret = XIAP_STATUS_RETRY;
			break;
		}
	}

	free(set_list);
	return ret;
}

void acl_print_msg(struct acl_msg *aclm)
{
	journal_ftrace(__func__);

	journal_notice("acl version : %u\n", aclm->version);
	journal_notice("acl type    : %u\n", aclm->type);
	journal_notice("acl addr    : %s\n", inet_ntoa(aclm->addr));
	journal_notice("acl name    : %s\n", aclm->name);
}

struct muxia_info *acl_demux(void *msgp)
{
	journal_ftrace(__func__);

	static struct muxia_info mux;
	struct acl_msg *aclm = msgp;

	int_vector *dbal_data;
	uint8_t node_type;

	memset(&mux, 0, sizeof(struct muxia_info));
	mux.msgp_size = sizeof(struct acl_msg);

	if (aclm == NULL) {
		mux.ret = XIAP_STATUS_UNEXPECTED;
		return &mux;
	}
	acl_print_msg(aclm);

	dbal_data = dbal_get_token_type(aclm->name);
	if (dbal_data == NULL) {
		journal_notice("acl]> dbal_get_token_type() failed :: %s:%i\n", __FILE__, __LINE__);
		mux.ret = XIAP_STATUS_FAILED;
		return &mux;
	}

	node_type = dbal_data[VECTOR_IDX_BEGIN];
	free(dbal_data);

	switch (node_type) {
		case XIA_NODE_TYPE_USR:
			mux.ret = acl_exec(aclm, ACL_CATEGORY_USR);
			break;

		case XIA_NODE_TYPE_BRIDGE_USR:
			/*
 			 * To comply with database design, we need to swap
 			 * the context with the one in owned_by field
 			 */
			if (acl_swap_context(aclm->name)) {
				journal_notice("%s :: %s:%i\n",
				    "acl_swap_context failed",
				    __FILE__, __LINE__);

				mux.ret = XIAP_STATUS_FAILED; // retry instead?
				break;
			}
			
			mux.ret = acl_exec(aclm, ACL_CATEGORY_USR);
			break;

		case XIA_NODE_TYPE_RES:
			mux.ret = acl_exec(aclm, ACL_CATEGORY_RES);
			break;

		case XIA_NODE_TYPE_BRIDGE_RES:
		case XIA_NODE_TYPE_TUNNEL:
			mux.ret = acl_exec(aclm, ACL_CATEGORY_ROOT);
			break;

		default:
			journal_notice("%s :: %s:%i\n",
				"wrong node type", __FILE__, __LINE__);
			mux.ret = XIAP_STATUS_FAILED;
			break;
	}

	return &mux;
}

int acl_set_create(const unsigned int set)
{
	journal_ftrace(__func__);

	/*
	 * acl set 0 is a special set initialized by acl_set_init()
	 */
	if (set > 0 && !bv_test(set, set_states, ACLSETSIZ)) {

		// sanitize
		journal_notice("---------failure is normal---------\n");
		mem_fun.set_destroy(set);
		journal_notice("-----------------------------------\n");

		if (mem_fun.set_create(set)) {
			// XXX - log_notice_msg does not work here ? */
			journal_notice("%s %u :: %s:%i\n",
				"unable to initialize acl set",
				set, __FILE__, __LINE__);
			return 1;
		}

		bv_set(set, set_states, ACLSETSIZ);
	}

	return 0;
}

void acl_fini()
{
	journal_ftrace(__func__);

	unsigned int i;

	for (i = 1; i < ACLSETSIZ; ++i)
		if (bv_test(i, set_states, ACLSETSIZ))
			mem_fun.set_destroy(i);

	mem_fun.set_fini();
}

int acl_init()
{
	journal_ftrace(__func__);

	mem_fun.hook = hooklet_inherit(HOOKLET_ACL);
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

	return 0;
}
