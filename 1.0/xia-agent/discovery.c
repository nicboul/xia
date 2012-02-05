
/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#include <xia/xiap.h>
#include <xia/discovery_msg.h>

#include "../lib/event.h"
#include "../lib/journal.h"
#include "../lib/netbus.h"

#include "reslist.h"

char *discovery_x509;
char *discovery_addr;
int discovery_port;

int discovery_send(struct discovery_msg *msg)
{
	journal_ftrace(__func__);

	struct xiap x, *x_reply;

	peer_t *peer;
	char mbuf[SLABSIZ];

	memset(&x, 0, sizeof(struct xiap));

	peer = netbus_newclient(discovery_addr, discovery_port, NULL, NULL);
	if (peer == NULL)
		return 1;

	msg->version = DISCOVERY_MSG_VERSION;
	strncpy((char *)msg->name, discovery_x509, XIAP_NAMESIZ);

	x.type = XIAMSG_DISCOVERY;
	x.len = htons(sizeof(struct discovery_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

retry:
	memset(mbuf, 0, SLABSIZ);
        memcpy(mbuf + sizeof(struct xiap), msg, sizeof(struct discovery_msg));
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

int discovery_set_res_addr(reslist_t *entry)
{
	journal_ftrace(__func__);

	static struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_ADDR;
	msg.addr.s_addr = entry->addr.s_addr;
	memcpy(msg.haddr, entry->chaddr, DISCOVERY_MSG_HADDRSIZ);

	ret = discovery_send(&msg);

	return ret;
}

int discovery_set_res_hostname(reslist_t *entry)
{
	journal_ftrace(__func__);

	struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_HOSTNAME;
	memcpy(msg.haddr, entry->chaddr, DISCOVERY_MSG_HADDRSIZ);
	strncpy((char *)&msg.hostname, entry->hostname,
	    DISCOVERY_MSG_HOSTNAMESIZ);

	ret = discovery_send(&msg);

	return ret;

}

int discovery_set_res_alive(reslist_t *entry)
{
	journal_ftrace(__func__);

	struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_HOST_ALIVE;
	memcpy(msg.haddr, entry->chaddr, DISCOVERY_MSG_HADDRSIZ);

	ret = discovery_send(&msg);

	return ret;
}

int discovery_set_res_dead(reslist_t *entry)
{
	journal_ftrace(__func__);

	struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_HOST_DEAD;
	memcpy(msg.haddr, entry->chaddr, DISCOVERY_MSG_HADDRSIZ);

	ret = discovery_send(&msg);

	return ret;
}

static void discovery_fini()
{
	journal_ftrace(__func__);

	free(discovery_x509);
	free(discovery_addr);
}

int discovery_init(char *x509, char *addr, uint16_t port)
{
	journal_ftrace(__func__);

	discovery_x509 = strdup(x509);
	set_context(discovery_x509);

	discovery_addr = strdup(addr);
	discovery_port = port;

	event_register(EVENT_EXIT, "discovery_fini", discovery_fini, PRIO_AGNOSTIC);

	return 0;
}
