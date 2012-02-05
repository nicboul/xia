
#include <stdio.h>

#include <xia/dnds.h>
#include <xia/journal.h>
#include <xia/netbus.h>
#include <xia/sched.h>
#include <xia/xtp.h>

static void client_xtp_input(peer_t *peer)
{

}

static void client_auth(peer_t *peer)
{
	int ret;
	ret = peer->recv(peer);

	printf("received %i bytes ]> %s\n", ret, peer->data);
	peer->send(peer, "switch", 6);
	peer->on_pollin = client_xtp_input;
}

peer_t *peer;
static void client_send_data()
{
	struct xtp *xtp_frame;
	struct dnds *dnds_msg;
	DNDS_NODE node;

	printf("client_send_data\n");
	if (peer == NULL)
		return;

	printf("client sennnedd data\n");
	memset(&node, 0, sizeof(DNDS_NODE));
	node.id = 1;
	node.type = DNDS_NODE_TYPE_SWITCH;
	strncpy(node.name, "switch\0", 7);
	node.perm = 3;
	node.flag = 0;
	node.age = 25;

	dnds_node_hton(&node);	
	dnds_msg = dnds_encapsulate(DNDS_HELLO, &node, sizeof(DNDS_NODE));
	xtp_frame = xtp_encapsulate(0, dnds_msg, DNDS_HEADER_SIZE + sizeof(DNDS_NODE));
	
	printf("before send\n");
	peer->send(peer, xtp_frame, XTP_HEADER_SIZE + DNDS_HEADER_SIZE + sizeof(DNDS_NODE));
	printf("after send\n");

	free(xtp_frame);
	free(dnds_msg);
}

void on_disconnect(peer_t *peerr)
{
	peer = NULL;
	printf("ON EST DANS LE ON_DISCONNECT !\n");
//	journal_failure(EXIT_ERR, "client]> we've got disconnected from the server :: %s:%i\n"__FILE__, __LINE__);
//	exit(EXIT_ERR);
}

int main()
{

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


	peer = netbus_newclient("127.0.0.1", 9090, on_disconnect, client_auth);
	if (peer == NULL) {
		journal_notice("client]> netbus_newclient failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	sched_register(SCHED_PERIODIC, "client_send_data", client_send_data, 5, NULL);

	scheduler();

}
