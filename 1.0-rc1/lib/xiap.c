/*
 * Copyright (c) 2008; Nicolas Bouliane <nicboul@gmail.com>
 * Copyright (c) 2008; Samuel Jean <peejix@gmail.com>
 * Copyright (c) 2008; Mind4Networks Technologies INC.
 */

#include <xia/xiap.h>

#include "journal.h"

void xiap_print(const struct xiap *xp)
{
	journal_notice("xiap protocol version : %i\n", xp->version);
	journal_notice("xiap msg type         : %i\n", xp->type);
	journal_notice("xiap msg size         : %u\n", ntohs(xp->len));
	journal_notice("xiap query status     : %i\n", xp->status);
}

int xiap_sanitize(const struct xiap *xp)
{
	if (xp->len == 0) {
		journal_notice("%s :: %s:%i\n",
		    "message length field is null",
		    __FILE__, __LINE__);

		return 1;
	}

	if (ntohs(xp->len) > XIAMSG_MAXSIZ) {
		journal_notice("%s %u :: %s:%i\n",
		    "remote side sends big messages!",
		    ntohs(xp->len), __FILE__, __LINE__);

		return 1;
	}

	return 0;
}
