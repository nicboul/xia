
#ifndef SERVER_H
#define SERVER_H

#include <xia/netbus.h>

extern int server_init();
extern void server_auth(peer_t *);
extern void server_handshake(peer_t *);

extern void server_imux_iface(iface_t *);
extern void server_imux_peer(peer_t *);

extern void server_disconnect(peer_t *);

#endif
