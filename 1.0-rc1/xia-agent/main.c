/*
 * See the COPYRIGHTS file
 */

# define __USE_BSD

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <stdio.h>
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
#include "capture.h"
#include "dhcp.h"
#include "discovery.h"
#include "pingring.h"
#include "reslist.h"

#ifndef PING_INTERVAL
# define PING_INTERVAL	20
#endif

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

struct sigaction sa;

/* Hashlist of ressources */
reslist_t *hlist[RESLIST_SIZE] = {0};

/* Ring of ressources to ping */
pingring_t *pring;

char *get_hostname(uint8_t *start, const uint8_t *end)
{
	journal_strace("get_hostname");

	char *str, *dot;

	if ((str = dhcp_get_hostname(start, end)) == NULL
	    && (str = dhcp_get_fqdn(start, end)) == NULL) {
		printf("dhcp message contains no hostname\n");
		return NULL;
	}

	dot = strchr(str, '.');
	if (dot == NULL)
		return str;

	*dot = '\0';
	return str;
}

int purge_entry(reslist_t *entry)
{
	journal_strace(__func__);

	// XXX discovery_res_del(entry); ?

	pingring_del(&pring, entry);
	reslist_del(hlist, entry);

	return 0;
}

void ping_timeout(reslist_t *entry)
{
	journal_strace(__func__);
	discovery_set_res_dead(entry);
}

void ping_replied(reslist_t *entry)
{
	journal_strace(__func__);
	discovery_set_res_alive(entry);
}

/*
 * handle_dhcp_packet(): callback function given to pcap_loop()
 *
 * This function is called via pcap_loop() each time a packet
 * is captured.
 *
 * ETHERLEN is the length of Ethernet header, which is always 14 bytes.
 */
#define ETHERLEN 14
void handle_dhcp_packet(u_char *args,
			const struct pcap_pkthdr *h,
			const u_char *pkt)
{
	journal_strace("handle_dhcp_packet");

	const struct ip *iph = (struct ip *)((uint8_t *)pkt + ETHERLEN);
	unsigned int	iphlen = (iph->ip_hl)*4,
			skip_h = ETHERLEN + iphlen + sizeof(struct udphdr);
	
	const struct dhcp_msg *dh;
	reslist_t *entry = NULL;

	uint8_t *pl_start;	/* Start of payload */
	const uint8_t *pl_end;	/* End of payload */

	int msgtype, ret;
	char *hostname;

	printf("handle_dhcp_packet() callback running...\n");

	if (iphlen < 20) {
		printf("malformed IP packet (iphlen = %i)\n", iphlen);
		return;
	}
	
	dh = (struct dhcp_msg *)((uint8_t *)pkt + skip_h);

	if (dhcp_verify(dh))
		return;

	entry = reslist_add(hlist, dh);
	if (entry == NULL) {
		printf("reslist_add() failed\n");
		return;
	}

	pl_start = (uint8_t *)dh + sizeof(struct dhcp_msg);
	pl_end = (uint8_t *)pkt + h->caplen;

	msgtype = dhcp_get_msgtype(pl_start, pl_end);

	if (entry->dhcp_state == msgtype) // avoid duplicated messages
		return;

	switch (msgtype) {

		case 0:
			if (entry->dhcp_state < DHCPDISCOVER) {
				reslist_del(hlist, entry);
			} else {
				journal_notice("%s :: %s:%i\n",
				    "strange dhcp message without msg type",
					__FILE__, __LINE__);
			}
			return;

		case DHCPDISCOVER:
		case DHCPOFFER:
		case DHCPDECLINE:
		case DHCPNAK:
		case DHCPINFORM:
			journal_notice("%s %u :: %s:%i\n",
			    "Caught ignored dhcp message type",
			    msgtype, __FILE__, __LINE__);
			// for the moment, we don't care about them
			return;

		case DHCPREQUEST:
			journal_notice("%s :: %s:%i\n",
			    "Caught dhcp REQUEST message",
			    __FILE__, __LINE__);

			entry->dhcp_state = msgtype;

			hostname = get_hostname(pl_start, pl_end);
			if (hostname) {
				printf("hostname is %s\n", hostname);
				strncpy(entry->hostname, hostname,
				    DISCOVERY_MSG_HOSTNAMESIZ);
			}
			break;

		case DHCPACK:
			journal_notice("%s :: %s:%i\n",
			    "Caught dhcp ACK message",
			    __FILE__, __LINE__);

			if (entry->dhcp_state < DHCPREQUEST) {
				journal_notice("%s :: %s:%i\n",
				    "dhcp_state is lower than DHCPREQUEST",
				    __FILE__, __LINE__);
				return;
			}

			if (ret = reslist_purge(hlist, purge_entry, dh))
				journal_notice("purged %u entry\n", ret);

			entry->dhcp_state = msgtype;

			entry->addr.s_addr = dh->yiaddr.s_addr;
			if (entry->addr.s_addr == 0) {
				journal_notice("%s :: %s:%i\n",
					"could not get ip address",
					__FILE__, __LINE__);
				return;

			}

			acl_res_add(entry);
			discovery_set_res_addr(entry);
			discovery_set_res_hostname(entry);
			discovery_set_res_alive(entry);

			entry->timeout = 15;
			pingring_add(&pring, entry);
			pingring_show(pring);

			break;

		case DHCPRELEASE:
			journal_notice("%s :: %s:%i\n",
			    "Caught dhcp RELEASE message",
			    __FILE__, __LINE__);

			pingring_del(&pring, entry);

			discovery_set_res_dead(entry);
			acl_res_del(entry);

			reslist_del(hlist, entry);
			break;

		default:
			journal_notice("%s %u :: %s:%i\n",
			    "Caught unmapped dhcp message type",
			    msgtype, __FILE__, __LINE__);
			return;
	}

	return;
}

