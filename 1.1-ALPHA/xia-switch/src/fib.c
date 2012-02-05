/*
 * See COPYRIGHTS file
 */

#include <stdlib.h>

#include "fib.h"
#include "switch.h"

static uint32_t fib_hash(const uint8_t *mac_addr)
{
	journal_ftrace(__func__);

	static uint32_t salt = 0;
	uint32_t result;
	uint32_t key[2] = {0};

	memcpy(key, mac_addr, 6);
	if (salt == 0)
		salt = rand();

	result = hashword(key, 2, salt);

	return result % 1024;
}

void fib_delete(fib_entry_t *fib_cache[], const uint8_t *mac_addr)
{
	journal_ftrace(__func__);

	uint32_t index;
	index = fib_hash(mac_addr);

	fib_cache[index] = NULL;
}
	
void fib_add(fib_entry_t *fib_cache[], fib_entry_t *fib_entry)
{
	journal_ftrace(__func__);

	uint32_t index;
	index = fib_hash(fib_entry->mac_addr);
	
	if (fib_cache[index] != NULL) {
		journal_notice("fib]> fib_cache collision at index {%i} :: %s:%i\n", index, __FILE__, __LINE__);
	}

	fib_cache[index] = fib_entry;
}

fib_entry_t *fib_lookup(fib_entry_t *fib_cache[], const uint8_t *mac_addr)
{
	journal_ftrace(__func__);

	uint32_t index;
	index = fib_hash(mac_addr);
	
	return fib_cache[index];
}
