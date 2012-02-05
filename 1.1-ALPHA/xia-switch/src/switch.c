/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <xia/event.h>
#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/xsm.h>
#include <xia/xtp.h>

#include "context.h"
#include "fib.h"
#include "inet.h"
#include "switch.h"

void switch_tunnel_in(iface_t *tunnel_in)
{
	journal_ftrace(__func__);

	int ret;
	int mac_addr_type;
	uint8_t mac_addr_dst[6];

	void *xtp_frame_buffer;
	int xtp_frame_len;

	fib_entry_t *fib_entry;
	context_t *context;
	port_t *port;

	context = tunnel_in->interface_data;
	if (context == NULL) {
		journal_notice("switch]> the context is NULL :: %s:%i\n", __FILE__, __LINE__);
		return;
	}

	if (tunnel_in  == context->tunnel_in) {
		journal_notice("switch]> from tunnel in %s\n", context->tunnel_in->devname);
	}
	else if (tunnel_in == context->tunnel_out) {
		journal_notice("switch]> from tunnel out %s\n", context->tunnel_out->devname);
	}
	
	ret = tunnel_in->read(tunnel_in);
	if (ret == -1) {
		journal_notice("switch]> tunnel in read error :: %s:%i\n", __FILE__, __LINE__);
		return;
	}

	xtp_frame_buffer = xtp_encapsulate(0, tunnel_in->frame, ret, XTP_TYPE_ETHERNET);
	xtp_frame_len = ret + XTP_HEADER_SIZE;

	inet_get_mac_addr_dst(tunnel_in->frame, mac_addr_dst);
	mac_addr_type = inet_get_mac_addr_type(mac_addr_dst);

	fib_entry = fib_lookup(context->fib_cache, mac_addr_dst);

	// Switch Forwarding
	if (mac_addr_type == ADDR_UNICAST	// the destination address is unicast
		&& fib_entry != NULL 		// AND the fib entry exist
		&& fib_entry->port != NULL) {	// AND the fib port is up

			printf("switch]> Switch Forwarding\n");

			port = fib_entry->port;
			ret = port->peer->send(port->peer, xtp_frame_buffer, xtp_frame_len);
			printf("switch]> peer sent {%i} bytes to %s\n", ret, port->ip);

	} 
	// Switch Flooding
	else if (mac_addr_type == ADDR_BROADCAST || 	// this packet has to be broadcasted
		fib_entry == NULL || 			// OR the fib entry doesn't exist
		fib_entry->port == NULL)  {		// OR the fib port is unknown

			printf("switch]> Switch Flooding\n");

			port = context->port_session;
			if (port == NULL) {
				journal_notice("switch]> the port session list is empty :: %s:%i\n", __FILE__, __LINE__);
			}

			for (; port != NULL; port = port->next) {
				ret = port->peer->send(port->peer, xtp_frame_buffer, xtp_frame_len);
				printf("switch]> peer sent {%i} bytes to %s\n", ret, port->ip);

			}
	}

	free(xtp_frame_buffer);
	return;
}

void switch_filter(port_t *port, struct xtp *xtp_frame)
{
	journal_ftrace(__func__);
	printf("switch_filter\n");

	int ret;
	iface_t *iface;
	fib_entry_t *fib_entry;
	uint8_t mac_addr_src[6];
	uint8_t mac_addr_type;

	if (port->auth == PORT_NOT_AUTH) {
		journal_notice("switch]> the port is not auth'ed :: %s:%i\n", __FILE__, __LINE__);
		return;
	}

	inet_get_mac_addr_src((uint8_t *)xtp_frame + XTP_HEADER_SIZE , mac_addr_src);
	mac_addr_type = inet_get_mac_addr_type(mac_addr_src);

	fib_entry = fib_lookup(port->context->fib_cache, mac_addr_src);
	
	if (fib_entry == NULL || fib_entry->port == NULL) {

		fib_entry = malloc(sizeof(fib_entry_t));
		fib_entry->port = port;
		memcpy(fib_entry->mac_addr, mac_addr_src, 6);
		fib_add(port->context->fib_cache, fib_entry);

		// update the port's fib entry list
		if (port->fib_entry_list == NULL) {
			fib_entry->next = NULL;
			fib_entry->prev = NULL;
			port->fib_entry_list = fib_entry;
		}
		else {
			fib_entry->next = port->fib_entry_list;
			port->fib_entry_list->prev = fib_entry;
			port->fib_entry_list = fib_entry;
		}
	}

	iface = port->context->tunnel_out;
	ret = iface->write(iface, (uint8_t *)xtp_frame + XTP_HEADER_SIZE, xtp_get_len(xtp_frame));
}

