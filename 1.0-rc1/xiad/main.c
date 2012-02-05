/*
 * Copyright (c) 2008; Nicolas Bouliane <nicboul@gmail.com>
 * Copyright (c) 2008; Mind4Networks Technologies INC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>

#include "../lib/netbus.h"
#include "../lib/event.h"
#include "../lib/hooklet.h"
#include "../lib/journal.h"
#include "../lib/options.h"
#include "../lib/sched.h"

#include "acl.h"
#include "bridge.h"
#include "dbal.h"
#include "discovery.h"
#include "muxia.h"
#include "session.h"

char *db_name = NULL;
char *db_usr = NULL;
char *db_pwd = NULL;
char *db_host = NULL;
char *hooklet_list = NULL;
char *listen_addr = NULL;
int  *port = NULL;

struct options opts[] = {

    { "db_name",	&db_name,	OPT_STR | OPT_MAN },
    { "db_usr",		&db_usr,	OPT_STR | OPT_MAN },
    { "db_pwd",		&db_pwd,	OPT_STR | OPT_MAN },
    { "db_host",	&db_host,	OPT_STR | OPT_MAN },
    { "hooklet_list",	&hooklet_list,	OPT_STR | OPT_MAN },
    { "listen_addr",	&listen_addr,	OPT_STR | OPT_MAN },
    { "port",		&port,		OPT_INT | OPT_MAN },
    
    { NULL }
};

static void daemonize()
{
	journal_strace("daemonize");

	pid_t pid, sid;

	if (getppid() == 1)
		return;

	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);

	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
}

static void xiad_fini(void *data)
{
    journal_strace("xiad_fini");

    option_free(opts);
}

int main(int argc, char *argv[])
{
	journal_strace("main");

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
				journal_failure(EXIT_ERR, "getopt() failed :: %s:%i\n", __FILE__, __LINE__);
				_exit(EXIT_ERR);
	    	}
    	}

	if (journal_init()) {
		journal_failure(EXIT_ERR, "journal_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (option_parse(opts, "/etc/xia/xiad.conf")) {
		journal_failure(EXIT_ERR, "option_parse() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	option_dump(opts); 
	event_register(EVENT_EXIT, "xiad:xiad_fini()", xiad_fini, PRIO_AGNOSTIC);

	/* 
	 * Phase 2 - Subsystem initialization
	 */

	if (event_init()) {
        	journal_failure(EXIT_ERR, "event_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (scheduler_init()) {
		journal_failure(EXIT_ERR, "scheduler_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (hooklet_init(hooklet_list)) {
		journal_failure(EXIT_ERR, "hooklet_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	hooklet_show();

	if (dbal_init(db_host, db_usr, db_pwd, db_name)) {
		journal_failure(EXIT_ERR, "dbal_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (acl_init()) {
		journal_failure(EXIT_ERR, "acl_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (session_init()) {
		journal_failure(EXIT_ERR, "session_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (bridge_init()) {
		journal_failure(EXIT_ERR, "bridge_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}
    
	if (discovery_init()) {
		journal_failure(EXIT_ERR, "discovery_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}


	if (netbus_init()) {
		journal_failure(EXIT_ERR, "netbus_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}
	
	/*
	    if (trumpet_sock_serv_init(listen_addr, *(int *)port))
	    log_exit_error(EXIT_ERR, "trumpet_sock_serv_init() failed. :: %s:%i\n", __FILE__, __LINE__);
	*/
    
	if (muxia_init(listen_addr, *(int *)port)) {
		journal_failure(EXIT_ERR, "muxia_init() failed. :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	/*
	 * Phase 3 - run !
	 */

	if (D_FLAG) { 
		daemonize(); 
		log_set_lvl(1);
	}
  
	scheduler();

	return 0;
}
