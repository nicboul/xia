
/*
 * See COPYRIGHTS file.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include <string.h>
#include <pcap.h>
#include "sniffer.h"

#include "../lib/journal.h"
#include "../lib/sched.h"

#define SNAPLEN 600 /* tcpdump -s 600 */

pcap_t *ph = NULL;
void *callback;
const char *dev;

void sniffer()
{
	journal_ftrace(__func__);
	pcap_dispatch(ph, -1, callback, NULL);
}

int sniffer_register(char *filter, void (*cb)(u_char *, const struct pcap_pkthdr *, const u_char *))
{
	journal_ftrace(__func__);
	
	struct bpf_program bp;	/* compiled filter expression */
	char errbuf[PCAP_ERRBUF_SIZE] = "\0";

	ph = pcap_open_live(dev, SNAPLEN, IFF_PROMISC, 1000, errbuf);
	if (strlen(errbuf))
		journal_notice("sniffer]> pcap_open_live: %s :: %s:%i\n", errbuf, __FILE__, __LINE__);

	if (ph == NULL)
		return -1;

	pcap_setnonblock(ph, 1, errbuf);

	/* add the protocol and the filter */
	if (pcap_compile(ph, &bp, filter, 0, 0) == -1) {
		pcap_perror(ph, "sniffer]> pcap_compile()");
		return 1;
	}

	if (pcap_setfilter(ph, &bp) == -1) {
		pcap_perror(ph, "sniffer]> pcap_setfilter()");
		return 1;
	}

	// XXX 
	callback = cb;

	return 0;
}

int sniffer_init(const char *d)
{
	journal_ftrace(__func__);

	//XXX
	dev = d;	

	sched_register(SCHED_APERIODIC, "sniffer", sniffer, 0, NULL);

	return 0;
}