int set_context(char *x509cn)
{
	journal_strace("set_context");

	unsigned int context = 0;

	journal_notice("%s %s :: %s:%i\n",
	    "x.509 commonName is", x509cn,
	    __FILE__, __LINE__);

	context = bridge_get_owner_context(x509cn);
	if (context == 0) {
		journal_notice("%s :: %s:%i\n",
		    "context is still 0... anormal.",
		    __FILE__, __LINE__);
		return 1;
	}

	journal_notice("%s %u :: %s:%i\n",
	    "owner context is", context, __FILE__, __LINE__);

	if (swap_context(x509cn, context)) {
		journal_notice("%s :: %s:%i\n",
		    "failed to swap context in x509cn",
		    __FILE__, __LINE__);
		return 1;
	}

	journal_notice("%s %s :: %s:%i\n",
	    "now using xia name", x509cn,
	    __FILE__, __LINE__);

	return 0;
}

static void xia_agent_fini(void *data)
{
	journal_strace("xia_agent_fini");
}

static void daemonize()
{
	journal_strace("daemonize");

	pid_t pid, sid;

	if (getppid() == 1)
		return;

	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);

	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);

	sid = setsid();

	if (sid < 0)
		exit(EXIT_FAILURE);

	if ((chdir("/")) < 0)
		exit(EXIT_FAILURE);

	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
}
	
int main(int argc, char **argv)
{
	journal_strace(__func__);

	int opt, D_FLAG = 0;

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

	if (option_parse(opts, "/etc/xia/xia-agent.conf")) {
		journal_failure(EXIT_ERR, "option_parse() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	option_dump(opts);

	event_register(EVENT_EXIT, "xia-agent:xia_agent_fini()",
	    xia_agent_fini, PRIO_AGNOSTIC);

	sched_register(SCHED_APERIODIC, "capture", capture, 0, NULL);

	pingring_set_replied_callback(ping_replied);
	pingring_set_timeout_callback(ping_timeout);
	sched_register(SCHED_PERIODIC, "pingring_ping",
	    &pingring_ping, PING_INTERVAL, &pring);

	if (capture_init(dev)) {
		journal_failure(EXIT_ERR, "capture_init() failed :: %s:%i\n",
		    __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	x509cn = x509_get_cn(cert_path);
	if (x509cn == NULL) {
		journal_failure(EXIT_ERR, "failed to get certificate name :: %s %i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (set_context(x509cn)) {
		journal_notice("set_context() failed :: %s:%i\n",
		    __FILE__, __LINE__);
		goto free;
	}

	capture_set_handler(handle_dhcp_packet);
	capture_dhcp_only();


	if (D_FLAG) {
		daemonize();
		log_set_lvl(1);
	}

	scheduler();

free:
	free(x509cn);
	option_free(opts);
	return 1;
}
