/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#include <xia/xiap.h>
#include <xia/discovery_msg.h>

#include "../lib/journal.h"
#include "../lib/netbus.h"

#include "reslist.h"

extern char *xiad_addr, *x509cn;
extern uint16_t *xiad_port;

int discovery_send(struct discovery_msg *msg)
{
	journal_strace("discovery_send");

	struct xiap x, *x_reply;

	peer_t *peer;
	char mbuf[SLABSIZ];

	memset(&x, 0, sizeof(struct xiap));

	peer = netbus_newclient(xiad_addr, *xiad_port, NULL, NULL);
	if (peer == NULL)
		return 1;

	msg->version = DISCOVERY_MSG_VERSION;
	strncpy((char *)msg->name, x509cn, XIAP_NAMESIZ);

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
	journal_strace("discovery_set_res_addr");

	static struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_ADDR;
	msg.addr.s_addr = entry->addr.s_addr;
	sprintf(msg.haddr, "%02x%02x%02x%02x%02x%02x",
	    entry->chaddr[0], entry->chaddr[1], entry->chaddr[2],
	    entry->chaddr[3], entry->chaddr[4], entry->chaddr[5]);

	ret = discovery_send(&msg);

	return ret;

}

int discovery_set_res_hostname(reslist_t *entry)
{
	journal_strace("discovery_set_res_hostname");

	struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_HOSTNAME;
	strncpy((char *)&msg.hostname, entry->hostname,
	    DISCOVERY_MSG_HOSTNAMESIZ);

	sprintf(msg.haddr, "%02x%02x%02x%02x%02x%02x",
	    entry->chaddr[0], entry->chaddr[1], entry->chaddr[2],
	    entry->chaddr[3], entry->chaddr[4], entry->chaddr[5]);

	ret = discovery_send(&msg);

	return ret;

}

int discovery_set_res_alive(reslist_t *entry)
{
	journal_strace("discovery_set_res_alive");

	struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_HOST_ALIVE;
	sprintf(msg.haddr, "%02x%02x%02x%02x%02x%02x",
	    entry->chaddr[0], entry->chaddr[1], entry->chaddr[2],
	    entry->chaddr[3], entry->chaddr[4], entry->chaddr[5]);

	ret = discovery_send(&msg);

	return ret;

}

int discovery_set_res_dead(reslist_t *entry)
{
	journal_strace("discovery_set_res_dead");

	struct discovery_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.type = DISCOVERY_MSG_SET_RES_HOST_DEAD;
	sprintf(msg.haddr, "%02x%02x%02x%02x%02x%02x",
	    entry->chaddr[0], entry->chaddr[1], entry->chaddr[2],
	    entry->chaddr[3], entry->chaddr[4], entry->chaddr[5]);

	ret = discovery_send(&msg);

	return ret;
}
