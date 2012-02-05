/*
 * See COPYRIGHTS file.
 */


#include <stdio.h>
#include <string.h>

#include <xia/dnds.h>
#include <xia/event.h>
#include <xia/hooklet.h>
#include <xia/journal.h>
#include <xia/utils.h>

#include "dbal.h"

/* Virtual functions */
static struct {

	int (*db_init)(const char *, const char *, const char *, const char *);
	int (*db_fini)();
	int (*db_ping)();

	/* DNDS_ACL */
	DNDS_ACL *(*acl_get)(const DNDS_ACL *);
	int (*acl_list)(DNDS_ACL *, void (*)(DNDS_ACL *));
	int (*acl_new)(const DNDS_ACL *);
	int (*acl_edit)(const DNDS_ACL *);
	int (*acl_clear)(const DNDS_ACL *);
	int (*acl_delete)(const DNDS_ACL *);
	int (*acl_src_group_member)(const DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
	int (*acl_src_group_not_member)(const DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
	int (*acl_src_group_map)(const DNDS_ACL *, const DNDS_ACL_GROUP *);
	int (*acl_src_group_unmap)(const DNDS_ACL *, const DNDS_ACL_GROUP *);
	int (*acl_src_host_member)(const DNDS_ACL *, void (*)(DNDS_HOST *));
	int (*acl_src_host_not_member)(const DNDS_ACL *, void (*)(DNDS_HOST *));
	int (*acl_src_host_map)(const DNDS_ACL *, const DNDS_HOST *);
	int (*acl_src_host_unmap)(const DNDS_ACL *, const DNDS_HOST *);
	int (*acl_dst_group_member)(const DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
	int (*acl_dst_group_not_member)(const DNDS_ACL *, void (*)(DNDS_ACL_GROUP *));
	int (*acl_dst_group_map)(const DNDS_ACL *, const DNDS_ACL_GROUP *);
	int (*acl_dst_group_unmap)(const DNDS_ACL *, const DNDS_ACL_GROUP *);
	int (*acl_dst_host_member)(const DNDS_ACL *, void (*)(DNDS_HOST *));
	int (*acl_dst_host_not_member)(const DNDS_ACL *, void (*)(DNDS_HOST *));
	int (*acl_dst_host_map)(const DNDS_ACL *, const DNDS_HOST *);
	int (*acl_dst_host_unmap)(const DNDS_ACL *, const DNDS_HOST *);

	/* DNDS_ACL_GROUP */
	DNDS_ACL_GROUP *(*acl_group_get)(const DNDS_ACL_GROUP *);
	int (*acl_group_list)(DNDS_ACL_GROUP *, void (*)(DNDS_ACL_GROUP *));
	int (*acl_group_new)(const DNDS_ACL_GROUP *);
	int (*acl_group_edit)(const DNDS_ACL_GROUP *);
	int (*acl_group_clear)(const DNDS_ACL_GROUP *);
	int (*acl_group_delete)(const DNDS_ACL_GROUP *);
	int (*acl_group_host_member)(const DNDS_ACL_GROUP *, void (*)(DNDS_HOST *));
	int (*acl_group_host_not_member)(const DNDS_ACL_GROUP *, void (*)(DNDS_HOST *));
	int (*acl_group_host_map)(const DNDS_ACL_GROUP *, const DNDS_HOST *);
	int (*acl_group_host_unmap)(const DNDS_ACL_GROUP *, const DNDS_HOST *);

	/* DNDS_ADDR_POOL */
	DNDS_ADDR_POOL *(*addr_pool_get)(const DNDS_ADDR_POOL *);
	int (*addr_pool_list)(void (*)(DNDS_ADDR_POOL *));
	int (*addr_pool_new)(const DNDS_ADDR_POOL *);
	int (*addr_pool_edit)(const DNDS_ADDR_POOL *);
	int (*addr_pool_clear)(const DNDS_ADDR_POOL *);
	int (*addr_pool_delete)(const DNDS_ADDR_POOL *);

	/* DNDS_CONTEXT */
	DNDS_CONTEXT *(*context_get)(const DNDS_CONTEXT *);
	int (*context_list)(void (*)(DNDS_CONTEXT *));
	int (*context_new)(const DNDS_CONTEXT *);
	int (*context_edit)(const DNDS_CONTEXT *);
	int (*context_clear)(const DNDS_CONTEXT *);
	int (*context_delete)(const DNDS_CONTEXT *);

	/* DNDS_HOST */
	DNDS_HOST *(*host_get)(const DNDS_HOST *);
	int (*host_list)(DNDS_HOST *, void (*)(DNDS_HOST *));
	int (*host_new)(const DNDS_HOST *);
	int (*host_edit)(const DNDS_HOST *);
	int (*host_clear)(const DNDS_HOST *);
	int (*host_delete)(const DNDS_HOST *);

	/* DNDS_NODE */
	DNDS_NODE *(*node_get)(const DNDS_NODE *);
	int (*node_list)(void (*)(DNDS_NODE *));
	int (*node_new)(const DNDS_NODE *);
	int (*node_edit)(const DNDS_NODE *);
	int (*node_clear)(const DNDS_NODE *);
	int (*node_delete)(const DNDS_NODE *);

	/* DNDS_PEER */
	DNDS_PEER *(*peer_get)(const DNDS_PEER *);
	int (*peer_list)(DNDS_PEER *, void (*)(DNDS_PEER *));
	int (*peer_new)(const DNDS_PEER *);
	int (*peer_edit)(const DNDS_PEER *);
	int (*peer_clear)(const DNDS_PEER *);
	int (*peer_delete)(const DNDS_PEER *);
	int (*peer_user_member)(const DNDS_PEER *, void (*)(DNDS_USER *));
	int (*peer_user_not_member)(const DNDS_PEER *, void (*)(DNDS_USER *));
	int (*peer_user_map)(const DNDS_PEER *, const DNDS_USER *);
	int (*peer_user_unmap)(const DNDS_PEER *, const DNDS_USER *);
	int (*peer_host_member)(const DNDS_PEER *, void (*)(DNDS_HOST *));
	int (*peer_host_not_member)(const DNDS_PEER *, void (*)(DNDS_HOST *));
	int (*peer_host_map)(const DNDS_PEER *, const DNDS_HOST *);
	int (*peer_host_unmap)(const DNDS_PEER *, const DNDS_HOST *);

	/* DNDS_PERM */
	DNDS_PERM *(*perm_get)(const DNDS_PERM *);
	int (*perm_list)(void (*)(DNDS_PERM *));
	int (*perm_new)(const DNDS_PERM *);
	int (*perm_edit)(const DNDS_PERM *);
	int (*perm_clear)(const DNDS_PERM *);
	int (*perm_delete)(const DNDS_PERM *);

	/* DNDS_USER */
	DNDS_USER *(*user_get)(const DNDS_USER *);
	int (*user_list)(DNDS_USER *, void (*)(DNDS_USER *));
	int (*user_new)(const DNDS_USER *);
	int (*user_edit)(const DNDS_USER *);
	int (*user_clear)(const DNDS_USER *);
	int (*user_delete)(const DNDS_USER *);

	/* We inherit the parent class */
	hooklet_t *hook;

} vfunc;

hooklet_cb_t dbal_cb[] = {

	{ "db_init",			CB(vfunc.db_init) },
	{ "db_fini",			CB(vfunc.db_fini) },
	{ "db_ping",			CB(vfunc.db_ping) },

	/* DNDS_ACL */
	{ "acl_get",			CB(vfunc.acl_get) },
	{ "acl_list",			CB(vfunc.acl_list) },
	{ "acl_new",			CB(vfunc.acl_new) },
	{ "acl_edit",			CB(vfunc.acl_edit) },
	{ "acl_clear",			CB(vfunc.acl_clear) },
	{ "acl_delete",			CB(vfunc.acl_delete) },
	{ "acl_src_group_member",	CB(vfunc.acl_src_group_member) },
	{ "acl_src_group_not_member",	CB(vfunc.acl_src_group_not_member) },
	{ "acl_src_group_map",		CB(vfunc.acl_src_group_map) },
	{ "acl_src_group_unmap",	CB(vfunc.acl_src_group_unmap) },
	{ "acl_src_host_member",	CB(vfunc.acl_src_host_member) },
	{ "acl_src_host_not_member",	CB(vfunc.acl_src_host_not_member) },
	{ "acl_src_host_map",		CB(vfunc.acl_src_host_map) },
	{ "acl_src_host_unmap",		CB(vfunc.acl_src_host_unmap) },
	{ "acl_dst_group_member",	CB(vfunc.acl_dst_group_member) },
	{ "acl_dst_group_not_member",	CB(vfunc.acl_dst_group_not_member) },
	{ "acl_dst_group_map",		CB(vfunc.acl_dst_group_map) },
	{ "acl_dst_group_unmap",	CB(vfunc.acl_dst_group_unmap) },
	{ "acl_dst_host_member",	CB(vfunc.acl_dst_host_member) },
	{ "acl_dst_host_not_member",	CB(vfunc.acl_dst_host_not_member) },
	{ "acl_dst_host_map",		CB(vfunc.acl_dst_host_map) },
	{ "acl_dst_host_unmap",		CB(vfunc.acl_dst_host_unmap) },

	/* DNDS_ACL_GROUP */
	{ "acl_group_get",		CB(vfunc.acl_group_get) },
	{ "acl_group_list",		CB(vfunc.acl_group_list) },
	{ "acl_group_new",		CB(vfunc.acl_group_new) },
	{ "acl_group_edit",		CB(vfunc.acl_group_edit) },
	{ "acl_group_clear",		CB(vfunc.acl_group_clear) },
	{ "acl_group_delete",		CB(vfunc.acl_group_delete) },
	{ "acl_group_host_member",	CB(vfunc.acl_group_host_member) },
	{ "acl_group_host_not_member",	CB(vfunc.acl_group_host_not_member) },
	{ "acl_group_host_map",		CB(vfunc.acl_group_host_map) },
	{ "acl_group_host_unmap",	CB(vfunc.acl_group_host_unmap) },

	/* DNDS_ADDR_POOL */
	{ "addr_pool_get",		CB(vfunc.addr_pool_get) },
	{ "addr_pool_list",		CB(vfunc.addr_pool_list) },
	{ "addr_pool_new",		CB(vfunc.addr_pool_new) },
	{ "addr_pool_edit",		CB(vfunc.addr_pool_edit) },
	{ "addr_pool_clear",		CB(vfunc.addr_pool_clear) },
	{ "addr_pool_delete",		CB(vfunc.addr_pool_delete) },

	/* DNDS_CONTEXT */
	{ "context_get",		CB(vfunc.context_get) },
	{ "context_list",		CB(vfunc.context_list) },
	{ "context_new",		CB(vfunc.context_new) },
	{ "context_edit",		CB(vfunc.context_edit) },
	{ "context_clear",		CB(vfunc.context_clear) },
	{ "context_delete",		CB(vfunc.context_delete) },

	/* DNDS_HOST */
	{ "host_get",			CB(vfunc.host_get) },
	{ "host_list",			CB(vfunc.host_list) },
	{ "host_new",			CB(vfunc.host_new) },
	{ "host_edit",			CB(vfunc.host_edit) },
	{ "host_clear",			CB(vfunc.host_clear) },
	{ "host_delete",		CB(vfunc.host_delete) },

	/* DNDS_NODE */
	{ "node_get",			CB(vfunc.node_get) },
	{ "node_list",			CB(vfunc.node_list) },
	{ "node_new",			CB(vfunc.node_new) },
	{ "node_edit",			CB(vfunc.node_edit) },
	{ "node_clear",			CB(vfunc.node_clear) },
	{ "node_delete",		CB(vfunc.node_delete) },

	/* DNDS_PEER */
	{ "peer_get",			CB(vfunc.peer_get) },
	{ "peer_list",			CB(vfunc.peer_list) },
	{ "peer_new",			CB(vfunc.peer_new) },
	{ "peer_edit",			CB(vfunc.peer_edit) },
	{ "peer_clear",			CB(vfunc.peer_clear) },
	{ "peer_delete",		CB(vfunc.peer_delete) },
	{ "peer_user_member",		CB(vfunc.peer_user_member) },
	{ "peer_user_not_member",	CB(vfunc.peer_user_not_member) },
	{ "peer_user_map",		CB(vfunc.peer_user_map) },
	{ "peer_user_unmap",		CB(vfunc.peer_user_unmap) },
	{ "peer_host_member",		CB(vfunc.peer_host_member) },
	{ "peer_host_not_member",	CB(vfunc.peer_host_not_member) },
	{ "peer_host_map",		CB(vfunc.peer_host_map) },
	{ "peer_host_unmap",		CB(vfunc.peer_host_unmap) },

	/* DNDS_PERM */
	{ "perm_get",			CB(vfunc.perm_get) },
	{ "perm_list",			CB(vfunc.perm_list) },
	{ "perm_new",			CB(vfunc.perm_new) },
	{ "perm_edit",			CB(vfunc.perm_edit) },
	{ "perm_clear",			CB(vfunc.perm_clear) },
	{ "perm_delete",		CB(vfunc.perm_delete) },

	/* DNDS_USER */
	{ "user_get",			CB(vfunc.user_get) },
	{ "user_list",			CB(vfunc.user_list) },
	{ "user_new",			CB(vfunc.user_new) },
	{ "user_edit",			CB(vfunc.user_edit) },
	{ "user_clear",			CB(vfunc.user_clear) },
	{ "user_delete",		CB(vfunc.user_delete) },

	{ NULL }
};

static inline int dbal_check_database()
{
	if (vfunc.db_ping()) {
		journal_notice("%s :: %s:%i\n",
		    "dbal]> database down, failed to reconnect",
		    __FILE__, __LINE__);
		return -1;
	}
	return 0;
}

/*
 * dbal_acl_get
 */
DNDS_ACL *dbal_acl_get(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return NULL;

	return vfunc.acl_get(acl);
}

/*
 * dbal_acl_list
 */
int dbal_acl_list(DNDS_ACL *acl, void (*cb)(DNDS_ACL *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_list(acl, cb);
}

/*
 * dbal_acl_new
 */
int dbal_acl_new(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_new(acl);
}

/*
 * dbal_acl_edit
 */
int dbal_acl_edit(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_edit(acl);
}

/*
 * dbal_acl_clear
 */
int dbal_acl_clear(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_clear(acl);
}

/*
 * dbal_acl_delete
 */
int dbal_acl_delete(DNDS_ACL *acl)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_delete(acl);
}

/*
 * dbal_acl_src_group_member
 */
int dbal_acl_src_group_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_group_member(acl, cb);
}

/*
 * dbal_acl_src_group_not_member
 */
int dbal_acl_src_group_not_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_group_not_member(acl, cb);
}

/*
 * dbal_acl_src_group_map
 */
int dbal_acl_src_group_map(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_group_map(acl, acl_group);
}

/*
 * dbal_acl_src_group_unmap
 */
int dbal_acl_src_group_unmap(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_group_unmap(acl, acl_group);
}

/*
 * dbal_acl_src_host_member
 */
int dbal_acl_src_host_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_host_member(acl, cb);
}

