
/*
 * See COPYRIGHTS file.
 */

#include <string.h>

#include <xia/xiap.h>
#include <xia/bridge_msg.h>

#include "../lib/netbus.h"

extern char *xiad_addr;
extern uint16_t *xiad_port;

unsigned int bridge_get_owner_context(char *name)
{
	journal_ftrace(__func__);

	struct bridge_msg msg, *msg_reply;
	struct xiap x, *x_reply;

	peer_t *peer;
	char mbuf[SLABSIZ];

	memset(&x, 0, sizeof(struct xiap));
	memset(&msg, 0, sizeof(struct bridge_msg));

	peer = netbus_newclient(xiad_addr, *xiad_port, NULL, NULL);
	if (peer == NULL)
		return 0;

	msg.version = BRIDGE_MSG_VERSION;
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
		x_reply = (struct xiap *)peer->buf;
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
	return 0; // 0 means a failure here
}
