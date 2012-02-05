
/*
 * See COPYRIGHTS file.
 */

#ifndef XIA_XIAP_H
#define XIA_XIAP_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#define XIAP_VERSION	1

#define XIAP_NAMESIZ	64	/* Max username length */

/* Numbering of XIA messages type */

enum {
	XIAMSG_SESSION = 1,
	XIAMSG_ACL,
	XIAMSG_BRIDGE,
	XIAMSG_DISCOVERY,
	XIAMSG_MAX
};

#define XIAMSG_MAXSIZ 512	/* Max message length */

/* Possible query status */

enum {
	XIAP_STATUS_OK = 0,	/* execution succeeded */
	XIAP_STATUS_FAILED,	/* execution failed */
	XIAP_STATUS_EXEC,	/* please execute this ! */
	XIAP_STATUS_NOLISTEN,	/* no subsystem listening */
	XIAP_STATUS_UNEXPECTED,	/* unexpected error happenned */
	XIAP_STATUS_RETRY,	/* a retry might work */
};

/*
 * We use these flags to classify the kind of possible nodes.
 * XIA deals with different type of nodes such as a user,
 * a ressource, a bridge or a tunnel.
 *
 * This flag is also used in the database.
 * See the `type` field of `tokens` table in `database/core.sql`
 */

enum {
	XIA_NODE_TYPE_NULL = 0,
	XIA_NODE_TYPE_USR,
	XIA_NODE_TYPE_RES,
	XIA_NODE_TYPE_BRIDGE_USR,
	XIA_NODE_TYPE_BRIDGE_RES,
	XIA_NODE_TYPE_TUNNEL
};

struct xiap {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int type:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4; /* Protocol version */
	unsigned int type:4;	/* XIA message type */
#else
# error "Please fix <bits/endian.h>"
#endif
	uint8_t status;		/* Query status */
	uint16_t len;		/* XIA message length */
};

#define SLABSIZ sizeof(struct xiap) + XIAMSG_MAXSIZ

#endif /* XIA_XIAP_H */
