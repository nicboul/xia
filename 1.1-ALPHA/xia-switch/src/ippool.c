/*
 * See COPYRIGHTS file
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ippool.h"

static int get_bit(const uint8_t bitmap[], size_t bit)
{
	return (bitmap[bit/8] >> (bit % 8)) & 1;
}

static void set_bit(uint8_t bitmap[], size_t bit)
{
	bitmap[bit/8] |= (1 << (bit % 8));
}

static void reset_bit(uint8_t bitmap[], size_t bit)
{
	bitmap[bit/8] &= ~(1 << (bit % 8));
}

static int alloc_bitmap(size_t bits, uint8_t **bitmap)
{
	int byte_size = (bits+7)/8;

	*bitmap = calloc(byte_size, sizeof(uint8_t));

	return *bitmap != 0;
}

static int allocate_bit(uint8_t bitmap[], size_t bits, uint32_t *bit)
{
	int i, j, byte_size;

	printf("bit: %i\n", *bit);
	byte_size = bits/8;

	/* byte */
	for (i = 0; (i < byte_size) && (bitmap[i] == 0xff); i++);

	printf("bit: %i\n", *bit);
	
	if (i == byte_size)
		return 0;

	/* bit */
	for (j = 0; get_bit( bitmap+i, j); j++);
	
	*bit = i * 8 + j;
	printf("bit: %i\n", *bit);
	set_bit(bitmap, *bit);

	return 1;
}

static int free_bit(uint8_t bitmap[], size_t bits, size_t bit)
{
	if (bit < bits) {
		reset_bit(bitmap, bit);
		return 1;
	}

	return 0;
}
 
char *ippool_get_ip(ippool_t *ippool)
{

	struct in_addr new_addr;
	uint32_t bit = 0;
	allocate_bit(ippool->pool, ippool->sz, &bit);

	new_addr = ippool->begin;
	new_addr.s_addr += bit;

	return inet_ntoa(new_addr);
}

void ippool_release_ip(ippool_t *ippool, char *ip)
{

	int bit = 0;
	struct in_addr addr;
	inet_aton(ip, &addr);

	printf("rem addr: %x\n", ntohl(addr.s_addr));
	printf("offset: %x\n", ntohl(ippool->begin.s_addr));

	bit = ntohl(ntohl(addr.s_addr) - ntohl(ippool->begin.s_addr));
	printf("bit to be released : %i\n", bit);

	free_bit(ippool->pool, ippool->sz, bit);
} 

ippool_t *ippool_new(char *begin, char *end)
{
	ippool_t *ippool;

	ippool = malloc(sizeof(ippool_t));

	inet_aton(begin, &ippool->begin);
	inet_aton(end, &ippool->end);

	ippool->sz = ntohl(ippool->end.s_addr) - ntohl(ippool->begin.s_addr);

	printf("begin     : %s\n", inet_ntoa(ippool->begin));
	printf("end       : %s\n", inet_ntoa(ippool->end));
	printf("ippool sz : %i\n", (int)ippool->sz);

	alloc_bitmap(ippool->sz, &(ippool->pool));

	return ippool;
}

