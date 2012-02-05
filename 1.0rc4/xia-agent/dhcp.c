
/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "../lib/journal.h"

#include "dhcp.h"
#include "sniffer.h"
#include "reslist.h"

#ifndef PING_TIMEOUT
# define PING_TIMEOUT 15
#endif

char *get_hostname(uint8_t *start, const uint8_t *end)
{
	journal_ftrace(__func__);

	char *str, *dot;

	if ((str = dhcp_get_hostname(start, end)) == NULL
	    && (str = dhcp_get_fqdn(start, end)) == NULL) {
		journal_notice("dhcp]> message contains no hostname\n");
		return NULL;
	}

	dot = strchr(str, '.');
	if (dot == NULL)
		return str;

	*dot = '\0';
	return str;
}

/*
 * dhcp_get_option(): parse option field to find value of a code
 * `start` is a pointer to the start of dhcp options payload
 * `end` is a pointer to the end of available payload
 * `code` is the dhcp code number to lookup for
 * returns a (struct dhcp_opt_hdr *) or NULL if code was not found.
 */
struct dhcp_opt_hdr *dhcp_get_option(uint8_t *start,
				     const uint8_t *end,
				     uint8_t code)
{
	journal_ftrace(__func__);

	struct dhcp_opt_hdr *opt;
	uint8_t *p = start;

	if ((end - start) < sizeof(struct dhcp_opt_hdr))
		return NULL;

	while (p < end)
		switch (*p) {
			case DHCP_OPT_PAD:
				++p;
				continue;

			case DHCP_OPT_END:
				return NULL;

			default:
				if (*p != code) {
					/* go to the next code */
					p++;
					p += *p + 1;
					continue;
				}

				if ((p + sizeof(struct dhcp_opt_hdr)) >= end)
					return NULL;

				opt = (struct dhcp_opt_hdr *)p;

				if (opt->length > (end - (p + sizeof(opt))))
					return NULL; /* corrupted packet */

				return opt;
		}

	return NULL;
}

char *dhcp_get_hostname(uint8_t *p, const uint8_t *z)
{
	journal_ftrace(__func__);

	struct dhcp_opt_hdr *h;
	char *dst, *src;
	uint8_t len;

	h = dhcp_get_option(p, z, DHCP_OPT_HOSTNAME);

	if (h == NULL)
		return NULL;

	len = h->length;
	h++;

	src = (char *)((uint8_t *)h);
	dst = calloc(1, len + 1);

	if (dst != NULL) {
		strncpy(dst, src, len);
		dst[len] = '\0';
		return dst;
	}

	return NULL;
}

uint8_t dhcp_get_msgtype(uint8_t *p, const uint8_t *z)
{
	journal_ftrace(__func__);

	struct dhcp_opt_hdr *h;

	h = dhcp_get_option(p, z, DHCP_OPT_MSGTYPE);
	if (h == NULL || h->length != 1)
		return 0;

	return *(uint8_t *)++h;
}

#define DHCP_OPT_FQDN_FLAGS_LENGTH 3
char *dhcp_get_fqdn(uint8_t *p, const uint8_t *z)
{
	journal_ftrace(__func__);

	struct dhcp_opt_hdr *h;
	char *dst, *src;
	uint8_t len;

	h = dhcp_get_option(p, z, DHCP_OPT_FQDN);

	if (h == NULL || h->length <= DHCP_OPT_FQDN_FLAGS_LENGTH)
		return NULL;

	len = h->length - DHCP_OPT_FQDN_FLAGS_LENGTH;
	h++;

	src = (char *)((uint8_t *)h + DHCP_OPT_FQDN_FLAGS_LENGTH);
	dst = calloc(1, len + 1);

	if (dst != NULL) {
		strncpy(dst, src, len);
		dst[len] = '\0';
		return dst;
	}

	return NULL;
}