/*
 * dbal_acl_src_host_not_member
 */
int dbal_acl_src_host_not_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_host_not_member(acl, cb);
}

/*
 * dbal_acl_src_host_map
 */
int dbal_acl_src_host_map(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_host_map(acl, host);
}

/*
 * dbal_acl_src_host_unmap
 */
int dbal_acl_src_host_unmap(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_src_host_unmap(acl, host);
}

/*
 * dbal_acl_dst_group_member
 */
int dbal_acl_dst_group_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_group_member(acl, cb);
}

/*
 * dbal_acl_dst_group_not_member
 */
int dbal_acl_dst_group_not_member(DNDS_ACL *acl, void (*cb)(DNDS_ACL_GROUP *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_group_not_member(acl, cb);
}

/*
 * dbal_acl_dst_group_map
 */
int dbal_acl_dst_group_map(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_group_map(acl, acl_group);
}

/*
 * dbal_acl_dst_group_unmap
 */
int dbal_acl_dst_group_unmap(DNDS_ACL *acl, DNDS_ACL_GROUP *acl_group)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_group_unmap(acl, acl_group);
}

/*
 * dbal_acl_dst_host_member
 */
int dbal_acl_dst_host_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_host_member(acl, cb);
}

/*
 * dbal_acl_dst_host_not_member
 */
