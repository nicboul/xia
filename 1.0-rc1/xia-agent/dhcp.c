/*
 * Copyright (c) 2008; Samuel Jean <peejix@gmail.com>
 * Copyright (c) 2008; Mind4Networks Technologies INC.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../lib/journal.h"

#include "dhcp.h"

/*
 * dhcp_get_option(): parse option field to find value of a code
 * `start` is a pointer to the start of dhcp options payload
 * `end` is a pointer to the end of available payload
 * `code` is the dhcp code number to lookup for
 * returns a (struct dhcp_opt_hdr *) or NULL if code was not found.
 */
struct dhcp_opt_hdr *dhcp_get_option(uint8_t *start,
				     const uint8_t *end,
				     uint8_t code)
{
	journal_strace(__func__);

	struct dhcp_opt_hdr *opt;
	uint8_t *p = start;

	if ((end - start) < sizeof(struct dhcp_opt_hdr))
		return NULL;

	while (p < end)
		switch (*p) {
			case DHCP_OPT_PAD:
				++p;
				continue;

			case DHCP_OPT_END:
				return NULL;

			default:
				if (*p != code) {
					/* go to the next code */
					p++;
					p += *p + 1;
					continue;
				}

				if ((p + sizeof(struct dhcp_opt_hdr)) >= end)
					return NULL;

				opt = (struct dhcp_opt_hdr *)p;

				if (opt->length > (end - (p + sizeof(opt))))
					return NULL; /* corrupted packet */

				return opt;
		}

	return NULL;
}

char *dhcp_get_hostname(uint8_t *p, const uint8_t *z)
{
	journal_strace(__func__);

	struct dhcp_opt_hdr *h;
	char *dst, *src;
	uint8_t len;

	h = dhcp_get_option(p, z, DHCP_OPT_HOSTNAME);

	if (h == NULL)
		return NULL;

	len = h->length;
	h++;

	src = (char *)((uint8_t *)h);
	dst = calloc(1, len + 1);

	if (dst != NULL) {
		strncpy(dst, src, len);
		dst[len] = '\0';
		return dst;
	}

	return NULL;
}
uint8_t dhcp_get_msgtype(uint8_t *p, const uint8_t *z)
{
	journal_strace(__func__);

	struct dhcp_opt_hdr *h;

	h = dhcp_get_option(p, z, DHCP_OPT_MSGTYPE);
	if (h == NULL || h->length != 1)
		return 0;

	return *(uint8_t *)++h;
}

#define DHCP_OPT_FQDN_FLAGS_LENGTH 3
char *dhcp_get_fqdn(uint8_t *p, const uint8_t *z)
{
	journal_strace(__func__);

	struct dhcp_opt_hdr *h;
	char *dst, *src;
	uint8_t len;

	h = dhcp_get_option(p, z, DHCP_OPT_FQDN);

	if (h == NULL || h->length <= DHCP_OPT_FQDN_FLAGS_LENGTH)
		return NULL;

	len = h->length - DHCP_OPT_FQDN_FLAGS_LENGTH;
	h++;

	src = (char *)((uint8_t *)h + DHCP_OPT_FQDN_FLAGS_LENGTH);
	dst = calloc(1, len + 1);

	if (dst != NULL) {
		strncpy(dst, src, len);
		dst[len] = '\0';
		return dst;
	}

	return NULL;
}

int dhcp_verify(const struct dhcp_msg *dh)
{
	journal_strace(__func__);

	uint8_t chaddr, i;

	if (dh->cookie[0] != 99 || dh->cookie[1] != 130
		|| dh->cookie[2] != 83 || dh->cookie[3] != 99) {
		printf("could not identify DHCP magic cookie\n");
		return 1;
	}

	if (dh->hlen != DHCP_HLEN) {
		printf("Ethernet MAC contains %u numbers, not %u\n",
		    DHCP_HLEN, dh->hlen);
		return 1;
	}

	for (i = 0; i < DHCP_HLEN; i++)
		chaddr |= dh->chaddr[i];

	if (chaddr == 0) {
		printf("hashing key chaddr is absent!\n");
		return 1;
	}

	if (dh->xid == 0) {
		printf("hashing key xid is absent!\n");
		return 1;
	}

	return 0;
}
