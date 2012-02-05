/*
 * See COPYRIGHTS file
 */

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>

#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/xtp.h>

#include "context.h"
#include "inet.h"

const uint8_t mac_addr_broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
const uint8_t mac_addr_empty[6] = { 0, 0, 0, 0, 0, 0 };

int inet_get_mac_addr_type(uint8_t *mac_addr)
{
	if (memcmp(mac_addr, mac_addr_broadcast, ETHER_ADDR_LEN) == 0) {
		return ADDR_BROADCAST;
	}

	// else, default
	return ADDR_UNICAST;
}

int inet_get_mac_addr_dst(void *data, uint8_t *mac_addr)
{
	struct ether_header *eth_hdr;
	eth_hdr = data;

	bcopy(eth_hdr->ether_dhost, mac_addr, ETHER_ADDR_LEN);

	return 0;
}

int inet_get_mac_addr_src(void *data, uint8_t *mac_addr)
{
	struct ether_header *eth_hdr;
	eth_hdr = data;

	bcopy(eth_hdr->ether_shost, mac_addr, ETHER_ADDR_LEN);

	return 0;
}

uint16_t inet_get_iphdr_len(void *data)
{
	journal_ftrace(__func__);

	struct ip *iph;
	iph = data + sizeof(struct ether_header);
	return ntohl(iph->ip_len);
}

void inet_print_iphdr(void *data)
{
	journal_ftrace(__func__);

	struct ip *iph;
	iph = data + sizeof(struct ether_header);
	journal_notice("inet]> iphdr len: %i\n", iph->ip_len);
}

int inet_is_ip(void *data)
{
	journal_ftrace(__func__);

	struct ether_header *hd;
	hd = data;

	if (htons(hd->ether_type) == 0x800)
		return 1;

	return 0;
}
 
void inet_print_ether_type(void *data)
{
	journal_ftrace(__func__);

	struct ether_header *hd;
	hd = data;

	journal_notice("inet]> ether type: %x\n", htons(hd->ether_type));
}

void inet_print_ethernet(void *data)
{
	journal_ftrace(__func__);

	struct ether_header *hd;
	hd = data;

	printf("inet]> ether_dhost: %02x:%02x:%02x:%02x:%02x:%02x\n", hd->ether_dhost[0],
		hd->ether_dhost[1], hd->ether_dhost[2], hd->ether_dhost[3], 
		hd->ether_dhost[4], hd->ether_dhost[5]);

	printf("inet]> ether_shost: %02x:%02x:%02x:%02x:%02x:%02x\n", hd->ether_shost[0],
		hd->ether_shost[1], hd->ether_shost[2], hd->ether_shost[3],
		hd->ether_shost[4], hd->ether_shost[5]);	

	printf("inet]> ether_type: %x\n", htons(hd->ether_type));
}

void inet_print_arp(peer_t *peer)
{
	journal_ftrace(__func__);

	struct ether_header *hd;
	struct ether_arp *ea;

	hd = peer->data;

	if (htons(hd->ether_type) == 0x806) { // ARP Request
		ea = peer->data + sizeof(struct ether_header);

		printf("arp_hdr: %i\n", ea->arp_hrd);

		printf("arp_sha:  %02x:%02x:%02x:%02x:%02x:%02x\n", ea->arp_sha[0],
			ea->arp_sha[1], ea->arp_sha[2], ea->arp_sha[3],
			ea->arp_sha[4], ea->arp_sha[5]);
		printf("arp_spa: %02x:%02x:%02x:%02x\n", ea->arp_spa[0],
			ea->arp_spa[1], ea->arp_spa[2], ea->arp_spa[3]);

		printf("arp_tha: %02x:%02x:%02x:%02x:%02x:%02x\n", ea->arp_tha[0],
			ea->arp_tha[1], ea->arp_tha[2], ea->arp_sha[3],
			ea->arp_tha[4], ea->arp_sha[5]);
		printf("arp_tpa: %02x:%02x:%02x:%02x\n", ea->arp_tpa[0],
			ea->arp_tpa[1], ea->arp_tpa[2], ea->arp_tpa[4]);
	}
}
