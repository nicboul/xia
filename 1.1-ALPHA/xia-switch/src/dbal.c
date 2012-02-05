/*
 * See COPYRIGHTS file.
 */


#include <stdio.h>

#include <xia/event.h>
#include <xia/hooklet.h>
#include <xia/journal.h>
#include <xia/utils.h>

static struct {

	int (*db_init)(const char *host, const char *user,
		const char *passwd, const char *dbname);
	int (*db_fini)();
	int (*db_ping)();

	int (*get_all_context_pool)(void (*cb)(unsigned short, char *, char *, char *, char *));
	int (*get_context_pool)(const int id, void (*cb)(char *, char *, char *, char *));
	int (*get_token_context)(const char *name, void (*cb)(int));

	/* We inherit the parent class */
	hooklet_t *hook;

} mem_fun;

hooklet_cb_t dbal_cb[] = {

	{ "db_init",		CB(mem_fun.db_init) },
	{ "db_fini",		CB(mem_fun.db_fini) },
	{ "db_ping",		CB(mem_fun.db_ping) },

	{ "get_all_context_pool",	CB(mem_fun.get_all_context_pool) },
	{ "get_context_pool",		CB(mem_fun.get_context_pool) },
	{ "get_token_context",		CB(mem_fun.get_token_context) },

	{ NULL }
};


int dbal_get_all_context_pool(void (*cb)(unsigned short, char *, char *, char *, char *))
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.get_all_context_pool(cb);
	else {
		journal_notice("dbal]> db_ping() failed, the database is not alive :: %s:%i\n",
		    __FILE__, __LINE__);
		return -1;
	}
}

int dbal_get_context_pool(int id, void (*cb)(char *, char *, char *, char *))
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.get_context_pool(id, cb);
	else {
		journal_notice("dbal]> db_ping() failed, the database is not alive :: %s:%i\n",
		    __FILE__, __LINE__);
		return -1;
	}
}

int dbal_get_token_context(char *certname, void (*cb)(int))
{
	journal_ftrace(__func__);

	int ret;

	ret = mem_fun.db_ping();
	if (ret == 0)
		return mem_fun.get_token_context(certname, cb);
	else {
		journal_notice("dbal]> db_ping() failed, the database is not alive :: %s:%i\n",
		    __FILE__, __LINE__);
		return -1;
	}
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
		journal_notice("dbal]> No hooklet available to inherit the DBAL subsystem :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	if (hooklet_map_cb(mem_fun.hook, dbal_cb))
		return -1;

	if (mem_fun.db_init(db_host, db_usr, db_pwd, db_name) != 0)
		return -1;

	event_register(EVENT_EXIT, "dbal:dbal_fini()", dbal_fini, PRIO_AGNOSTIC);

	return 0;
 }
