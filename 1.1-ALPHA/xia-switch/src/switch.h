/*
 * See COPYRIGHTS file.
 */

#ifndef SWITCH_H
#define SWITCH_H

#include <xia/netbus.h>
#include <xia/mbuf.h>

#include "fib.h"

#define PORT_AUTH	0x1
#define PORT_NOT_AUTH	0x2

#define SWITCH_SERVER 0x1
#define SWITCH_CLIENT 0x2

typedef struct port {

	char *ip;			// peer ip address
	uint8_t type;			// port type
	uint8_t auth;			// is the port authenticated

	peer_t *peer;			// peer's informations
	struct context *context;	// context's informations that the port belong to

	frag_buffer_t fb;		// fragmented buffer information
	generic_buffer_t gb;		// generic buffer information
	mbuf_t *mbuf_head;		// list of buffered packets

	struct fib_entry *fib_entry_list;	// list of all hosts behind the peer

	struct port *next;		// next port in list
	struct port *prev;		// previous port in list

} port_t;

extern int switch_init(char *, int);
extern void switch_auth(peer_t *);
extern void switch_handshake(peer_t *);

extern void switch_demux_tunnel_in(iface_t *);
extern void switch_demux_tunnel_out(peer_t *);

extern void switch_disconnect(peer_t *);

#endif /* SWITCH_H */