int dbal_acl_dst_host_not_member(DNDS_ACL *acl, void (*cb)(DNDS_HOST *))
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_host_not_member(acl, cb);
}

/*
 * dbal_acl_dst_host_map
 */
int dbal_acl_dst_host_map(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_host_map(acl, host);
}

/*
 * dbal_acl_dst_host_unmap
 */
int dbal_acl_dst_host_unmap(DNDS_ACL *acl, DNDS_HOST *host)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return -1;

	return vfunc.acl_dst_host_unmap(acl, host);
}

/*
 * dbal_acl_group_get
 */
DNDS_ACL_GROUP *dbal_acl_group_get(DNDS_ACL_GROUP *acl_group)
{
}

/*
 * dbal_acl_group_list
 */
int dbal_acl_group_list(DNDS_ACL_GROUP *acl_group, void (*cb)(DNDS_ACL_GROUP *))
{
}

/*
 * dbal_acl_group_new
 */
int dbal_acl_group_new(DNDS_ACL_GROUP *acl_group)
{
}

/*
 * dbal_acl_group_edit
 */
int dbal_acl_group_edit(DNDS_ACL_GROUP *acl_group)
{
}

/*
 * dbal_acl_group_clear
 */
int dbal_acl_group_clear(DNDS_ACL_GROUP *acl_group)
{
}

