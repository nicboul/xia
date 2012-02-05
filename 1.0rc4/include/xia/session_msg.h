
/*
 * See COPYRIGHTS file.
 */

#ifndef XIA_SESSION_MSG_H
#define XIA_SESSION_MSG_H

#include <stdint.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <xia/xiap.h>

#define SESSION_MSG_VERSION	1

#define SESSION_MSG_UP		1
#define SESSION_MSG_DOWN	2

enum {
	SESSION_STATUS_DISABLED = 0,
	SESSION_STATUS_ENABLED,
	SESSION_STATUS_COMPROMISED,
};

struct session_msg {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int type:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;		/* Protocol version */
	unsigned int type:4;		/* Message type */
#else
# error "Please fix <bits/endian.h>"
#endif
	char name[XIAP_NAMESIZ];	/* Username */

	struct in_addr addr;		/* IP address */
};

#endif /* XIA_SESSION_MSG_H */
