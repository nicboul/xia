/*
 * See COPYRIGHTS file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dnds.h"
#include "journal.h"

struct dnds *dnds_encapsulate(uint16_t subject, void *payload, uint16_t p_size)
{
	struct dnds *dnds;

	dnds = malloc(DNDS_HEADER_SIZE + p_size);
	if (dnds == NULL)
		return NULL;

	memset(dnds, 0, DNDS_HEADER_SIZE + p_size);
	dnds->version = htons(DNDS_VERSION);
	dnds->subject = htons(subject);

	memmove(dnds + 1, payload, p_size);
	return dnds;
}

/*
 * dnds_acl_print
 */
void dnds_acl_print(DNDS_ACL *acl)
{
	journal_notice("acl identifier         : %u\n", acl->id);
	journal_notice("acl context            : %u\n", acl->context);
	journal_notice("acl description        : %s\n", acl->desc);
	journal_notice("acl age                : %u\n", acl->age);
}

/*
 * dnds_node_print
 */
void dnds_node_print(DNDS_NODE *node)
{
	journal_notice("node identifier        : %u\n", node->id);
	journal_notice("node type              : %u\n", node->type);
	journal_notice("node name              : %s\n", node->name);
	journal_notice("node ip address        : %s\n", inet_ntoa(node->addr));
	journal_notice("node perm identifier   : %u\n", node->perm);
	journal_notice("node flag              : %u\n", node->flag);
	journal_notice("node age               : %u\n", node->age);
}

/*
 * dnds_perm_print
 */
void dnds_perm_print(DNDS_PERM *perm)
{
	journal_notice("perm identifier        : %u\n", perm->id);
	journal_notice("perm name              : %s\n", perm->name);
	journal_notice("perm matrix            : %u\n", perm->matrix[0]);
	journal_notice("perm age               : %u\n", perm->age);
}

/*
 * dnds_fini
 */
void dnds_fini()
{
	/* empty */
}

/*
 * dnds_init
 */
int dnds_init()
{
	return 0;
}