/*
 * dbal_acl_group_delete
 */
int dbal_acl_group_delete(DNDS_ACL_GROUP *acl_group)
{
}

/*
 * dbal_acl_group_host_member
 */
int dbal_acl_group_host_member(DNDS_ACL_GROUP *acl_group, void (*cb)(DNDS_HOST *))
{
}

/*
 * dbal_acl_group_host_not_member
 */
int dbal_acl_group_host_not_member(DNDS_ACL_GROUP *acl_group, void (*cb)(DNDS_HOST *))
{
}

/*
 * dbal_acl_group_host_map
 */
int dbal_acl_group_host_map(DNDS_ACL_GROUP *acl_group, DNDS_HOST *host)
{
}

/*
 * dbal_acl_group_host_unmap
 */
int dbal_acl_group_host_unmap(DNDS_ACL_GROUP *acl_group, DNDS_HOST *host)
{
}

/*
 * dbal_addr_pool_get
 */
DNDS_ADDR_POOL *dbal_addr_pool_get(DNDS_ADDR_POOL *addr_pool)
{
}

/*
 * dbal_addr_pool_list
 */
int dbal_addr_pool_list(void (*cb)(DNDS_ADDR_POOL *))
{
}

/*
 * dbal_addr_pool_new
 */
int dbal_addr_pool_new(DNDS_ADDR_POOL *addr_pool)
{
}

