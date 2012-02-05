
/*
 * See COPYRIGHTS file.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <xia/xiap.h>

#include "../lib/netbus.h"
#include "../lib/event.h"
#include "../lib/journal.h"
#include "../lib/utils.h"
#include "../lib/xiap.h"

#include "muxia.h"

void *MBUF;

static struct muxia_info *(*muxia_demux[XIAMSG_MAX])(void *) = {0};
void muxia_register(struct muxia_info *(*cb)(void *), uint8_t msgtype)
{
	journal_ftrace(__func__);

	if (muxia_demux[msgtype] != NULL)
		journal_notice("%s :: %s:%i\n",
			"XXX - It is now time to list chain the bucket");

	muxia_demux[msgtype] = cb;
}

/* XXX - muxia_exec() and callbacks return a local static variable
 * This will need some locking in the future; or a new approach.
 */

struct muxia_info *muxia_exec(const uint8_t msgtype, void *msgp)
{
	journal_ftrace(__func__);

	static struct muxia_info mux;
	memset(&mux, 0, sizeof(struct muxia_info));
	
	if (muxia_demux[msgtype] == NULL) {
		journal_notice("subsystem %i %s :: %s:%i\n", msgtype,
			"is not registered", __FILE__, __LINE__);

		mux.ret = XIAP_STATUS_NOLISTEN;
		return &mux;
	}

	/* XXX - iterate over a list to exec listening subsystems
	 * If the exception of muxia_register() occurs, we'll
	 * need to support multiple messages in a single xiap packet
	 */

	return muxia_demux[msgtype](msgp);
}

void muxia_reply(peer_t *peer, struct muxia_info *mux)
{
	journal_ftrace(__func__);

	struct xiap *xp = MBUF;
	void *msgp = MBUF + sizeof(struct xiap);

	struct xiap x;
	static char reply_mbuf[SLABSIZ];
	int len;
	
	x.type = xp->type;
	x.len = xp->len;
	x.version = XIAP_VERSION;

	len = ntohs(x.len);

	if (mux == NULL) {
		journal_notice("%s :: %s%i\n",
			"mux reply was NULL ?",
			__FILE__, __LINE__);

		/* be polite and return the failing message */
		x.status = XIAP_STATUS_UNEXPECTED;
	}

	switch (mux->ret) {

		case XIAP_STATUS_NOLISTEN:
			x.status = mux->ret;
			break;

		default:
			x.status = mux->ret;
			break;
	}

	memset(reply_mbuf, 0, SLABSIZ);
	memcpy(reply_mbuf + sizeof(struct xiap), msgp, len);
	memcpy(reply_mbuf, &x, sizeof(struct xiap));

	peer->send(peer, reply_mbuf, SLABSIZ);
}

void muxia(peer_t *peer)
{
	journal_ftrace(__func__);

	struct xiap *xp = MBUF;
	struct muxia_info *mux;
	int ret;
	
	/* recv() returns the number of bytes received,
	 * or -1 if an error occurred.
	 *
	 * If the peer has performed an orderly shutdown,
	 * a value of 0 will be returned.
	 */
	// XXX should use peer->recv()
	ret = recv(peer->sck, MBUF, SLABSIZ, MSG_NOSIGNAL);

	switch (ret) {
		case -1:
			journal_notice("recv() %s :: %s:%i\n",
				strerror(errno), __FILE__, __LINE__);
			peer->disconnect(peer);
			return;

		case 0:
			journal_notice("%s :: %s:%i\n",
				"recv() the peer has performed "
				"an orderly shutdown.",
				__FILE__, __LINE__);
			peer->disconnect(peer);
			return;

		default:
			break;
	}

	if (ret < sizeof(struct xiap)) {
		journal_notice("%s :: %s:%i\n",
			"abnormaly small message", __FILE__, __LINE__);

		return;
	}

	if (xiap_sanitize(xp))
		return;

	xiap_print(xp);

	mux = muxia_exec(xp->type, MBUF + sizeof(struct xiap));
	muxia_reply(peer, mux);
}


void muxia_fini(void *z)
{
	journal_ftrace(__func__);

	free(MBUF);
	MBUF = NULL;
}

int muxia_init(char *in_addr, int port)
{
	journal_ftrace(__func__);
	
	int ret;

	/* 
	 * s_tk: current token
	 * a_tk: begin a token
	 * z_tk: end a token
	 */
	char *s_tk, *z_tk, *a_tk = in_addr;

	MBUF = calloc(1, SLABSIZ);
	if (MBUF == NULL) {
		journal_notice("%s :: %s:%i\n",
			"calloc failed", __FILE__, __LINE__);

		return 1;
	}

	event_register(EVENT_EXIT, "muxia:muxia_fini()", muxia_fini, PRIO_AGNOSTIC);

	while ((s_tk = x_strtok(&a_tk, &z_tk, ','))) {

		ret = netbus_newserv(trim(s_tk), port,  NULL, NULL, muxia);
		if (ret < 0) {
			journal_notice("netbus_newserv failed :: %s:%i\n", __FILE__, __LINE__);
			return -1;
		}
	}
	
	return 0;
}
