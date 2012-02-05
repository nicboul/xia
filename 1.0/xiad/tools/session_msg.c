/*
 * Copyright 2008 Mind4Networks Technologies INC.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <xia/xiap.h>
#include <xia/session_msg.h>

#include "../../lib/netbus.h"
#include "../../lib/xiap.h"

#define CERT_NAME	"nib/1"

struct client {
	peer_t *peer;
} cl;

int main(int argc, char *argv[])
{
	struct session_msg msg, *sm_reply;
	struct xiap x;

	char mbuf[SLABSIZ];
	int ret;

	char addr[] = "127.0.0.1";

	netbus_init();

	cl.peer = netbus_newclient(addr, 9090, NULL, NULL);

	memset(&msg, 0, sizeof(msg));

	msg.version = SESSION_MSG_VERSION;
	msg.type = SESSION_MSG_UP;
	msg.addr.s_addr = inet_addr("1.1.1.1");
	strncpy(msg.name, CERT_NAME, strlen(CERT_NAME));

	x.type = XIAMSG_ACL;
	x.len = htons(sizeof(struct session_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

	memmove(mbuf + sizeof(struct xiap), &msg, sizeof(struct session_msg));
	memmove(mbuf, &x, sizeof(struct xiap));

	cl.peer->send(cl.peer, mbuf, SLABSIZ);
	ret = cl.peer->recv(cl.peer);
	printf("recv value : %d\n", ret);

	if (ret > 0) {
		printf("\nANSWER\n");
		xiap_print((struct xiap *)&mbuf);
		sm_reply = (struct session_msg *)(mbuf + sizeof(struct xiap));
		printf("SESSION ANSWER : %i\n", sm_reply->type);
	}

	cl.peer->disconnect(cl.peer);

	return 0;
}
