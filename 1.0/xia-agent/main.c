
/*
 * See COPYRIGHTS file
 */

#include "config.h"

#define __USE_BSD
#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pcap.h>
#include <unistd.h>

#include "../lib/event.h"
#include "../lib/journal.h"
#include "../lib/netbus.h"
#include "../lib/options.h"
#include "../lib/utils.h"
#include "../lib/sched.h"

#include "acl.h"
#include "sniffer.h"
#include "dhcp.h"
#include "discovery.h"
#include "pingring.h"
#include "reslist.h"

char	*dev,		/* interface we listen on */
	*uid,		/* setuid() */
	*gid,		/* setgid() */
	*cert_path,	/* x509 Certficate path */
	*xiad_addr,	/* XIA server IP address */
	*x509cn;	/* X.509 commonName */

uint16_t *xiad_port;	/* XIA server TCP port */

static struct options opts[] = {

    { "listen",		&dev,		OPT_STR|OPT_MAN },
    { "cert_path",	&cert_path,	OPT_STR|OPT_MAN },
    { "remote",		&xiad_addr,	OPT_STR|OPT_MAN },
    { "port",		&xiad_port,	OPT_INT|OPT_MAN },
    { "user",		&uid,		OPT_STR },
    { "group",		&gid,		OPT_STR },
    { NULL }
};

	
static void xia_agent_fini(void *data)
{
	journal_ftrace(__func__);

	free(x509cn);
	x509cn = NULL;

	option_free(opts);
}

#ifndef CONFIG_FILE
# define CONFIG_FILE "/etc/xia/xia-agent.conf"
#endif

int main(int argc, char **argv)
{
	journal_ftrace(__func__);

	int opt, D_FLAG = 0;

	/* step 1 */
	if (journal_init()) {
		journal_failure(EXIT_ERR, "journal_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (getuid() != 0) {
		journal_notice("%s must be run as root\n", argv[0]);
	}

	while ((opt = getopt(argc, argv, "dv")) != -1) {
		switch (opt) {
			case 'd':
				D_FLAG = 1;
				break;
			case 'v':
				journal_notice("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
				exit(EXIT_SUCCESS);
			default:
				journal_notice("-d , -v\n");
				journal_failure(EXIT_ERR, "getopt() failed :: %s:%i\n", __FILE__, __LINE__);
				exit(EXIT_FAILURE);
		}
	}

	if (event_init()) {
		journal_failure(EXIT_ERR, "event_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (scheduler_init()) {
		journal_failure(EXIT_ERR, "scheduler_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (netbus_init()) {
		journal_failure(EXIT_ERR, "netbus_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (option_parse(opts, CONFIG_FILE)) {
		journal_failure(EXIT_ERR, "option_parse() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}


	/* step 2 */
	if (sniffer_init(dev)) {
		journal_failure(EXIT_ERR, "sniffer_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (dhcp_init()) {
		journal_failure(EXIT_ERR, "dhcp_init failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (pingring_init()) {
		journal_failure(EXIT_ERR, "pingring_init failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (kalive_init(xiad_addr)) {
		journal_failure(EXIT_ERR, "kalive_init failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	/* step 3 */
	x509cn = x509_get_cn(cert_path);
	if (x509cn == NULL) {
		journal_failure(EXIT_ERR, "failed to get certificate name :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (bridge_init(x509cn, xiad_addr, *xiad_port)) {
		journal_failure(EXIT_ERR, "bridge_init failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (discovery_init(x509cn, xiad_addr, *xiad_port)) {
		journal_failure(EXIT_ERR, "discovery_init failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (acl_init(x509cn, xiad_addr, *xiad_port)) {
		journal_failure(EXIT_ERR, "acl_init failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	/* step 4 */
	event_register(EVENT_EXIT, "xia_agent_fini", xia_agent_fini, PRIO_AGNOSTIC);

	if (D_FLAG) {
		daemonize();
		journal_set_lvl(1);
	}

	scheduler();

	return 0;
}
