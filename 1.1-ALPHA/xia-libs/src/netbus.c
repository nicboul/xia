/*
 * See COPYRIGHTS file.
 */


#define __USE_BSD
#define __favor_BSD

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "ion.h"
#include "journal.h"
#include "netbus.h"
#include "sched.h"
#include "tun.h"
#include "xtp.h"

#define CONN_BACKLOG 512
#define INVALID_Q -1

int netbus_queue = INVALID_Q;

#define NETBUS_TCP_SERVER	0x1
#define NETBUS_TCP_CLIENT	0x2
#define NETBUS_ICMP_PING	0x3
#define NETBUS_UDP_SERVER	0x4
#define NETBUS_UDP_CLIENT	0x5

static int setnonblocking(int socket)
{
	journal_ftrace(__func__);

	int ret;
	ret = fcntl(socket, F_GETFL);
	if (ret >= 0) {
		ret = fcntl(socket, F_SETFL, ret | O_NONBLOCK);
	}
	return ret;
}

static int setreuse(int socket)
{
	journal_ftrace(__func__);

	int ret, on = 1;
	ret = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	return ret;
}

/* Directly stolen from OpenBSD{ping.c} */
static unsigned short chksum(u_short *addr, int len)
{
	journal_ftrace(__func__);

        int nleft = len;
        u_short *w = addr;
        int sum = 0;
        u_short answer = 0;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}

