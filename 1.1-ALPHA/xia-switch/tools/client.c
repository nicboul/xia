/*
 * See COPYRIGHTS file
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <xia/journal.h>
#include <xia/mbuf.h>
#include <xia/netbus.h>
#include <xia/tun.h>
#include <xia/xtp.h>

#include "client.h"

void  client_tunnel_in(iface_t *iface)
{
	journal_ftrace(__func__);
	printf("client_tunne_in\n");

	int ret;
	int total, byteleft, xtp_frame_len;
	peer_t *peer;
	void *xtp_frame_buffer;

	port_t *port;
	port = (port_t *)iface->interface_data;
	
	// not auth'ed yet
	if (port->auth == PORT_NOT_AUTH) {
		printf("not auth yet !\n");
		return;
	}

	ret = iface->read(iface);
	journal_notice("client]> iface->read: %i iface->devname %s\n ", ret, iface->devname);
	peer = port->peer;
	if (peer == NULL) {
		journal_notice("client]> peer is NULL\n");
		return;
	}
	xtp_frame_buffer = xtp_encapsulate(0, iface->frame, ret, XTP_TYPE_ETHERNET);
	xtp_frame_len = ret + XTP_HEADER_SIZE;
	xtp_print(xtp_frame_buffer);
	ret = peer->send(peer, xtp_frame_buffer, xtp_frame_len);

	printf("client]> frame len %i\n", xtp_frame_len);
	printf("client]> peer send %i\n", ret);

	return;
}

void client_authenticate(peer_t *peer)
{
	journal_ftrace(__func__);
	printf("client authenticate\n");

	int ret;
	char *key = "password";
	void *xtp_frame_buffer;
	uint32_t xtp_frame_len;

	xtp_frame_buffer = xtp_encapsulate(0, key, 8, XTP_TYPE_XSM);
	xtp_frame_len = 8+XTP_HEADER_SIZE;
	xtp_print(xtp_frame_buffer);

	ret = peer->send(peer, (uint8_t *)xtp_frame_buffer, xtp_frame_len);

	printf("client]> frame len %i\n", xtp_frame_len);
	printf("client]> peer send %i\n", ret);

//	ret = peer->recv(peer);

//	printf("received %i: %s\n", ret, peer->data);
/*
	if (strcmp(peer->data, "hello") == 0)  {
		peer->send(peer, "session", 7);
		return;
	}
*/
	/* We've got an IP address 
	if (strncmp(peer->data, "192", 3) == 0) {
	 	
		client_peer = peer;	
		client_iface = netbus_newtun(client_imux_iface);
		tun_up(client_iface->devname, peer->data);
		peer->on_pollin = client_imux_peer;
	}
*/
	return;
}

void client_forward_frame(port_t *port, struct xtp *xtp_frame)
{
	journal_ftrace(__func__);
	printf("forward frame\n");

	int ret;
	iface_t *iface;
	iface = port->iface;
	ret = iface->write(iface, (uint8_t*)port->mbuf_head->ext_buf + XTP_HEADER_SIZE, xtp_get_len(port->mbuf_head->ext_buf));
}

void client_xsm(port_t *port, struct xtp *xtp_frame)
{
	printf("switch_xsm\n");
	char *ip = xtp_frame+1;

//	printf("data %s\n", ip);

	port->iface = netbus_newtun(client_tunnel_in);
	printf("devname %s\n", port->iface->devname);
	tun_up(port->iface->devname, ip);

	port->iface->interface_data = port;
	port->auth = PORT_AUTH;
}

int client_demux_xtp(port_t *port)
{
	journal_ftrace(__func__);
	printf("demux xtp\n");

	int ret;
	struct xtp *xtp_frame;

	while (port->mbuf_head != NULL) {

		xtp_frame = (struct xtp *)port->mbuf_head->ext_buf;

		switch (xtp_frame->type) {

			case XTP_TYPE_ETHERNET:
				client_forward_frame(port, xtp_frame);
			break;
 
			case XTP_TYPE_XSM:
				client_xsm(port, xtp_frame);
			break;

			default:
				journal_notice("client]> unknown XTP type :: %s:%i\n", __FILE__, __LINE__);
			break;
		}

		mbuf_del_packet(&(port->mbuf_head), port->mbuf_head);
	}
}

int client_frame_queue(void *opaque_ptr, void *frame, uint32_t len)
{
	journal_ftrace(__func__);
	printf("frame queue\n");	

	mbuf_t *mbuf;
	port_t *port;
	port = opaque_ptr;

	mbuf = mbuf_new(frame, len, MBUF_PKTHDR);
	mbuf_add_packet(&(port->mbuf_head), mbuf);
}

void client_on_input(peer_t *peer)
{
	journal_ftrace(__func__);
	printf("on input\n");        

	int ret;
	uint32_t len;
	port_t *port;

	port = peer->connection_data;

        ret = peer->recv(peer);
	printf("client]> peer->read: %i\n", ret);

	port->gb.data_buffer = peer->data;
	port->gb.data_buffer_len = ret;
	port->gb.proto_header_len = XTP_HEADER_SIZE;

	netbus_reassembly(&port->gb,
				&port->fb,
				xtp_get_len,
				client_frame_queue,
				port);

	client_demux_xtp(port);

	return;
}

void client_on_disconnect(peer_t *peer)
{
	printf("on_disconnect\n");
}

int main()
{
	printf("main\n");

	peer_t *peer;
	port_t *port;

	if (journal_init()) {
		journal_failure(EXIT_ERR, "client]> journal_init() failed:: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (event_init()) {
		journal_failure(EXIT_ERR, "client]> event_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (scheduler_init()) {
		journal_failure(EXIT_ERR, "client]> scheduler_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	if (netbus_init()) {
		journal_failure(EXIT_ERR, "client]> netbus_init() failed :: %s:%i\n", __FILE__, __LINE__);
		_exit(EXIT_ERR);
	}

	peer = netbus_newclient("192.168.2.6", 1100, client_on_disconnect, client_on_input);
	if (peer == NULL) {
		journal_notice("switch]> netbus_newclient failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	port = malloc(sizeof(port_t));
	port->iface = NULL;
	port->peer = peer;
	port->mbuf_head = NULL;
	port->auth = PORT_NOT_AUTH;
	port->fb.fragment = malloc(2000);

	peer->connection_data = port;

	client_authenticate(peer);

	scheduler();
	
	return 0;
}
