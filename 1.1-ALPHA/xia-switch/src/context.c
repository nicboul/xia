/*
 * See COPYRIGHTS file
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <xia/bridge.h>
#include <xia/event.h>
#include <xia/hash.h>
#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/tun.h>
#include <xia/xmp.h>

#include "context.h"
#include "inet.h"

void (*context_cb)(iface_t *) = NULL;

#define CONTEXT_LIST_SIZE 256
context_t *context_table[CONTEXT_LIST_SIZE] = {NULL};

void context_add_port(context_t *context, port_t *port)
{
	if (port->type == PORT_SESSION) {

		if (context->port_session == NULL)
			context->port_session = port;
		else {
			port->next = context->port_session;
			context->port_session->prev = port;
			context->port_session = port;
		}

	}
}

context_t *context_lookup(uint32_t context_id)
{
	if (context_id >= 0 && context_id < CONTEXT_LIST_SIZE)	
		return context_table[context_id];

	return NULL;
}

int context_create(uint8_t context_id, char *ip_local, char *ip_begin, char *ip_end, char *netmask )
{
	journal_ftrace(__func__);

	struct in_addr;
	context_t *context;

	journal_notice("context]> ctx_id :: %i\n", context_id);
	journal_notice("context]> local  :: %s\n", ip_local);
 	journal_notice("context]> begin  :: %s\n", ip_begin);
	journal_notice("context]> end    :: %s\n", ip_end);
	journal_notice("context]> mask   :: %s\n", netmask);

	context = (context_t*)malloc(sizeof(context_t));
	context_table[context_id] = context;

	context->ippool = ippool_new(ip_begin, ip_end);

	context->id = context_id;
	context->bridge = bridge_create(context->id);
	
	// no port yet
	context->port_session = NULL;

	// TUN <- BRIDGE -> TUN	
	context->tunnel_in = netbus_newtun(context_cb);
	context->tunnel_in->interface_data = context;
	tun_up(context->tunnel_in->devname, ip_local);
	journal_notice("ctx]> tunnel_in %s\n", context->tunnel_in->devname);

	context->tunnel_out = netbus_newtun(context_cb);
	context->tunnel_out->interface_data = context;
	tun_up(context->tunnel_out->devname, "0");
	journal_notice("ctx]> tunnel_out %s\n", context->tunnel_out->devname);

	bridge_add(context->bridge, context->tunnel_in);
	bridge_add(context->bridge, context->tunnel_out);

	return 0;
}

void context_fini(void *z)
{
	journal_ftrace(__func__);

	int i = 1;
	context_t *context;
/*
	while (context_table[i] != NULL) {

		context = context_table[i];

		bridge_destroy(context->bridge);

		tun_destroy(context->tunnel_out);
		tun_destroy(context->tunnel_in);
		
		i++;		
	}
*/
}

int context_init(void (*cb)(iface_t *))
{
	journal_ftrace(__func__);
	int ret;

	event_register(EVENT_EXIT, "context:context_fini()", context_fini, PRIO_AGNOSTIC);
	context_cb = cb;

//	ret = dbal_get_all_context_pool(ctx_create);
	context_create(1, "192.168.10.1", "192.168.10.2", "192.168.10.254", "255.255.255.255");

	return 0;	
}
