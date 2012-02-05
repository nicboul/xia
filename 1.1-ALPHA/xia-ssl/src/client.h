
#ifndef CLIENT_H
#define CLIENT_H

#include <xia/netbus.h>

extern int client_init(peer_t *);
extern void client_auth(peer_t *);
extern void client_imux_iface(iface_t *);
extern void client_imux_peer(peer_t *);

#endif
