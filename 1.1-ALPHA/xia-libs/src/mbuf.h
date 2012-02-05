/*
 * See COPYRIGHTS file.
 */

#ifndef MBUF_H
#define MBUF_H

#include <stdint.h>
#include <sys/types.h>

#define MBUF_MORE_FRAGMENT	0x1
#define MBUF_PKTHDR		0x2

typedef struct mbuf {

	struct mbuf *next, *prev;		/* next buffer in chain */
	struct mbuf *nextpkt, *prevpkt;		/* next chain in list */
	uint8_t mh_flags;			/* flags M_ */

	uint8_t *ext_buf;			/* start of buffer */
	uint32_t ext_size;			/* size of buffer */

} mbuf_t;

int mbuf_add_in_chain(mbuf_t **, mbuf_t *);
int mbuf_del_in_chain(mbuf_t **, mbuf_t *);
int mbuf_add_packet(mbuf_t **, mbuf_t *);
int mbuf_del_packet(mbuf_t **, mbuf_t *);
mbuf_t *mbuf_new(uint8_t *, uint32_t, uint16_t);

#endif /* MBUF_H */
