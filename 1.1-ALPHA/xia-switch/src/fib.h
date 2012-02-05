/*
 * See COPYRIGHTS file.
 */

#ifndef FIB_H
#define FIB_H

#include <stdint.h>

#include <xia/netbus.h>

#include "switch.h"

typedef struct fib_entry {
	
	uint8_t mac_addr[6];
	struct port *port;
	uint32_t vlan_id;

	struct fib_entry *next;
	struct fib_entry *prev;

} fib_entry_t;

void fib_delete(fib_entry_t *[], const uint8_t *);
void fib_add(fib_entry_t *[], fib_entry_t *);
fib_entry_t *fib_lookup(fib_entry_t *[], const uint8_t *);

#endif /* FIB_H */