/*
 * dbal_addr_pool_edit
 */
int dbal_addr_pool_edit(DNDS_ADDR_POOL *addr_pool)
{
}

/*
 * dbal_addr_pool_clear
 */
int dbal_addr_pool_clear(DNDS_ADDR_POOL *addr_pool)
{
}

/*
 * dbal_addr_pool_delete
 */
int dbal_addr_pool_delete(DNDS_ADDR_POOL *addr_pool)
{
}

/*
 * dbal_context_get
 */
DNDS_CONTEXT *dbal_context_get(DNDS_CONTEXT *context)
{
}

/*
 * dbal_context_list
 */
int dbal_context_list(void (*cb)(DNDS_CONTEXT *))
{
}

/*
 * dbal_context_new
 */
int dbal_context_new(DNDS_CONTEXT *context)
{
}

/*
 * dbal_context_edit
 */
int dbal_context_edit(DNDS_CONTEXT *context)
{
}

/*
 * dbal_context_clear
 */
int dbal_context_clear(DNDS_CONTEXT *context)
{
}

/*
 * dbal_context_delete
 */
int dbal_context_delete(DNDS_CONTEXT *context)
{
}

/*
 * dbal_host_get
 */
DNDS_HOST *dbal_host_get(DNDS_HOST *host)
{
}

