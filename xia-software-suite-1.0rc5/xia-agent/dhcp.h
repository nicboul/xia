#ifndef __DHCP_H
#define __DHCP_H

#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>

#define DHCP_OPT_PAD			0
#define DHCP_OPT_HOSTNAME		12
#define DHCP_OPT_MSGTYPE		53
#define DHCP_OPT_FQDN			81
#define DHCP_OPT_END			255

#define DHCPDISCOVER            1
#define DHCPOFFER               2
#define DHCPREQUEST             3
#define DHCPDECLINE             4
#define DHCPACK                 5
#define DHCPNAK                 6
#define DHCPRELEASE             7
#define DHCPINFORM              8

#define DHCP_HLEN 6	/* client hardware address length
			   In this case, 10 MB Ethernet */

struct dhcp_msg {
	uint8_t		op;			/* Opcode */
	uint8_t		htype;			/* Hardware type */
	uint8_t		hlen;			/* Hardware address length */
	uint8_t		hops;			/* Hop count */
	uint32_t	xid;			/* Transaction ID */
	uint16_t	secs;			/* Number of seconds */
	uint16_t	flags;			/* Flags */

	struct in_addr	ciaddr,			/* Client IP address */
			yiaddr,			/* Your IP address */
			siaddr,			/* Server IP address */
			giaddr;			/* Gateway IP address */

	uint8_t		chaddr[16];		/* Client MAC address */
	uint8_t		sname[64];		/* Server hostname */
        uint8_t		file[128];		/* Boot filename */
	uint8_t		cookie[4];		/* 99.130.83.99 */
};

struct dhcp_opt_hdr {
	uint8_t		code;
	uint8_t		length;
};

extern uint8_t dhcp_get_msgtype(uint8_t *, const uint8_t *);
extern char *dhcp_get_fqdn(uint8_t *, const uint8_t *);
extern char *dhcp_get_hostname(uint8_t *, const uint8_t *);
extern int dhcp_verify(const struct dhcp_msg *);

#endif /* __DHCP_H */
