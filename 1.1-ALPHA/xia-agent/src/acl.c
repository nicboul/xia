/*
 * See COPYRIGHTS file.
 */


#include <string.h>

#include <xia/event.h>
#include <xia/netbus.h>

#include <xia/xiap.h>
#include <xia/msg/acl.h>

#include "reslist.h"

char * acl_x509;
char * acl_addr;
int acl_port;

static int acl_send(struct acl_msg *msg)
{
	journal_ftrace(__func__);

	struct xiap x, *x_reply;

	peer_t *peer;
	char mbuf[SLABSIZ];

	memset(&x, 0, sizeof(struct xiap));

	peer = netbus_newclient(acl_addr, acl_port, NULL, NULL);
	if (peer == NULL)
		return 1;

	msg->version = XIA_MSG_ACL_VERSION;

	x.type = XIAMSG_ACL;
	x.len = htons(sizeof(struct acl_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

retry:
	memset(mbuf, 0, SLABSIZ);
	memcpy(mbuf + sizeof(struct xiap), msg, sizeof(struct acl_msg));
	memcpy(mbuf, &x, sizeof(struct xiap));

	if (peer->send(peer, mbuf, SLABSIZ) > 0 && peer->recv(peer) > 0) {
		x_reply = (struct xiap *)peer->data;
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
	journal_ftrace(__func__);
	
	struct acl_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = XIA_MSG_ACL_ADD;
	msg.addr.s_addr = entry->addr.s_addr;
	strncpy((char *)&msg.name, acl_x509, XIAP_NAMESIZ);

	ret = acl_send(&msg);

	return ret;
}

int acl_res_del(reslist_t *entry)
{
	journal_ftrace(__func__);
	
	struct acl_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = XIA_MSG_ACL_DEL;
	msg.addr.s_addr = entry->addr.s_addr;
	strncpy((char *)&msg.name, acl_x509, XIAP_NAMESIZ);

	ret = acl_send(&msg);

	return ret;
}

static void acl_fini()
{
	journal_ftrace(__func__);

	free(acl_x509);
	free(acl_addr);
}

int acl_init(char *x509, char *addr, uint16_t port)
{
	journal_ftrace(__func__);

	acl_x509 = strdup(x509);
	set_context(acl_x509);

	acl_addr = strdup(addr);
	acl_port = port;

	event_register(EVENT_EXIT, "acl_fini", acl_fini, PRIO_AGNOSTIC);

	return 0;
}