int dhcp_verify(const struct dhcp_msg *dh)
{
	journal_ftrace(__func__);

	uint8_t chaddr, i;

	if (dh->cookie[0] != 99 || dh->cookie[1] != 130
		|| dh->cookie[2] != 83 || dh->cookie[3] != 99) {
		journal_notice("dhcp]> could not identify DHCP magic cookie :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	if (dh->hlen != DHCP_HLEN) {
		journal_notice("dhcp]> Ethernet MAC contains %u numbers, not %u :: %s:%i\n", DHCP_HLEN, dh->hlen, __FILE__, __LINE__);
		return -1;
	}

	for (i = 0; i < DHCP_HLEN; i++)
		chaddr |= dh->chaddr[i];

	if (chaddr == 0) {
		journal_notice("dhcp]> hashing key chaddr is absent! :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	if (dh->xid == 0) {
		journal_notice("dhcp]> hashing key xid is absent! :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	return 0;
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
	journal_ftrace(__func__);

	const struct ip *iph = (struct ip *)((uint8_t *)pkt + ETHERLEN);
	unsigned int	iphlen = (iph->ip_hl)*4,
			skip_h = ETHERLEN + iphlen + sizeof(struct udphdr);
	
	const struct dhcp_msg *dh;
	reslist_t *entry = NULL;

	uint8_t *pl_start;	/* Start of payload */
	const uint8_t *pl_end;	/* End of payload */

	int msgtype, ret;
	char *hostname;

	journal_notice("dhcp]> handle_dhcp_packet() callback running... :: %s:%i\n", __FILE__, __LINE__);

	if (iphlen < 20) {
		journal_notice("dhcp]> malformed IP packet (iphlen = %i) :: %s:%i\n", iphlen, __FILE__, __LINE__);
		return;
	}
	
	dh = (struct dhcp_msg *)((uint8_t *)pkt + skip_h);

	if (dhcp_verify(dh))
		return;

	entry = reslist_add(dh);
	if (entry == NULL) {
		journal_notice("dhcp]> reslist_add() failed :: %s:%i\n", __FILE__, __LINE__);
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
				reslist_del(entry);
			} else {
				journal_notice("dhcp]> strange dhcp message without msg types :: %s:%i\n", __FILE__, __LINE__);
			}
			return;

		case DHCPDISCOVER:
		case DHCPOFFER:
		case DHCPDECLINE:
		case DHCPNAK:
		case DHCPINFORM:
			journal_notice("dhcp]> Caught ignored dhcp message type %u :: %s:%i\n", msgtype, __FILE__, __LINE__);
			// for the moment, we don't care about them
			return;

		case DHCPREQUEST:
			journal_notice("dhcp]> Caught dhcp REQUEST message :: %s:%i\n", __FILE__, __LINE__);

			entry->dhcp_state = msgtype;

			hostname = get_hostname(pl_start, pl_end);
			if (hostname) {
				journal_notice("dhcp]> hostname is %s :: %s:%i\n", hostname, __FILE__, __LINE__);
				strncpy(entry->hostname, hostname, DISCOVERY_MSG_HOSTNAMESIZ);
			}
			break;

		case DHCPACK:
			journal_notice("dhcp]> Caught dhcp ACK message :: %s:%i\n", __FILE__, __LINE__);

			if (entry->dhcp_state < DHCPREQUEST) {
				journal_notice("dhcp]> state lower than DHCPREQUEST :: %s:%i\n", __FILE__, __LINE__);
				return;
			}

			if (ret = reslist_purge(dh))
				journal_notice("dhcp]> purged %u entry :: %s:%i\n", ret, __FILE__, __LINE__);

			entry->dhcp_state = msgtype;
			entry->addr.s_addr = dh->yiaddr.s_addr;
			if (entry->addr.s_addr == 0) {
				journal_notice("dhcp]> could not get ip address :: %s:%i\n", __FILE__, __LINE__);
				return;

			}

			acl_res_add(entry);
			discovery_set_res_addr(entry);
			discovery_set_res_hostname(entry);
			discovery_set_res_alive(entry);

			// add to pingring only once, avoid duplicate entry
			if (entry->timeout == 0) {
				entry->timeout = PING_TIMEOUT;
				pingring_add(entry);
				pingring_show();
			} else {
				journal_notice("dhcp]> entry is already monitored by pingring :: %s:%i\n", __FILE__, __LINE__);
			}

			break;

		case DHCPRELEASE:
			journal_notice("dhcp]> Caught dhcp RELEASE message :: %s:%i\n", __FILE__, __LINE__);

			pingring_del(entry);

			discovery_set_res_dead(entry);
			acl_res_del(entry);

			reslist_del(entry);
			break;

		default:
			journal_notice("dhcp]> Caught unmapped dhcp message type %u :: %s:%i\n", msgtype, __FILE__, __LINE__);
			return;
	}

	return;
}

int dhcp_init()
{
	journal_ftrace(__func__);

	sniffer_register("udp port 67", handle_dhcp_packet);
}
