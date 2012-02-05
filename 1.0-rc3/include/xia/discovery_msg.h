
/*
 * See COPYRIGHTS file.
 */

#ifndef XIA_DISCOVERY_MSG_H
#define XIA_DISCOVERY_MSG_H

#include <stdint.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <xia/xiap.h>

#define DISCOVERY_MSG_VERSION	1

#define DISCOVERY_MSG_HOSTNAMESIZ	128

#define DISCOVERY_MSG_HADDRSIZ 6

/* Query type */

#define DISCOVERY_MSG_SET_RES_ADDR		1
#define DISCOVERY_MSG_SET_RES_HOSTNAME		2
#define DISCOVERY_MSG_SET_RES_HOST_DEAD		3
#define DISCOVERY_MSG_SET_RES_HOST_ALIVE	4

struct discovery_msg {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int type:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;		/* Message version */
	unsigned int type:4;		/* Query type */
#else
# error "Please fix <bits/endian.h>"
#endif
	char name[XIAP_NAMESIZ];	/* XIA Token Name */

	uint8_t haddr[DISCOVERY_MSG_HADDRSIZ];
	struct in_addr addr;	/* IP address */

	char hostname[DISCOVERY_MSG_HOSTNAMESIZ];	/* Hostname */
};

#endif /* XIA_DISCOVERY_MSG_H */
