/*
 * See COPYRIGHTS file.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>

#include <xia/dnds.h>
#include <xia/event.h>
#include <xia/hooklet.h>
#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/options.h>
#include <xia/sched.h>
#include <xia/utils.h>

#include "dbal.h"
#include "dnds.h"

char *listen_address = NULL;
int  *tcp_port = NULL;

char *hooklet_list = NULL;
char *hooklet_path = NULL;

char *db_host = NULL;
char *db_usr = NULL;
char *db_pwd = NULL;
char *db_name = NULL;

struct options opts[] = {

	{ "listen_address",	&listen_address,	OPT_STR | OPT_MAN },
	{ "tcp_port",		&tcp_port,		OPT_INT | OPT_MAN },
	{ "hooklet_list",	&hooklet_list,		OPT_STR | OPT_MAN },
	{ "hooklet_path",	&hooklet_path,		OPT_STR | OPT_MAN },
	{ "db_host",		&db_host,		OPT_STR | OPT_MAN },
	{ "db_usr",		&db_usr,		OPT_STR | OPT_MAN },
	{ "db_pwd",		&db_pwd,		OPT_STR | OPT_MAN },
	{ "db_name",		&db_name,		OPT_STR | OPT_MAN },

	{ NULL }
};

int main(int argc, char *argv[])
{
	journal_ftrace(__func__);

	int opt, D_FLAG = 0;
	/* 
	 * Phase 1 - State initialization 
	 */
  
	if (getuid() != 0) {
		journal_notice("%s must be run as root\n", argv[0]);
	}
    
	while ((opt = getopt(argc, argv, "dv")) != -1) {
		switch (opt) {
			case 'd':
				D_FLAG = 1;
				break;
	    		case 'v':
				printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
				exit(EXIT_SUCCESS);
	    		default:
				printf("-d , -v\n");
				journal_failure(EXIT_ERR, "dnds]> getopt() failed :: %s:%i\n", __FILE__, __LINE__);
				_exit(EXIT_ERR);
	    	}
    	}

	if (journal_init()) {
		journal_failure(EXIT_ERR, "dnds]> journal_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (option_parse(opts, CONFIG_FILE)) {
		journal_failure(EXIT_ERR, "dnds]> option_parse() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	option_dump(opts); 

	/* 
	 * Phase 2 - Subsystem initialization
	 */

	if (event_init()) {
        	journal_failure(EXIT_ERR, "dnds]> event_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (scheduler_init()) {
		journal_failure(EXIT_ERR, "dnds]> scheduler_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (hooklet_init(hooklet_list, hooklet_path)) {
		journal_failure(EXIT_ERR, "dnds]> hooklet_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	hooklet_show();

	if (dbal_init(db_host, db_usr, db_pwd, db_name)) {
		journal_failure(EXIT_ERR, "dnds]> dbal_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}
	
	if (netbus_init()) {
		journal_failure(EXIT_ERR, "dnds]> netbus_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}
	
	if (dnds_init(listen_address, *(int *)tcp_port)) {
		journal_failure(EXIT_ERR, "dnds]> dnds_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	/*
	 * Phase 3 - run !
	 */
	if (D_FLAG) { 
		daemonize(); 
		journal_set_lvl(1);
	}
  
	scheduler();

	return 0;
}
