/*
 * See COPYRIGHTS file.
 */

#include <string.h>

#include <xia/xiap.h>
#include <xia/acl_msg.h>

#include "../lib/netbus.h"

#include "reslist.h"

extern char *x509cn, *xiad_addr;
extern uint16_t *xiad_port;

int acl_send(struct acl_msg *msg)
{
	journal_strace("acl_send");

	struct xiap x, *x_reply;

	peer_t *peer;
	char mbuf[SLABSIZ];

	memset(&x, 0, sizeof(struct xiap));

	peer = netbus_newclient(xiad_addr, *xiad_port, NULL, NULL);
	if (peer == NULL)
		return 1;

	msg->version = ACL_MSG_VERSION;

	x.type = XIAMSG_ACL;
	x.len = htons(sizeof(struct acl_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

retry:
	memset(mbuf, 0, SLABSIZ);
	memcpy(mbuf + sizeof(struct xiap), msg, sizeof(struct acl_msg));
	memcpy(mbuf, &x, sizeof(struct xiap));

	if (peer->send(peer, mbuf, SLABSIZ) > 0 && peer->recv(peer) > 0) {
		x_reply = (struct xiap *)peer->buf;
		switch (x_reply->status) {
			case XIAP_STATUS_OK:
				peer->disconnect(peer);
				return 0;

			case XIAP_STATUS_RETRY:
				goto retry;

			default:
			case XIAP_STATUS_FAILED:
				break;
		}

	}

	peer->disconnect(peer);
	return 1;
}

int acl_res_add(reslist_t *entry)
{
	journal_strace("acl_res_add");
	
	struct acl_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = ACL_MSG_ADD;
	msg.addr.s_addr = entry->addr.s_addr;
	strncpy((char *)&msg.name, x509cn, XIAP_NAMESIZ);

	ret = acl_send(&msg);

	return ret;
}

int acl_res_del(reslist_t *entry)
{
	journal_strace("acl_res_del");
	
	struct acl_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = ACL_MSG_DEL;
	msg.addr.s_addr = entry->addr.s_addr;
	strncpy((char *)&msg.name, x509cn, XIAP_NAMESIZ);

	ret = acl_send(&msg);

	return ret;
}
