/*
 * See COPYRIGHTS file.
 */


#ifndef XIA_NETBUS_H
#define XIA_NETBUS_H

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>


typedef struct {

	uint8_t *fragment;
	uint32_t head_len;
	uint32_t tail_len;

} frag_buffer_t;

typedef struct {
	
	uint8_t *data_buffer;
	uint32_t data_buffer_len;
	uint32_t proto_header_len;

} generic_buffer_t;

typedef uint32_t (frame_get_len_fct)(uint8_t *);
typedef uint32_t (frame_queue_fct)(void *, uint8_t *, int);

int netbus_reassembly(generic_buffer_t *,
			frag_buffer_t *,
			frame_get_len_fct,
			frame_queue_fct,
			void *);

typedef struct peer {

	int type;
	int socket;
	struct in_addr dst;

	void (*on_add)();
	void (*on_del)();
	void (*on_pollin)();
	void (*on_pingreply)(struct peer *);

	int (*ping)();
	int (*send)();
	int (*recv)();
	void (*disconnect)();

	void *data;
	void *connection_data;

} peer_t;

extern void poke_queue(void *);
extern peer_t *netbus_newping(const char *, void (*)(peer_t *), void *);
extern peer_t *netbus_newclient(const char *, const int, void (*)(peer_t*), void (*)(peer_t*));
extern int netbus_newserv(const char *, const int, void (*)(), void (*)(), void (*)()); 

typedef struct iface {
	
	char devname[16];
	int fd;

	void (*on_pollin)();

	int (*write)(struct iface *, void *, int);
	int (*read)(struct iface *);
	void (*shutdown)();

	void *frame;
	void *interface_data;

} iface_t;

extern iface_t *netbus_newtun(void (*)(iface_t *));

#define NETBUS_PEER 0x1
#define NETBUS_IFACE 0x4
struct netbus_sys {

	int type;

	peer_t *peer;
	iface_t *iface;
};

extern int netbus_init();

#endif /* XIA_NETBUS_H */
