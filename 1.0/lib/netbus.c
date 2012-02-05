
/*
 * See COPYRIGHTS file.
 */

#define __USE_BSD
#define __favor_BSD

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

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

#include <xia/xiap.h>

#include "journal.h"
#include "ion.h"
#include "netbus.h"
#include "sched.h"

#define CONN_BACKLOG 512
#define INVALID_Q -1

int netbus_queue = INVALID_Q;

#define NETBUS_TCP_SERVER 0x1
#define NETBUS_TCP_CLIENT 0x2
#define NETBUS_ICMP_PING 0x3

static int setnonblocking(int sck)
{
	journal_ftrace(__func__);

	int ret;
	ret = fcntl(sck, F_GETFL);
	if (ret >= 0) {
		ret = fcntl(sck, F_SETFL, ret | O_NONBLOCK);
	}
	return ret;
}

static int setreuse(int sck)
{
	journal_ftrace(__func__);

	int ret, on = 1;
	ret = setsockopt(sck, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
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
	icmphdr->icmp_id = peer->sck;

	icmphdr->icmp_cksum = chksum((unsigned short *)icmphdr, sizeof(struct icmp));
	iphdr->ip_sum = chksum((unsigned short *)iphdr, sizeof(struct ip));

	dst_addr.sin_addr.s_addr = peer->dst.s_addr;
	len = sizeof(struct ip) + sizeof(struct icmp);
	ret = sendto(peer->sck, icmp_packet, len, 0, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr));
	if (ret < 0) {
		journal_notice("netbus]> netbus_ping() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

static int netbus_send(peer_t *peer, void *buf, int sz)
{
	journal_ftrace(__func__);	

	int ret = 0;
	ret = send(peer->sck, buf, sz, 0);
	return ret;
}

#define PEER_BUF_SZ SLABSIZ
static int netbus_recv(peer_t *peer)
{
	journal_ftrace(__func__);

	fd_set rfds;
	struct timeval tv;
	int ret = 0;

	FD_ZERO(&rfds);
	FD_SET(peer->sck, &rfds);

	/* Wait up to five seconds */
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	ret = select(peer->sck + 1, &rfds, NULL, NULL, &tv);
	if (ret == 0) { /* TIMEOUT !*/
		journal_notice("netbus]> netbus_recv() TIMEOUT peer->sck(%i) :: %s:%i\n", peer->sck, __FILE__, __LINE__);
		return -1;
	}

	if (peer->buf == NULL) {
		peer->buf = calloc(1, PEER_BUF_SZ);
		if (peer->buf == NULL) {
			journal_notice("netbus]> netbus_recv() calloc FAILED :: %s:%i\n", __FILE__, __LINE__);
			return -1;
		}
	}
	else
		memset(peer->buf, 0, PEER_BUF_SZ);

	/* 		
	 * XXX It may happen that the buffer is too short,
	 * in this case we should then re-alloc and move
	 * the bytes.
	 */
	ret = recv(peer->sck, peer->buf, PEER_BUF_SZ, 0);
	if (ret < 0)
		return -1;

	return ret;
}

static void netbus_disconnect(peer_t *peer)
{
	journal_ftrace(__func__);
	int ret;

	/* 
	 * close() will cause the socket to be automatically removed from the queue
         */
	ret = close(peer->sck);
	if (ret < 0) {
		journal_notice("netbus]> client closed: %u %u %s :: %s:%i\n", peer->sck, ret, strerror(errno), __FILE__, __LINE__);
		return;
	}
	free(peer->buf);
	free(peer);
}

static void pollin_payload(peer_t *peer)
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
	ret = recvfrom(peer->sck, buffer, sizeof(struct ip) + sizeof(struct icmp), 0, (struct sockaddr*)&saddr, &addrlen);

	ip_reply = (struct ip*)buffer;
	icmp_reply = (struct icmp*)(buffer + sizeof(struct ip));

	/* 
	 * does this echo-reply belongs to this socket ? 
	 */
	if (icmp_reply->icmp_id == peer->sck && 
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
	peer_t *npeer;

	journal_notice("netbus]> add_client %i\n", peer->sck);

	npeer = calloc(sizeof(peer_t), 1);	
        addrlen = sizeof(struct sockaddr_in);
        
        npeer->sck = accept(peer->sck, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (npeer->sck < 0) {
                journal_notice("netbus]> accept() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		free(npeer);
                return;
        }

        ret = setnonblocking(npeer->sck);
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
	npeer->buf = NULL;

	ret = ion_add(netbus_queue, npeer->sck, npeer);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		free(npeer);
	}

	if (peer->on_add)
		peer->on_add(peer);
}

// Handle IO Events
static void netbus_hioe(void *udata, int flag)
{
	journal_ftrace(__func__);

	peer_t *peer;
	peer = (peer_t *)udata;

	if (flag == ION_EROR) {
		netbus_disconnect(peer);
	}
	else if (flag == ION_READ) {
		if (peer->type == NETBUS_TCP_SERVER) {
			add_client(peer);
		}
		else if (peer->type == NETBUS_TCP_CLIENT) {
			pollin_payload(peer);
		}
		else if (peer->type = NETBUS_ICMP_PING) {
			catch_pingreply(peer);
		}
	}
}

static void poke_queue(void *udata)
{
	journal_ftrace(__func__);

	int ret;

	ret = ion_poke(netbus_queue, netbus_hioe);
	if (ret < 0) {
		journal_notice("netbus]> ion_poke() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return;
	}
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
	peer_t *peer;

	FD_ZERO(&wfds);
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus]> subsystem unitialized :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	peer = calloc(sizeof(peer_t), 1);
	peer->sck = socket(PF_INET, SOCK_STREAM, 0);
	if (peer->sck == -1) {
		journal_notice("netbus]> socket() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return NULL;
	}

        ret = setnonblocking(peer->sck);
        if (ret < 0) {
                journal_notice("netbus]> setnonblocking() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
                free(peer);
                return NULL;
        }

	memset(&addr_in, 0, sizeof(struct sockaddr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(port);
	addr_in.sin_addr.s_addr = inet_addr(addr);

	FD_SET(peer->sck, &wfds);
	errno = 0;

	ret = connect(peer->sck, (const struct sockaddr *)&addr_in, sizeof(const struct sockaddr));
	if (ret == -1) {
		if (errno == EINPROGRESS) {
			/* The socket is non-blocking and the
			 * connection cannot be completed immediately.
			 */
			ret = select(peer->sck + 1, NULL, &wfds, NULL, &tv);
			if (ret == 0) { /* TIMEOUT */
				journal_notice("netbus]> connect() timed out :: %s:%i\n", __FILE__, __LINE__);
				close(peer->sck);
				free(peer);
				return NULL;
			}
			else {
				optlen = sizeof(optval);
				/* use getsockopt(2) with SO_ERROR to check for error conditions */
				ret = getsockopt(peer->sck, SOL_SOCKET, SO_ERROR, &optval, &optlen);
				if (ret == -1) {
					journal_notice("netbus]> getsockopt() %s :: %s:%i\n", __FILE__, __LINE__);
					close(peer->sck);
					free(peer);
					return NULL;
				}

				if (optval != 0) { /* NOT CONNECTED ! TIMEOUT... */
					journal_notice("netbus]> connect() timed out :: %s:%i\n", __FILE__, __LINE__);
					close(peer->sck);
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
			close(peer->sck);
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
	peer->buf = NULL;
	
	ret = ion_add(netbus_queue, peer->sck, peer);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return NULL;
	}
	
	return peer;
}

peer_t *netbus_newping(const char *addr, void (*on_pingreply)(peer_t *), void *udata)
{
	journal_ftrace(__func__);
	
	int ret, optval;
	struct in_addr dst;
	peer_t *peer;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus]> subsystem unitialized :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	peer = calloc(sizeof(peer_t), 1);
	ret = inet_aton(addr, (struct in_addr *)&dst);

	peer->type = NETBUS_ICMP_PING;
	peer->ping = netbus_ping;
	peer->on_pingreply = on_pingreply;
	peer->dst = dst;
	peer->disconnect = netbus_disconnect;
	peer->udata = udata;
	peer->buf = NULL;

	peer->sck = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (peer->sck < 0) {
		journal_notice("netbus]> socket() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);	
		free(peer);
		return NULL;

	}
	fcntl(peer->sck, F_SETOWN, (int)getpid());	
	ret = setsockopt(peer->sck, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int));
	if (ret < 0) {
		journal_notice("netbus]> setsockopt() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return NULL;
	}

	ret = ion_add(netbus_queue, peer->sck, peer);
	if (ret < 0) {
		journal_notice("netbus]> ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return NULL;
	}

	journal_notice("netbus]> new ping added\n");

	return peer;
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
	peer_t *peer;

	if (netbus_queue == INVALID_Q) {
		journal_notice("netbus subsystem unitialized :: %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	peer = calloc(sizeof(peer_t), 1);

	if (!peer)
		return -1;

	peer->type = NETBUS_TCP_SERVER;
	peer->on_add = on_add;
	peer->on_del = on_del;
	peer->on_pollin = on_pollin;
	peer->recv = netbus_recv;
	peer->send = netbus_send;
	peer->disconnect = netbus_disconnect;
	peer->buf = NULL;

	peer->sck = socket(PF_INET, SOCK_STREAM, 0);
	if (peer->sck < 0) {
		journal_notice("socket() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(in_addr);

	ret = setreuse(peer->sck);
	if (ret < 0) {
		journal_notice("setreuse() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return -1;
	}

	ret = bind(peer->sck, (const struct sockaddr *)&addr, sizeof(const struct sockaddr));
	if (ret < 0) {
		journal_notice("bind() %s %s:: %s:%i\n", strerror(errno), in_addr, __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return -1;
	}

	/* The backlog parameter defines the maximum length the
	 * queue of pending connection may grow to. LISTEN(2)
	 */
	ret = listen(peer->sck, CONN_BACKLOG);
	if (ret < 0) {
		journal_notice("set_nonblocking() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
		free(peer);
		return -1;
	}

	ret = ion_add(netbus_queue, peer->sck, peer);
	if (ret < 0) {
		journal_notice("ion_add() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		close(peer->sck);
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
		journal_notice("ion_new() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		netbus_queue = INVALID_Q;
		return -1;
	}

	sched_register(SCHED_APERIODIC, "poke_queue", poke_queue, 0, NULL);

	return 0;
}
