
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
#include <xia/discovery_msg.h>

#include "../../lib/netbus.h"
#include "../../lib/xiap.h"

#define CERT_NAME	"000024caa468@2"

struct client {
	peer_t *peer;
} cl;

int main(int argc, char *argv[])
{
	struct discovery_msg msg;
	struct xiap x;

	char mbuf[SLABSIZ];
	int ret;

	char addr[] = "127.0.0.1";

	netbus_init();

	cl.peer = netbus_newclient(addr, 9090, NULL, NULL);

	memset(&msg, 0, sizeof(msg));

	msg.version = DISCOVERY_MSG_VERSION;
	msg.type = DISCOVERY_MSG_SET_RES_HOSTNAME;
	msg.addr.s_addr = inet_addr("1.1.1.1");
	//strncpy(msg.hostname, "just-a-tests", strlen("just-a-tests"));
	strncpy(msg.name, CERT_NAME, strlen(CERT_NAME));

	x.type = XIAMSG_DISCOVERY;
	x.len = htons(sizeof(struct discovery_msg));
	x.status = XIAP_STATUS_EXEC;
	x.version = XIAP_VERSION;

	memmove(mbuf + sizeof(struct xiap), &msg, sizeof(struct discovery_msg));
	memmove(mbuf, &x, sizeof(struct xiap));

	cl.peer->send(cl.peer, mbuf, SLABSIZ);
	ret = cl.peer->recv(cl.peer);
	printf("recv value : %d\n", ret);
	
	printf("\nANSWER\n");
	xiap_print((struct xiap *)&mbuf);
	
	cl.peer->disconnect(cl.peer);

	return 0;
}
