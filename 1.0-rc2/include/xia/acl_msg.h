#ifndef XIA_ACL_MSG_H
#define XIA_ACL_MSG_H

#include <stdint.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <xia/xiap.h>

#define ACL_MSG_VERSION	1

/* ACL messages types */

#define ACL_MSG_ADD	1
#define ACL_MSG_DEL	2

struct acl_msg {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int type:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;		/* Protocol version */
	unsigned int type:4;		/* Message type */
#else
# error "Please fix <bits/endian.h>"
#endif
	char name[XIAP_NAMESIZ];	/* XIA username */
	struct in_addr addr;		/* IP address to work with */
};

#endif /* XIA_ACL_MSG_H */
