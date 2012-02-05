#ifndef _NETBUS_H_
#define _NETBUS_H_

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

typedef struct peer {

	int type;
	int sck;
	struct in_addr dst;

	void (*on_add)();
	void (*on_del)();
	void (*on_pollin)();
	void (*on_pingreply)(struct peer *);

	int (*ping)();
	int (*send)();
	int (*recv)();
	void (*disconnect)();

	void *buf;
	void *udata;

} peer_t;

extern peer_t *netbus_newping(const char *, void (*)(peer_t *), void *);
extern int netbus_newserv(const char *, const int, void (*)(), void (*)(), void (*)()); 
extern peer_t *netbus_newclient(const char *, const int, void (*)(peer_t*), void (*)(peer_t*));
extern int netbus_init();

#endif // _NETBUS_H_
