/* 
 * See COPYRIGHTS file.
 */

#include <unistd.h>

#include <xia/event.h>
#include <xia/hooklet.h>
#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/options.h>
#include <xia/sched.h>

#include "dbal.h"
#include "switch.h"

#ifndef CONFIG_FILE
# define CONFIG_FILE "/etc/xia/xia-switch.conf"
#endif

char *switch_addr = NULL;
int *switch_port = NULL;

char *hooklet_list = NULL;
char *hooklet_path = NULL;

char *db_host = NULL;
char *db_usr = NULL;
char *db_pwd = NULL;
char *db_name = NULL;

struct options opts[] = {

	{ "switch_addr",	&switch_addr,	OPT_STR | OPT_MAN },
	{ "switch_port",	&switch_port,	OPT_INT | OPT_MAN },
	{ "hooklet_list",	&hooklet_list, 	OPT_STR | OPT_MAN },
	{ "hooklet_path",	&hooklet_path,	OPT_STR | OPT_MAN },
	{ "db_host",		&db_host,	OPT_STR | OPT_MAN },
	{ "db_usr",		&db_usr,	OPT_STR | OPT_MAN },
	{ "db_pwd",		&db_pwd,	OPT_STR | OPT_MAN },
	{ "db_name",		&db_name,	OPT_STR | OPT_MAN },

	{ NULL }
};

int main(int argc, char *argv[])
{
	/*
	 * Phase 1 - State initialization
	 */
	if (journal_init()) {
		journal_failure(EXIT_ERR, "xia-switch]> journal_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (option_parse(opts, CONFIG_FILE)) {
		journal_failure(EXIT_ERR, "xia-switch]> option_prase() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}
	
	option_dump(opts);

	/*
	 * Phase 2 - System initialization
	 */
	if (event_init()) {
		journal_failure(EXIT_ERR, "xia-switch]> event_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (scheduler_init()) {
		journal_failure(EXIT_ERR, "xia-switch]> scheduler_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (netbus_init()) {
		journal_failure(EXIT_ERR, "xia-switch]> netbus_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (hooklet_init(hooklet_list, hooklet_path)) {
		journal_failure(EXIT_ERR, "xia-switch]> hooklet_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	hooklet_show();

	if (dbal_init(db_host, db_usr, db_pwd, db_name)) {
		journal_failure(EXIT_ERR, "xia-switch]> dbal_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (switch_init(switch_addr, *switch_port)) {
		journal_failure(EXIT_ERR, "xia-switch]> switch_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}
 
	/*
	 * Phase 3 - Start the game !
	 */
	scheduler();

	return 0;
}
