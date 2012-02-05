/*
 * See COPYRIGHTS file.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <xia/event.h>
#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/utils.h>
#include <xia/xtp.h>

#include "dnds.h"
#include "mgmt.h"
#include "switch.h"
  
static int dnds_authenticate(struct dnds_query *dnds_query)
{
	DNDS_NODE *qry_node, *ans_node;
	DNDS_PERM *node_perm;

	qry_node = (DNDS_NODE*)(dnds_query->dnds_hdr + 1);
	dnds_node_ntoh(qry_node);

	printf("dnds]> qry_node id %u\n", qry_node->id);
	printf("dnds]> qry_node type %u\n", qry_node->type);
	printf("dnds]> qry_node name %s\n", qry_node->name);
	printf("dnds]> qry_node flag %x\n", qry_node->flag);

	ans_node = dbal_node_get(qry_node);

	if (ans_node) {

		dnds_node_print(ans_node);
		dnds_query->dnds_sess->node_type = ans_node->type;
		dnds_query->dnds_sess->auth = DNDS_SESS_AUTH;
	}	
	else {
		free(ans_node);
		return -1;
	}

	node_perm = dbal_perm_get_by_node(ans_node);

	printf("dnds]> node_perm id %u\n", node_perm->id);
	printf("dnds]> node_perm name %s\n", node_perm->name);
	printf("dnds]> node_perm matrix %i\n", node_perm->matrix[0]);
	printf("dnds]> node_perm age %u\n", node_perm->age);

	free(ans_node);
	return 0;
}

static int dnds_preprocess_query(struct dnds_query *dnds_query)
{
	printf("dnds_preprocess_query\n");

	if (dnds_query->dnds_sess->auth == DNDS_SESS_NOT_AUTH) {

		if (dnds_query->dnds_hdr->subject == DNDS_HELLO)
			dnds_authenticate(dnds_query);
	}

	if (dnds_query->dnds_sess->auth == DNDS_SESS_AUTH) {

		switch (dnds_query->dnds_sess->node_type) 
		{
			case DNDS_NODE_TYPE_SWITCH:
				switch_process_query(dnds_query);
				break;

			case DNDS_NODE_TYPE_MGMT:
				mgmt_process_query(dnds_query);
				break;

			default:
				journal_notice("dnds]> disconnect socket {%i}, unexpected node type {%i} :: %s:%i\n",
						dnds_query->dnds_sess->peer->socket, 
						dnds_query->dnds_sess->node_type, __FILE__, __LINE__);
				dnds_query->dnds_sess->peer->disconnect(dnds_query->dnds_sess->peer);
		}
	}
	else {
		printf("DNDS_GOODBYE!\n");
		// send DNDS_GOODBYE
		// terminate dnds session
	}

	return 0;
}

static int dnds_frame_queue(void *opaque_ptr, void *frame, uint32_t len)
{
	printf("dnds_frame_queue\n");

	struct xtp *xtp_frame;
	struct dnds_session *dnds_sess;
	uint8_t subchannel_id;
	mbuf_t *mbuf;

	dnds_sess = opaque_ptr;
	xtp_frame = frame;
	subchannel_id = xtp_frame->subchannel;

	printf("frame queue subchannel %u\n", xtp_frame->subchannel);

	mbuf = mbuf_new(frame, len, MBUF_PKTHDR);
	mbuf_add_packet(&(dnds_sess->subchannel[subchannel_id].mbuf), mbuf);

	return 0;
}

static int dnds_demux_subchannel(struct dnds_session *dnds_sess)
{
	struct dnds_query *dnds_query;
	size_t subchannel_id;
	mbuf_t **mbuf_head;
	int ret;

	printf("dnds_demux_frame\n");

	for (subchannel_id = 0; subchannel_id < XTP_SUBCHANNEL_MAX; subchannel_id++) {

		mbuf_head = &(dnds_sess->subchannel[subchannel_id].mbuf);

		while (*mbuf_head != NULL) {

			xtp_print((*mbuf_head)->ext_buf);

			// create dnds query from the current XTP frame.
			dnds_query = malloc(sizeof(struct dnds_query));
			dnds_query->dnds_sess = dnds_sess;
			dnds_query->subchannel_id = subchannel_id;
			dnds_query->dnds_hdr = (struct dnds*)(((uint8_t*)(*mbuf_head)->ext_buf) + XTP_HEADER_SIZE); 

			ret = dnds_preprocess_query(dnds_query);
			mbuf_del_packet(mbuf_head, *mbuf_head);
		}
	}

	return 0;
}

static void dnds_on_input(peer_t *peer)
{
	printf("dnds_on_input\n");

	int ret;
	struct dnds_session *dnds_sess;

	ret = peer->recv(peer);
	dnds_sess = peer->connection_data;

	dnds_sess->gb->data_buffer = peer->data;
	dnds_sess->gb->data_buffer_len = ret;
	dnds_sess->gb->proto_header_len = XTP_HEADER_SIZE;

	netbus_reassembly(dnds_sess->gb,
				dnds_sess->fb,
				xtp_get_len,
				dnds_frame_queue,
				dnds_sess);

	dnds_demux_subchannel(dnds_sess);
}

static void dnds_on_disconnect(peer_t *peer)
{
	printf("dnds_on_disconnect\n");
//	terminate dnds session

}

static void dnds_on_connect(peer_t *peer)
{
	struct dnds_session *dnds_sess;

	printf("dnds_on_connect\n");	
	
	dnds_sess = malloc(sizeof(struct dnds_session));
	dnds_sess->timestamp = time(NULL);
	dnds_sess->peer = peer;
	dnds_sess->auth = DNDS_SESS_NOT_AUTH;

	dnds_sess->fb = malloc(sizeof(frag_buffer_t));
	dnds_sess->fb->fragment = malloc(2000);
	dnds_sess->fb->head_len = 0;
	dnds_sess->fb->tail_len = 0;

	dnds_sess->gb = malloc(sizeof(generic_buffer_t));
	dnds_sess->gb->data_buffer = NULL;
	dnds_sess->gb->data_buffer_len = 0;
	dnds_sess->gb->proto_header_len = 0;

	memset(dnds_sess->subchannel, NULL, sizeof(subchannel_t) * XTP_SUBCHANNEL_MAX);

	peer->connection_data = dnds_sess;

//	timer_add(dnds_session_timeout, dnds_sess, 5);
}

void dnds_fini(void *session_data)
{
	journal_ftrace(__func__);
}

int dnds_init(char *listen_address, int tcp_port)
{
	journal_ftrace(__func__);
	
	int ret;

	event_register(EVENT_EXIT, "dnds]> dnds_fini", dnds_fini, PRIO_AGNOSTIC);
	ret = netbus_newserv(listen_address, tcp_port,  dnds_on_connect, dnds_on_disconnect, dnds_on_input);
	if (ret < 0) {
		journal_notice("dnds]> netbus_newserv failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	ret = switch_init();
	if (ret < 0) {
		journal_notice("dnds]> switch_init failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	ret = mgmt_init();
	if (ret < 0) {
		journal_notice("dnds]> mgmt_init failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}
