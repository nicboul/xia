/*
 * See COPYRIGHTS file.
 */


#ifndef XIA_XTP_H
#define XIA_XTP_H

#ifndef __USE_BSD
#define __USE_BSD
#endif

#include <stdint.h>
#include <sys/types.h>

#define XTP_VERSION		1
#define XTP_SUBCHANNEL_MAX	256

#define XTP_TYPE_ETHERNET	0x1
#define XTP_TYPE_DNDS		0x2
#define XTP_TYPE_XSM		0x4

struct xtp {
	uint8_t version;
	uint8_t flag;
	uint16_t len;
	uint8_t subchannel;
	uint8_t type;
} __attribute__((__packed__));

/*
struct xtp {
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t flag:4;
	uint8_t int version:4;
#elif BYTE_ORDER == BIG_ENDIAN
	uint8_t version:4;		// version
	uint8_t flag:4;			// flag
#else
# error "Please fix BYTE_ORDER"
#endif
	uint8_t pad;
	uint16_t len;			// payload length
};
*/
#define XTP_HEADER_SIZE sizeof(struct xtp)

struct xtp_session {
};

void xtp_print(void *);
uint16_t xtp_get_len(void *);
uint8_t xtp_get_version(void *);
struct xtp *xtp_encapsulate(uint8_t, void *, uint16_t, uint8_t);


#endif /* XIA_XTP_H */
