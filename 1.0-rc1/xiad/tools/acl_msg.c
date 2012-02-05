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
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <xia/xiap.h>
#include <xia/acl_msg.h>

#include "../../lib/netbus.h"
#include "../../lib/xiap.h"

#define CERT_NAME	"nib@1"

struct client {
	peer_t *peer;
} cl;

int main(int argc, char *argv[])
{
	struct acl_msg aclm;
	struct xiap x;

	char mbuf[SLABSIZ];
	int ret;

	char addr[] = "127.0.0.1";
	
	cl.peer = netbus_newclient(addr, 9090, NULL, NULL);

	memset(&aclm, 0, sizeof(struct acl_msg));

	aclm.version = ACL_MSG_VERSION;
	aclm.type = ACL_MSG_ADD;
	aclm.addr.s_addr = inet_addr("1.1.1.1");
	strncpy(aclm.name, CERT_NAME, strlen(CERT_NAME));

	x.type = XIAMSG_ACL;
	x.len = htons(sizeof(struct acl_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

	memmove(mbuf + sizeof(struct xiap), &aclm, sizeof(struct acl_msg));
	memmove(mbuf, &x, sizeof(struct xiap));
	
	cl.peer->send(cl.peer, mbuf, SLABSIZ);
	ret = cl.peer->recv(cl.peer);
	printf("recv value : %d\n", ret);
	
	printf("\nANSWER\n");
	xiap_print((struct xiap *)cl.peer->buf);

	cl.peer->disconnect(cl.peer);

	return 0;
}