void switch_xsm(port_t *port, struct xtp *xtp_frame)
{
	journal_ftrace(__func__);
	printf("switch_xsm\n");

	// XXX Here we will demux XSM packets

	peer_t *peer;
	char *ip;
	void *xtp_frame_buffer;
	int xtp_frame_len;

	struct xsm *xsm_request;
	xsm_request = xtp_frame+1;
	xsm_ntoh(xsm_request);

	printf("xsm version    %u\n", xsm_request->version);
	printf("xsm request id %u\n", xsm_request->request_id);

	return;

	peer = port->peer;
	port->context = context_lookup(1);
	
	ip = ippool_get_ip(port->context->ippool);
	port->ip = strdup(ip);
	printf("ip %s\n", port->ip);

	xtp_frame_buffer = xtp_encapsulate(0, port->ip, strlen(port->ip), XTP_TYPE_XSM);
	xtp_frame_len = strlen(port->ip) + XTP_HEADER_SIZE;
	peer->send(peer, xtp_frame_buffer, xtp_frame_len);

	if (port->context->port_session == NULL)
		port->context->port_session = port;
	else {
		port->next = port->context->port_session;
		port->context->port_session->prev = port;
		port->context->port_session = port;
	}

	port->auth = PORT_AUTH;
}

void switch_demux_xtp(port_t *port)
{
	journal_ftrace(__func__);
	printf("switch_demux_xtp\n");

	int ret;
	struct xtp *xtp_frame;

	while (port->mbuf_head != NULL) {
		
		xtp_frame = (struct xtp *)port->mbuf_head->ext_buf;
		switch (xtp_frame->type) {

			case XTP_TYPE_ETHERNET:
				switch_filter(port, xtp_frame);
			break;

			case XTP_TYPE_XSM:	
				switch_xsm(port, xtp_frame);
			break;

			default:
				journal_notice("switch]> XTP type unknown {%i} :: %s:%i \n", xtp_frame->type, __FILE__, __LINE__);
			break;
		}

		mbuf_del_packet(&(port->mbuf_head), port->mbuf_head);
	}
}

void switch_frame_queue(void *opaque_ptr, void *frame, uint32_t len)
{
	journal_ftrace(__func__);
	printf("frame queue\n");

	port_t *port;
	mbuf_t *mbuf;

	port = (port_t *)opaque_ptr;

	mbuf = mbuf_new(frame, len, MBUF_PKTHDR);
	mbuf_add_packet(&(port->mbuf_head), mbuf);
}

void switch_on_input(peer_t *peer)
{
	journal_ftrace(__func__);
	journal_notice("switch_on_input\n");
        
	int ret;
	port_t *port;

	ret = peer->recv(peer);
	port = peer->connection_data;
	if (port == NULL) {
		printf("switch]> port is NULL :: %s:%i\n", __FILE__, __LINE__);
		return;
	}

	printf("switch]> received {%i} bytes from {%s}\n", ret);

	port->gb.data_buffer = peer->data;
	port->gb.data_buffer_len = ret;
	port->gb.proto_header_len = XTP_HEADER_SIZE;

	netbus_reassembly(&port->gb,
				&port->fb,
				xtp_get_len,
				switch_frame_queue,
				port);

	switch_demux_xtp(port);

	return;
}

void switch_on_connect(peer_t *peer)
{
	journal_ftrace(__func__);
	journal_notice("switch_on_connect\n");

	port_t *port;
	port = (port_t *)malloc(sizeof(port_t));
	port->next = NULL;
	port->prev = NULL;

	port->auth = PORT_NOT_AUTH;

	port->fb.fragment = malloc(2000);
	port->fb.head_len = 0;
	port->fb.tail_len = 0;

	port->gb.data_buffer = NULL;
	port->gb.data_buffer_len = 0;
	port->gb.proto_header_len = 0;

	port->mbuf_head = NULL;
	port->peer = peer;
	port->fib_entry_list = NULL;
	peer->connection_data = port;

	return;
}

void switch_on_disconnect(peer_t *peer)
{
	journal_ftrace(__func__);
	journal_notice("switch_disconnect\n");

	int ret;
	port_t *port;
	fib_entry_t *fib_entry;
	port = peer->connection_data;

	if (port->auth == PORT_NOT_AUTH) {
		free(port);
		return;
	}

	journal_notice("server]> disconnecting ip {%s}\n", port->ip); 

	// remove the port from the context port session list
	if (port == port->context->port_session) {
		port->context->port_session = port->next;	
	}
	else {
		port->prev->next = port->next;
		if (port->next)
			port->next->prev = port->prev;
	}
	
	// nullify the port's fib entries 
	for (fib_entry = port->fib_entry_list; 
		fib_entry != NULL; fib_entry = fib_entry->next) {

		if (fib_entry)
			fib_entry->port = NULL;
	}
	
	// release the ip address
	ippool_release_ip(port->context->ippool, port->ip);
	
	free(port->ip);
	free(port);

	return;
}

int switch_fini(void *data)
{

}

int switch_init(char *listen_address, int port_number)
{
	journal_ftrace(__func__);
	int ret;

	event_register(EVENT_EXIT, "switch]> switch_fini", switch_fini, PRIO_AGNOSTIC);
	ret = netbus_newserv(listen_address, port_number, switch_on_connect, switch_on_disconnect, switch_on_input);
	if (ret < 0) {
		journal_notice("switch]> netbus_newserv failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
#if 0
	ret = netbus_newserv("192.168.2.6", switch_port, switch_handshake, switch_disconnect, switch_auth);
	if (ret < 0) {
		journal_notice("switch]> netbus_newserv failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
#endif

	context_init(switch_tunnel_in);	
	return 0;
}
