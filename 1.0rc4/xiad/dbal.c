
/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>

#include "../lib/event.h"
#include "../lib/hooklet.h"
#include "../lib/journal.h"
#include "../lib/utils.h"

#include "dbal.h"

static struct {

	int (*db_init)(const char *host, const char *user,
	    const char *passwd, const char *dbname);
	int (*db_fini)();
	int (*db_ping)();

	int_vector *(*get_token_type)(const char *name);
	int_vector *(*get_token_status)(const char *name);
	int_vector *(*get_res_membership)(const char *name);
	int_vector *(*get_usr_membership)(const char *name);
	int_vector *(*get_bridge_owner_context)(const char *name);
	int (*set_token_addr_mapping)(const char *name, const char *addr);
	int (*unset_token_addr_mapping)(const char *addr);
	int (*set_res_addr_mapping)(const char *name,
	    const char *haddr, const char *addr);
	int (*set_res_hostname_mapping)(const char *name,
	    const char *addr, const char *hostname);
	int (*set_res_status)(const char *name,
	    const char *addr, const unsigned int status);


	/* We inherit the parent class */
	hooklet_t *hook;

} mem_fun;

hooklet_cb_t dbal_cb[] = {

	{ "db_init",			CB(mem_fun.db_init) },
	{ "db_fini",			CB(mem_fun.db_fini) },
	{ "db_ping",			CB(mem_fun.db_ping) },
	{ "get_token_type",		CB(mem_fun.get_token_type) },
	{ "get_token_status",		CB(mem_fun.get_token_status) },
	{ "get_res_membership",		CB(mem_fun.get_res_membership) },
	{ "get_usr_membership",		CB(mem_fun.get_usr_membership) },
	{ "get_bridge_owner_context",	CB(mem_fun.get_bridge_owner_context) },
	{ "set_token_addr_mapping",	CB(mem_fun.set_token_addr_mapping) },
	{ "unset_token_addr_mapping",	CB(mem_fun.unset_token_addr_mapping) },
	{ "set_res_addr_mapping",	CB(mem_fun.set_res_addr_mapping) },
	{ "set_res_hostname_mapping",	CB(mem_fun.set_res_hostname_mapping) },
	{ "set_res_status",		CB(mem_fun.set_res_status) },

	{ NULL }
};

int_vector *dbal_get_token_type(const char *name)
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.get_token_type(name);

	else
		return NULL;
}

int_vector *dbal_get_token_status(const char *name)
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.get_token_status(name);

	else
		return NULL;
}

int_vector *dbal_get_res_membership(const char *name)
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
	    	return mem_fun.get_res_membership(name);

	else
		return NULL;
}

int_vector *dbal_get_usr_membership(const char *name)
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.get_usr_membership(name);

	else
		return NULL;
}

int_vector *dbal_get_bridge_owner_context(const char *name)
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.get_bridge_owner_context(name);

	else
		return NULL;
}

int dbal_set_token_addr_mapping(const char *name, const char *addr)
{ 
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.set_token_addr_mapping(name, addr);

	else
		return -1;
}

int dbal_unset_token_addr_mapping(const char *addr)
{ 
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.unset_token_addr_mapping(addr);

	else
		return -1;
}

int dbal_set_res_addr_mapping(const char *name,
			      const char *haddr, const char *addr)
{
	journal_ftrace(__func__);
	
	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.set_res_addr_mapping(name, haddr, addr);

	else
		return -1;
}

int dbal_set_res_hostname_mapping(const char *name,
				  const char *haddr,
				  const char *hostname)
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.set_res_hostname_mapping(name, haddr, hostname);

	else
		return -1;	
}

int dbal_set_res_status(const char *name, const char *haddr, const unsigned int status)
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.set_res_status(name, haddr, status);

	else
		return -1;
}

void dbal_fini()
{
	mem_fun.db_fini();
	return;
}

int dbal_init(const char *db_host, const char *db_usr,
	      const char *db_pwd, const char *db_name)
{
    journal_ftrace(__func__);

    mem_fun.hook = hooklet_inherit(HOOKLET_DBAL);
    if (mem_fun.hook == NULL) {
	journal_notice("%s :: %s:%i\n",
		       "No hooklet available to inherit "
		       "the DBAL subsystem", __FILE__, __LINE__);
	    return -1;
    }

    if (hooklet_map_cb(mem_fun.hook, dbal_cb))
	return -1;

   if (mem_fun.db_init(db_host, db_usr, db_pwd, db_name) != 0)
       return -1;
 
    event_register(EVENT_EXIT, "dbal:dbal_fini()", dbal_fini, PRIO_AGNOSTIC);

    return 0;
}
