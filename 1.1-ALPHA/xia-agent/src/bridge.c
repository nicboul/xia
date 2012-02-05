/*
 * See COPYRIGHTS file.
 */


#include <string.h>

#include <xia/event.h>
#include <xia/journal.h>
#include <xia/netbus.h>

#include <xia/xiap.h>
#include <xia/msg/bridge.h>

char *bridge_x509;
char *bridge_addr;
int bridge_port;

int bridge_get_owner_context(char *name)
{
	journal_ftrace(__func__);

	struct bridge_msg msg, *msg_reply;
	struct xiap x, *x_reply;

	peer_t *peer;
	char mbuf[SLABSIZ];

	memset(&x, 0, sizeof(struct xiap));
	memset(&msg, 0, sizeof(struct bridge_msg));

	peer = netbus_newclient(bridge_addr, bridge_port, NULL, NULL);
	if (peer == NULL)
		return -1;

	msg.version = XIA_MSG_BRIDGE_VERSION;
	strncpy((char *)&msg.name, name, XIAP_NAMESIZ);

	x.type = XIAMSG_BRIDGE;
	x.len = htons(sizeof(struct bridge_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

retry:
	memset(mbuf, 0, SLABSIZ);
        memcpy(mbuf + sizeof(struct xiap), &msg, sizeof(struct bridge_msg));
        memcpy(mbuf, &x, sizeof(struct xiap));

	if (peer->send(peer, mbuf, SLABSIZ) > 0 && peer->recv(peer) > 0) {
		x_reply = (struct xiap *)peer->data;
		switch (x_reply->status) {
			case XIAP_STATUS_OK:
				++x_reply;
				msg_reply = (struct bridge_msg *)x_reply;
				peer->disconnect(peer);
				return msg_reply->owned_by;

			case XIAP_STATUS_RETRY:
				goto retry;

			default:
			case XIAP_STATUS_FAILED:
				break;
		}
	}

	peer->disconnect(peer);
	return -1;
}

int set_context(char *x509cn)
{
	journal_ftrace(__func__);

	static int context = 0;

	journal_notice("bridge]> x.509 commonName is %s :: %s:%i\n", x509cn, __FILE__, __LINE__);

	if (context == 0)
		context = bridge_get_owner_context(x509cn);

	if (context == 0) {
		journal_notice("bridge]> context is still 0... anormal :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	
	if (context == -1) {
		journal_notice("bridge]> bridge_get_owner_context failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	journal_notice("bridge]> owner context is %u :: %s:%i\n", context, __FILE__, __LINE__);

	if (swap_context(x509cn, context)) {
		journal_notice("bridge]> failed to swap context in x509cn :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	journal_notice("bridge]> now using xia name %s :: %s:%i\n", x509cn, __FILE__, __LINE__);

	return 0;
}


void bridge_fini()
{
	journal_ftrace(__func__);

	free(bridge_x509);
	free(bridge_addr);
}

int bridge_init(char *x509, char *addr, uint16_t port)
{
	journal_ftrace(__func__);

	bridge_x509 = strdup(x509);
	bridge_addr = strdup(addr);
	bridge_port = port;
	
	if (set_context(bridge_x509)) {
		journal_notice("bridge]> set_context failed :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	event_register(EVENT_EXIT, "bridge_fini", bridge_fini, PRIO_AGNOSTIC);

	return 0;
}