/*
 * dbal_host_list
 */
int dbal_host_list(DNDS_HOST *host, void (*cb)(DNDS_HOST *))
{
}

/*
 * dbal_host_new
 */
int dbal_host_new(DNDS_HOST *host)
{
}

/*
 * dbal_host_edit
 */
int dbal_host_edit(DNDS_HOST *host)
{
}

/*
 * dbal_host_clear
 */
int dbal_host_clear(DNDS_HOST *host)
{
}

/*
 * dbal_host_delete
 */
int dbal_host_delete(DNDS_HOST *host)
{
}

/*
 * dbal_node_get
 */
DNDS_NODE *dbal_node_get(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	if (dbal_check_database())
		return NULL;

	return vfunc.node_get(node);
}

/*
 * dbal_node_list
 */
int dbal_node_list(void (*cb)(DNDS_NODE *))
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.node_list(cb);
}

/*
 * dbal_node_new
 */
int dbal_node_new(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.node_new(node);
}

/*
 * dbal_node_edit
 */
int dbal_node_edit(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.node_edit(node);
}

/*
 * dbal_node_clear
 */
int dbal_node_clear(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.node_clear(node);
}

/*
 * dbal_node_delete
 */
int dbal_node_delete(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.node_delete(node);
}

/*
 * dbal_peer_get
 */
DNDS_PEER *dbal_peer_get(DNDS_PEER *peer)
{
}

/*
 * dbal_peer_list
 */
int dbal_peer_list(DNDS_PEER *peer, void (*cb)(DNDS_PEER *))
{
}

/*
 * dbal_peer_new
 */
int dbal_peer_new(DNDS_PEER *peer)
{
}

/*
 * dbal_peer_edit
 */
int dbal_peer_edit(DNDS_PEER *peer)
{
}

/*
 * dbal_peer_clear
 */
int dbal_peer_clear(DNDS_PEER *peer)
{
}

/*
 * dbal_peer_delete
 */
int dbal_peer_delete(DNDS_PEER *peer)
{
}

/*
 * dbal_peer_user_member
 */
int dbal_peer_user_member(DNDS_PEER *peer, void (*cb)(DNDS_USER *))
{
}

/*
 * dbal_peer_user_not_member
 */
int dbal_peer_user_not_member(DNDS_PEER *peer, void (*cb)(DNDS_USER *))
{
}

/*
 * dbal_peer_user_map
 */
int dbal_peer_user_map(DNDS_PEER *peer, DNDS_USER *user)
{
}

/*
 * dbal_peer_user_unmap
 */
