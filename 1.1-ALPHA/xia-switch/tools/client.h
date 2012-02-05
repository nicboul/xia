/*
 * See COPYRIGHTS file.
 */


#ifndef CLIENT_H
#define CLIENT_H

#include <xia/netbus.h>

#define PORT_AUTH	0x1
#define PORT_NOT_AUTH	0x2

typedef struct port {

	iface_t *iface;
	peer_t *peer;

	generic_buffer_t gb;
	frag_buffer_t fb;

	mbuf_t *mbuf_head;
	uint8_t auth;

} port_t;
	

extern int client_init(char *, int);
extern void client_auth(peer_t *);
extern void client_imux_iface(iface_t *);
extern void client_imux_peer(peer_t *);

#endif /* CLIENT_H */
