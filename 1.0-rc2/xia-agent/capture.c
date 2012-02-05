
/*
 * See COPYRIGHTS file.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include <string.h>
#include <pcap.h>

#include "../lib/journal.h"

#define SNAPLEN 600 /* tcpdump -s 600 */

pcap_t *ph = NULL;
pcap_handler callback = NULL;

int capture_init(const char *dev)
{
	journal_ftrace(__func__);

	char errbuf[PCAP_ERRBUF_SIZE] = "\0";
        	
	ph = pcap_open_live(dev, SNAPLEN, IFF_PROMISC, 1000, errbuf);

	if (strlen(errbuf))
		/* pcap_open_live() also reports non-fatal errors */
		journal_notice("pcap_open_live: %s :: %s:%i\n",
		    errbuf, __FILE__, __LINE__);

	if (ph == NULL)
		return 1;
		
	pcap_setnonblock(ph, 1, errbuf);
	return 0;
}

int capture_dhcp_only()
{
	journal_ftrace(__func__);

	struct bpf_program bp;	/* compiled filter expression */

	if (pcap_compile(ph, &bp, "udp port 67", 0, 0) == -1) {
		pcap_perror(ph, "pcap_compile()");
		return 1;
	}

	if (pcap_setfilter(ph, &bp) == -1) {
		pcap_perror(ph, "pcap_setfilter()");
		return 1;
	}

	return 0;
}

void capture_set_handler(pcap_handler cb)
{
	journal_ftrace(__func__);
	callback = cb;
}

void capture()
{
	journal_ftrace(__func__);

	if (callback == NULL) {
		journal_notice("%s :: %s:%i\n",
		    "must use capture_set_callback() first.",
		    __FILE__, __LINE__);
		return;
	}

	pcap_dispatch(ph, -1, callback, NULL);
}