int dbal_peer_user_unmap(DNDS_PEER *peer, DNDS_USER *user)
{
}

/*
 * dbal_peer_host_member
 */
int dbal_peer_host_member(DNDS_PEER *peer, void (*cb)(DNDS_HOST *))
{
}

/*
 * dbal_peer_host_not_member
 */
int dbal_peer_host_not_member(DNDS_PEER *peer, void (*cb)(DNDS_HOST *))
{
}

/*
 * dbal_peer_host_map
 */
int dbal_peer_host_map(DNDS_PEER *peer, DNDS_HOST *host)
{
}

/*
 * dbal_peer_host_unmap
 */
int dbal_peer_host_unmap(DNDS_PEER *peer, DNDS_HOST *host)
{
}

/*
 * dbal_perm_get
 */
DNDS_PERM *dbal_perm_get(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return NULL;

	return vfunc.perm_get(perm);
}

/*
 * dbal_perm_get_by_node
 */
DNDS_PERM *dbal_perm_get_by_node(DNDS_NODE *node)
{
	journal_ftrace(__func__);

	DNDS_PERM perm;

	if(dbal_check_database())
		return NULL;

	memset(&perm, 0, sizeof(DNDS_PERM));
	perm.id = node->perm;

	return vfunc.perm_get(&perm);
}

/*
 * dbal_perm_list
 */
int dbal_perm_list(void (*cb)(DNDS_PERM *))
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.perm_list(cb);
}

/*
 * dbal_perm_new
 */
int dbal_perm_new(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.perm_new(perm);
}

/*
 * dbal_perm_edit
 */
int dbal_perm_edit(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.perm_edit(perm);
}

/*
 * dbal_perm_clear
 */
int dbal_perm_clear(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.perm_clear(perm);
}

/*
 * dbal_perm_delete
 */
int dbal_perm_delete(DNDS_PERM *perm)
{
	journal_ftrace(__func__);

	if(dbal_check_database())
		return -1;

	return vfunc.perm_delete(perm);
}

/*
 * dbal_user_get
 */
DNDS_USER *dbal_user_get(DNDS_USER *user)
{
}

/*
 * dbal_user_list
 */
int dbal_user_list(DNDS_USER *user, void (*cb)(DNDS_USER *))
{
}

/*
 * dbal_user_new
 */
int dbal_user_new(DNDS_USER *user)
{
}

/*
 * dbal_user_edit
 */
int dbal_user_edit(DNDS_USER *user)
{
}

/*
 * dbal_user_clear
 */
int dbal_user_clear(DNDS_USER *user)
{
}

/*
 * dbal_user_delete
 */
int dbal_user_delete(DNDS_USER *user)
{
}

/*
 * dbal_fini
 */
void dbal_fini()
{
	vfunc.db_fini();
	return;
}

/*
 * XXX - should dbal_init() abstract the arguments and let the dbal hooklets
 * choose whatever parameters they need ? ex. storing information in a binary
 * file only requires to specify the file path, no host, no username,
 * no password, not even a database name. only a filename.
 * This is only an example.
 *
 * Sounds good ?
 */
int dbal_init(const char *db_host, const char *db_usr,
	      const char *db_pwd, const char *db_name)
{
	journal_ftrace(__func__);

	vfunc.hook = hooklet_inherit(HOOKLET_DBAL);
	if (vfunc.hook == NULL) {
		journal_notice("%s :: %s:%i\n",
		    "No hooklet available to inherit "
		    "the DBAL subsystem", __FILE__, __LINE__);
		return -1;
	}

	if (hooklet_map_cb(vfunc.hook, dbal_cb))
		return -1;
	
	if (vfunc.db_init(db_host, db_usr, db_pwd, db_name) != 0)
		return -1;

	event_register(EVENT_EXIT, "dbal:dbal_fini()",
	    dbal_fini, PRIO_AGNOSTIC);

	return 0;
}
