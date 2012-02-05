/*
 * See COPYRIGHTS file.
 */

#ifndef DNDS_H
#define DNDS_H

#include <xia/dnds.h>
#include <xia/netbus.h>
#include <xia/mbuf.h>
#include <xia/xtp.h>

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#define DNDS_SESS_NOT_AUTH	0x1
#define DNDS_SESS_AUTH		0x2

struct dnds_query {

	struct dnds_session *dnds_sess;
	uint8_t subchannel_id;
	struct dnds *dnds_hdr;
};

typedef struct subchannel {
	mbuf_t *mbuf;
} subchannel_t;

struct dnds_session {

	time_t timestamp;
	peer_t *peer;
	subchannel_t subchannel[XTP_SUBCHANNEL_MAX];

	uint8_t auth;
	uint8_t perm_matrix[DNDS_SUBJECT_COUNT];
	uint8_t node_type;

	frag_buffer_t *fb;
	generic_buffer_t *gb;

};

extern int dnds_init(char *, int);

#endif /* DNDS_H */
