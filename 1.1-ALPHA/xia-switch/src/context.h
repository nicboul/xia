/*
 * See COPYRIGHTS file.
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <xia/bridge.h>
#include <xia/netbus.h>
#include <xia/mbuf.h>

#include "fib.h"
#include "ippool.h"
#include "switch.h"

#define PORT_SESSION	0x1
#define PORT_MANAGEMENT	0x2

typedef struct context {

	int id;					// context unique ID
	fib_entry_t *fib_cache[1024];		// forwarding information base

	bridge_t *bridge;			// bridge interface
	iface_t *tunnel_in;			// tunnel in interface
	iface_t *tunnel_out;			// tunnel out interface
	ippool_t *ippool;			// ip address pool

	port_t *port_session;			// port list of type 'session'
	port_t *port_management;		// port list of type 'management'

} context_t;

void context_add_port(context_t *, port_t *port);
context_t *context_lookup(uint32_t);

void context_fini(void *);
int context_init(void (*)(iface_t *));

#endif /* CONTEXT_H */
