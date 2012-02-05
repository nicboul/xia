/*
 * See COPYRIGHTS file.
 */


#ifndef IPPOOL_H
#define IPPOOL_H

#include <netinet/in.h>

typedef struct ippool {

	size_t sz;

	struct in_addr begin;
	struct in_addr end;

	uint8_t *pool;

} ippool_t;

extern char *ippool_get_ip(ippool_t *);
extern void ippool_release_ip(ippool_t *, char *);
extern ippool_t *ippool_new(char *, char *);

#endif /* IPPOOL_H */
