
/*
 * See COPYRIGHTS file.
 */

#ifndef XIA_BRIDGE_MSG_H
#define XIA_BRIDGE_MSG_H

#include <stdint.h>
#include <sys/types.h>

#include <xia/xiap.h>

#define BRIDGE_MSG_VERSION	1

struct bridge_msg {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int padding:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;		/* Protocol version */
	unsigned int padding:4;		/* Padding */
#else
# error "Please fix <bits/endian.h>"
#endif
	char name[XIAP_NAMESIZ];	/* XIA username */
	uint32_t owned_by;		/* owned by this context */
};

#endif /* XIA_BRIDGE_MSG_H */