static int netbus_ping(peer_t *peer)
{
	journal_ftrace(__func__);

	int ret;
	size_t len;

	struct sockaddr_in dst_addr;
	char icmp_packet[sizeof(struct ip) + sizeof(struct icmp)];

	memset(icmp_packet, 0, sizeof(struct ip) + sizeof(struct icmp));
	memset(&dst_addr, 0, sizeof(struct sockaddr_in));

	struct ip *iphdr = (struct ip *)icmp_packet;
	struct icmp *icmphdr = (struct icmp *)(icmp_packet + sizeof(struct ip));

	/* build IP header */
	iphdr->ip_v = IPVERSION;
	iphdr->ip_hl = sizeof(struct ip) >> 2;
	iphdr->ip_tos = 0; /* 0 means kernel set appropriate value */
	iphdr->ip_id = 0;
	iphdr->ip_len = htons(sizeof(struct ip) + sizeof(struct icmp));
	iphdr->ip_ttl = 2;
	iphdr->ip_p = IPPROTO_ICMP;
	iphdr->ip_sum = 0;
	iphdr->ip_src.s_addr = INADDR_ANY;
	iphdr->ip_dst.s_addr = peer->dst.s_addr;

	/* build ICMP header */
	icmphdr->icmp_type = ICMP_ECHO;
	icmphdr->icmp_code = 0;
	icmphdr->icmp_cksum = 0;

	/* We use the socket descriptor id/process id as a seq/id number.
	 * cheapest way to track icmp-reply.
	 */
	icmphdr->icmp_seq = getpid();
	icmphdr->icmp_id = peer->socket;

	icmphdr->icmp_cksum = chksum((unsigned short *)icmphdr, sizeof(struct icmp));
	iphdr->ip_sum = chksum((unsigned short *)iphdr, sizeof(struct ip));

	dst_addr.sin_addr.s_addr = peer->dst.s_addr;
	len = sizeof(struct ip) + sizeof(struct icmp);
	ret = sendto(peer->socket, icmp_packet, len, 0, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr));
	if (ret < 0) {
		journal_notice("netbus]> netbus_ping() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

static int netbus_write(iface_t *iface, void *frame, int sz)
{
	journal_ftrace(__func__);

	int ret;
	ret = write(iface->fd, frame, sz);
	
	return ret;
}

static int netbus_read(iface_t *iface)
{
#define IFACE_BUF_SZ 5000
	journal_ftrace(__func__);

	int ret = 0;

	if (iface->frame == NULL) {
		iface->frame = calloc(1, IFACE_BUF_SZ);
		if (iface->frame == NULL) {
			journal_notice("netbus]> netbus_read() calloc FAILED :: %s:%i\n", __FILE__, __LINE__);
			return -1;
		}
	}
	else
		memset(iface->frame, 0, IFACE_BUF_SZ);

	ret = read(iface->fd, iface->frame, IFACE_BUF_SZ);
	if (ret < 0) {
		journal_notice("netbus]> netbus_read() failed %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return -1;
	}
	
	return ret;
}

static int netbus_send(peer_t *peer, void *data, int len)
{
	journal_ftrace(__func__);	

	int ret = 0;
	int total = 0;
	int byteleft = len;

	errno = 0;
	while (total < len) {

		ret = send(peer->socket, (uint8_t*)data + total, byteleft, 0);
		if (errno != 0)
			journal_notice("netbus]> netbus_send errno [%i] :: %s:%i\n", __FILE__, __LINE__);
		
		if (ret == -1 && errno != 11)
			return -1;

		if (ret != -1) {
			total += ret;
			byteleft -= ret;
		}
	}
	return ret;
}

static int netbus_recv(peer_t *peer)
{
#define PEER_BUF_SZ 5000
	journal_ftrace(__func__);

	fd_set rfds;
	struct timeval tv;
	int ret = 0;

	FD_ZERO(&rfds);
	FD_SET(peer->socket, &rfds);

	/* Wait up to five seconds */
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	ret = select(peer->socket + 1, &rfds, NULL, NULL, &tv);
	if (ret == 0) { /* TIMEOUT !*/
		journal_notice("netbus]> netbus_recv() TIMEOUT peer->socket(%i) :: %s:%i\n", peer->socket, __FILE__, __LINE__);
		return -1;
	}

	if (peer->data == NULL) {
		peer->data = calloc(1, PEER_BUF_SZ);
		if (peer->data == NULL) {
			journal_notice("netbus]> netbus_recv() calloc FAILED :: %s:%i\n", __FILE__, __LINE__);
			return -1;
		}
	}
	else
		memset(peer->data, 0, PEER_BUF_SZ);

	/* 		
	 * XXX It may happen that the buffer is too short,
	 * in that case we should then re-alloc and move
	 * the bytes.
	 */
	ret = recv(peer->socket, peer->data, PEER_BUF_SZ, 0);
	if (ret < 0)
		return -1;

	return ret;
}


static void netbus_shutdown(iface_t *iface)
{
	journal_ftrace(__func__);

	int ret;
	ret = close(iface->fd);
	if (ret < 0) {
		journal_notice("netbus]> iface closed: %u %u %s :: %s:%i\n", iface->fd, ret, strerror(errno), __FILE__, __LINE__);
		return;
	}

	free(iface->frame);
	free(iface);
}

static void netbus_disconnect(peer_t *peer)
{
	journal_ftrace(__func__);
	int ret;

	if (peer->on_del)
		peer->on_del(peer);

	/* 
	 * close() will cause the socket to be automatically removed from the queue
         */
	ret = close(peer->socket);
	if (ret < 0) {
		journal_notice("netbus]> close() failed: %u %u %s :: %s:%i\n", peer->socket, ret, strerror(errno), __FILE__, __LINE__);
		return;
	}

	journal_notice("netbus]> client close: %u\n", peer->socket);

	free(peer->data);
	free(peer);
}
static void pollin_frame(iface_t *iface)
{
	journal_ftrace(__func__);

	if (iface->on_pollin)
		iface->on_pollin(iface);
}

static void pollin_data(peer_t *peer)
{
	journal_ftrace(__func__);

	if (peer->on_pollin)
		peer->on_pollin(peer);
}

static void catch_pingreply(peer_t *peer)
{
	journal_ftrace(__func__);

	int ret;
	int addrlen;
	struct ip *ip_reply;
	struct icmp *icmp_reply;

	struct sockaddr_in saddr;

	fd_set rfds;
	struct timeval tv;
	
	static char buffer[sizeof(struct ip) + sizeof(struct icmp)];
	memset(buffer, 0, sizeof(buffer));

	addrlen = sizeof(struct sockaddr_in);
	ret = recvfrom(peer->socket, buffer, sizeof(struct ip) + sizeof(struct icmp), 0, (struct sockaddr*)&saddr, &addrlen);

	ip_reply = (struct ip*)buffer;
	icmp_reply = (struct icmp*)(buffer + sizeof(struct ip));

	/* 
	 * does this echo-reply belongs to this socket ? 
	 */
	if (icmp_reply->icmp_id == peer->socket && 
		icmp_reply->icmp_seq == getpid() && 
		icmp_reply->icmp_type == 0) { // ECHO-REPLY

		if (peer->on_pingreply)
			peer->on_pingreply(peer);
		
		journal_notice("ping]> ID: %d\n", ntohs(ip_reply->ip_id));
		journal_notice("ping]> TTL: %d\n", ip_reply->ip_ttl);
		journal_notice("ping]> Received %d byte reply from %s:\n",
			sizeof(buffer), inet_ntoa(ip_reply->ip_src));

		journal_notice("ping]> type: %i\n", icmp_reply->icmp_type);
		journal_notice("ping]> code: %i\n", icmp_reply->icmp_code);
		journal_notice("ping]> id  : %i\n", icmp_reply->icmp_id);
		journal_notice("ping]> seq : %i\n", icmp_reply->icmp_seq);		
	}	
}

static void add_client(peer_t *peer)
{
	journal_ftrace(__func__);

	int ret, addrlen;
	struct sockaddr_in addr;
	struct netbus_sys *nsys;
	peer_t *npeer;


	nsys = calloc(sizeof(struct netbus_sys), 1);
	npeer = calloc(sizeof(peer_t), 1);

	if (!nsys || !npeer)
		return;

	addrlen = sizeof(struct sockaddr_in);        
	npeer->socket = accept(peer->socket, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
	if (npeer->socket < 0) {
		journal_notice("netbus]> accept() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		free(npeer);
		return;
	}

	journal_notice("netbus]> add_client{%i} srv{%i}\n", npeer->socket, peer->socket);

	ret = setnonblocking(npeer->socket);
        if (ret < 0) {
                journal_notice("netbus]> setnonblocking() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		free(npeer);
                return;
        }

	// XXX need an init func ?
	npeer->type = NETBUS_TCP_CLIENT;
	npeer->on_add = peer->on_add;
	npeer->on_del = peer->on_del;
	npeer->on_pollin = peer->on_pollin;
	npeer->recv = peer->recv;
	npeer->send = peer->send;
	npeer->disconnect = peer->disconnect;
	npeer->data = NULL;

	nsys->type = NETBUS_PEER;
	nsys->peer = npeer;
	ret = ion_add(netbus_queue, npeer->socket, nsys);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		free(npeer);
	}

	if (peer->on_add)
		peer->on_add(npeer);
}

// Handle IO Events
static void netbus_hioe(void *udata, int flag)
{
	journal_ftrace(__func__);

	struct netbus_sys *nsys;
	nsys = udata;

	if (nsys->type == NETBUS_PEER) {

		if (flag == ION_EROR) {
			netbus_disconnect(nsys->peer);
		}
		else if (flag == ION_READ) {
			if (nsys->peer->type == NETBUS_TCP_SERVER) {
				add_client(nsys->peer);
			}
			else if (nsys->peer->type == NETBUS_TCP_CLIENT) {
				pollin_data(nsys->peer);
			}
			else if (nsys->peer->type = NETBUS_ICMP_PING) {
				catch_pingreply(nsys->peer);
			}
		}
	}

	else if (nsys->type == NETBUS_IFACE) {

		if (flag == ION_EROR) {
			netbus_shutdown(nsys->iface);
		}
		else if (flag == ION_READ) {
			pollin_frame(nsys->iface);
		}
	}
}

void poke_queue(void *udata)
{
	journal_ftrace(__func__);

	int ret;

	ret = ion_poke(netbus_queue, netbus_hioe);
	if (ret < 0) {
		journal_notice("netbus]> ion_poke() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return;
	}
}

int netbus_reassembly(generic_buffer_t *gb,
			frag_buffer_t *fb,
			frame_get_len_fct frame_get_len,
			frame_queue_fct frame_queue,
			void *opaque_ptr)
{
	uint32_t offset = gb->data_buffer_len;
	uint16_t len = 0;
	uint32_t ret = 0;

	//printf("1fragment %p\n", fb->fragment);
	//printf("2head_len %i\n", fb->head_len);
	//printf("3tail_len %i\n", fb->tail_len);

	//printf("4data_buffer %p\n", gb->data_buffer);
	//printf("5d_b_len %i\n", gb->data_buffer_len);
	//printf("6header_len %i\n", gb->proto_header_len);

	//printf("7offset [%i] len [%i]\n", offset, len);

	if (fb->head_len < gb->proto_header_len && fb->head_len != 0) {
		if (offset < gb->proto_header_len - fb->head_len) {
			memmove(((uint8_t*)fb->fragment + fb->head_len),
			((uint8_t*)gb->data_buffer), offset);

			fb->tail_len -= offset;
			fb->head_len += offset;

			offset = 0;
		}
		else if (offset >= gb->proto_header_len - fb->head_len) {
		
	//	printf("8frag head len %i\n", fb->head_len);
	//	printf("9hdsize %i\n", gb->proto_header_len);

			memmove(((uint8_t*)fb->fragment + fb->head_len),
			((uint8_t*)gb->data_buffer), fb->tail_len);

			offset -= fb->tail_len;
			fb->head_len += fb->tail_len;

			fb->tail_len = frame_get_len(fb->fragment);

	//	printf("11head len %i\n", fb->head_len);
	//	printf("22tail len %i\n", fb->tail_len);	
		}

	//	printf("33offset [%i] len [%i]\n", offset, len);

	}

	if (fb->head_len >= gb->proto_header_len) {

	//	printf("42 buffer_len [%i] offset [%i]\n", gb->data_buffer_len, offset);

	//	printf("43 len [%i] tail_len [%i] \n", len, fb->tail_len);

		if (offset <= fb->tail_len) {
				memmove((uint8_t*)fb->fragment + fb->head_len,
					(uint8_t*)gb->data_buffer + (gb->data_buffer_len - offset), offset);

			fb->tail_len -= offset;
			fb->head_len += offset;

			offset = 0;

	//	printf("44nbyte [%i] offset [%i] len [%i]\n", gb->data_buffer_len, offset, len);
		}
		else if (offset > fb->tail_len) {
				memmove((uint8_t*)fb->fragment + fb->head_len,
					((uint8_t*)gb->data_buffer + (gb->data_buffer_len - offset)), fb->tail_len);

			offset -= fb->tail_len;
			fb->head_len += fb->tail_len;

			fb->tail_len = 0;

	//		printf("44nbyte [%i] offset [%i] len [%i] tail_len[%i]\n", gb->data_buffer_len, offset, len, fb->tail_len);
		}

		if (fb->tail_len == 0) {
			len = frame_get_len(fb->fragment);
	//		printf("45len [%i] head_len [%i]\n", len, fb->head_len);
			frame_queue(opaque_ptr, fb->fragment, fb->head_len);
			fb->head_len = fb->tail_len = 0;
		}
	}

	while (offset > 0) {
		if (offset < gb->proto_header_len) {
			len = 0;
		}
		else {
			len = frame_get_len(((uint8_t*)gb->data_buffer + (gb->data_buffer_len - offset)));

	//		printf("55nbyte [%i] offset [%i] len [%i]\n", gb->data_buffer_len, offset, len);
		}	
	
		if (offset < gb->proto_header_len) {

			memmove(fb->fragment, ((uint8_t *)gb->data_buffer + (gb->data_buffer_len - offset)), offset);
			fb->head_len = offset;
			fb->tail_len = gb->proto_header_len - offset;

			offset = 0;
			break;
		}
		else if (len + gb->proto_header_len > offset) {

	//		printf("56len+head_len[%i] offset[%i]\n", len + gb->proto_header_len, offset);
			
			memmove(fb->fragment, ((uint8_t *)gb->data_buffer + (gb->data_buffer_len - offset)), offset);
	//		printf("57move\n");
			fb->head_len = offset;
			fb->tail_len = len - (offset - gb->proto_header_len);

			offset = 0;
	//		printf("57break\n");
			break;
		}

	//	printf("66nbyte [%i] offset [%i] len [%i]\n", gb->data_buffer_len, offset, len);
//		xmp_print((uint8_t*)gb->data_buffer + (gb->data_buffer_len - offset));
		frame_queue(opaque_ptr, (uint8_t *)gb->data_buffer + (gb->data_buffer_len - offset), len + gb->proto_header_len);
		offset -= gb->proto_header_len + len;
	}
}

iface_t *netbus_newtun(void (*on_pollin)(iface_t *))
{
	journal_ftrace(__func__);

	int ret;

	struct netbus_sys *nsys;
	iface_t *iface;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus]> subsystem unitialized :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	nsys = calloc(sizeof(struct netbus_sys), 1);
	iface = calloc(sizeof(iface_t), 1);

	if (!nsys || !iface)
		return NULL;

	ret = tun_create(&(iface->devname), &(iface->fd));
        if (ret < 0) {
                journal_notice("netbus]> tun_create failed %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
                free(iface);
                free(nsys);
                return NULL;
        }

	ret = setnonblocking(iface->fd);
	if (ret < 0) {
                journal_notice("netbus]> setnonblocking() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		tun_destroy(iface);
                close(iface->fd);
                free(iface);
                free(nsys);
                return NULL;
        }
	
        iface->on_pollin = on_pollin;
        iface->write = netbus_write;
        iface->read = netbus_read;
        iface->shutdown = netbus_shutdown;
	iface->frame = NULL;

        nsys->type = NETBUS_IFACE;
        nsys->iface = iface;
        ret = ion_add(netbus_queue, iface->fd, nsys);
        if (ret < 0) {

                journal_notice("ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		tun_destroy(iface);
                close(iface->fd);
                free(iface);
                free(nsys);
		return NULL;
        }

        return iface;
}

peer_t *netbus_udp_client(const char *addr,
				const int port,
				void (*on_del)(peer_t*),
				void (*on_pollin)(peer_t*))
{



}

peer_t *netbus_newclient(const char *addr,
			  const int port,
			  void (*on_del)(peer_t*),
			  void (*on_pollin)(peer_t*))
{
	journal_ftrace(__func__);

	fd_set wfds;
	struct timeval tv;
	int ret;
	int optval, optlen;
	struct sockaddr_in addr_in;
	struct netbus_sys *nsys;
	peer_t *peer;

	FD_ZERO(&wfds);
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus]> subsystem unitialized :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	nsys = calloc(sizeof(struct netbus_sys), 1);
	peer = calloc(sizeof(peer_t), 1);

	if (!nsys || !peer)
		return NULL;

	peer->socket = socket(PF_INET, SOCK_STREAM, 0);
	if (peer->socket == -1) {
		journal_notice("netbus]> socket() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return NULL;
	}

        ret = setnonblocking(peer->socket);
        if (ret < 0) {
                journal_notice("netbus]> setnonblocking() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
                free(peer);
                return NULL;
        }

	memset(&addr_in, 0, sizeof(struct sockaddr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(port);
	addr_in.sin_addr.s_addr = inet_addr(addr);

	FD_SET(peer->socket, &wfds);
	errno = 0;

	ret = connect(peer->socket, (const struct sockaddr *)&addr_in, sizeof(const struct sockaddr));
	if (ret == -1) {
		if (errno == EINPROGRESS) {
			/* The socket is non-blocking and the
			 * connection cannot be completed immediately.
			 */
			ret = select(peer->socket + 1, NULL, &wfds, NULL, &tv);
			if (ret == 0) { /* TIMEOUT */
				journal_notice("netbus]> connect() timed out :: %s:%i\n", __FILE__, __LINE__);
				close(peer->socket);
				free(peer);
				return NULL;
			}
			else {
				optlen = sizeof(optval);
				/* use getsockopt(2) with SO_ERROR to check for error conditions */
				ret = getsockopt(peer->socket, SOL_SOCKET, SO_ERROR, &optval, &optlen);
				if (ret == -1) {
					journal_notice("netbus]> getsockopt() %s :: %s:%i\n", __FILE__, __LINE__);
					close(peer->socket);
					free(peer);
					return NULL;
				}

				if (optval != 0) { /* NOT CONNECTED ! TIMEOUT... */
					journal_notice("netbus]> connect() timed out :: %s:%i\n", __FILE__, __LINE__);
					close(peer->socket);
					free(peer);
	                                return NULL;
				}
				else {
					/* ... connected, we continue ! */
				}
			}
			
		}
		else { 
			journal_notice("netbus]> connect() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
			close(peer->socket);
			free(peer);
			return NULL;
		}
	}

	peer->type = NETBUS_TCP_CLIENT;
	peer->on_del = on_del;
	peer->on_pollin = on_pollin;
	peer->send = netbus_send;
	peer->recv = netbus_recv;
	peer->disconnect = netbus_disconnect;
	peer->data = NULL;

	nsys->type = NETBUS_PEER;
	nsys->peer = peer;
	
	ret = ion_add(netbus_queue, peer->socket, nsys);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return NULL;
	}
	
	return peer;
}

peer_t *netbus_newping(const char *addr, void (*on_pingreply)(peer_t *), void *connection_data)
{
	journal_ftrace(__func__);
	
	int ret, optval;
	struct in_addr dst;
	struct netbus_sys *nsys;
	peer_t *peer;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus]> subsystem unitialized :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}
	
	nsys = calloc(sizeof(struct netbus_sys), 1);
	peer = calloc(sizeof(peer_t), 1);

	if (!nsys || !peer)
		return NULL;
	ret = inet_aton(addr, (struct in_addr *)&dst);

	peer->type = NETBUS_ICMP_PING;
	peer->ping = netbus_ping;
	peer->on_pingreply = on_pingreply;
	peer->dst = dst;
	peer->disconnect = netbus_disconnect;
	peer->connection_data = connection_data;
	peer->data = NULL;

	peer->socket = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (peer->socket < 0) {
		journal_notice("netbus]> socket() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);	
		free(peer);
		return NULL;

	}
	fcntl(peer->socket, F_SETOWN, (int)getpid());	
	ret = setsockopt(peer->socket, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int));
	if (ret < 0) {
		journal_notice("netbus]> setsockopt() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return NULL;
	}

	nsys->type = NETBUS_PEER;
	nsys->peer = peer;
	ret = ion_add(netbus_queue, peer->socket, nsys);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return NULL;
	}

	journal_notice("netbus]> new ping added\n");

	return peer;
}

int netbus_udp_server(const char *in_addr,
			const int port,
			void (*on_add)(peer_t*),
			void (*on_del)(peer_t*),
			void (*on_pollin)(peer_t*))
{
	journal_ftrace(__func__);

	int ret;
	struct sockaddr_in addr;
	struct netbus_sys *nsys;
	peer_t *peer;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus]> subsystem uninitialized :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	nsys = calloc(sizeof(struct netbus_sys), 1);
	peer = calloc(sizeof(peer_t), 1);

	if (!nsys || !peer)
		return -1;

	peer->type = NETBUS_UDP_SERVER;
	peer->on_add = on_add;
	peer->on_del = on_del;
	peer->on_pollin = on_pollin;
	
	peer->data = NULL;

	peer->socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (peer->socket < 0) {
		journal_notice("netbus]> socket() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}
	
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(in_addr);

	ret = setreuse(peer->socket);
	if (ret < 0) {
		journal_notice("netbus]> setreuse() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}	

	ret = bind(peer->socket, (const struct sockaddr *)&addr, sizeof(const struct sockaddr));
	if (ret < 0) {
		journal_notice("netbus]> bind() %s %s :: %s:%i\n", strerror(errno), in_addr, __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}

	nsys->type = NETBUS_PEER;
	nsys->peer = peer;
	ret = ion_add(netbus_queue, peer->socket, nsys);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}

	return 0;
}

int netbus_newserv(const char *in_addr,
		   const int port,
		   void (*on_add)(peer_t*),
		   void (*on_del)(peer_t*),
		   void (*on_pollin)(peer_t*))
{
	journal_ftrace(__func__);

	int ret;
	struct sockaddr_in addr;
	struct netbus_sys *nsys;
	peer_t *peer;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus]> subsystem unitialized :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	nsys = calloc(sizeof(struct netbus_sys), 1);
	peer = calloc(sizeof(peer_t), 1);

	if (!nsys || !peer)
		return -1;

	peer->type = NETBUS_TCP_SERVER;
	peer->on_add = on_add;
	peer->on_del = on_del;
	peer->on_pollin = on_pollin;
	peer->recv = netbus_recv;
	peer->send = netbus_send;
	peer->disconnect = netbus_disconnect;
	peer->data = NULL;

	peer->socket = socket(PF_INET, SOCK_STREAM, 0);
	if (peer->socket < 0) {
		journal_notice("netbus]> socket() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(in_addr);

	ret = setreuse(peer->socket);
	if (ret < 0) {
		journal_notice("netbus]> setreuse() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}

	ret = bind(peer->socket, (const struct sockaddr *)&addr, sizeof(const struct sockaddr));
	if (ret < 0) {
		journal_notice("netbus]> bind() %s %s :: %s:%i\n", strerror(errno), in_addr, __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}

	/* The backlog parameter defines the maximum length the
	 * queue of pending connection may grow to. LISTEN(2)
	 */
	ret = listen(peer->socket, CONN_BACKLOG);
	if (ret < 0) {
		journal_notice("netbus]> set_nonblocking() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}

	nsys->type = NETBUS_PEER;
	nsys->peer = peer;
	ret = ion_add(netbus_queue, peer->socket, nsys);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->socket);
		free(peer);
		return -1;
	}

	return 0;
}

int netbus_init() 
{
	journal_ftrace(__func__);

	/* Open an ion file descriptor */
	netbus_queue = ion_new();

	if (netbus_queue < 0) {
		journal_notice("netbus]> ion_new() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		netbus_queue = INVALID_Q;
		return -1;
	}

	sched_register(SCHED_APERIODIC, "poke_queue", poke_queue, 0, NULL);

	return 0;
}
