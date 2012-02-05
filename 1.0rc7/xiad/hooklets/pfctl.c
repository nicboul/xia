
/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../lib/hooklet.h"
#include "../../lib/journal.h"

#define ANCHORNAME "xiad"

#define PFCTL_SBIN_PATH		"/sbin"
#define PFCTL_RULESET_FILE	"/etc/xia/pfctl.rules"

#define MAXCMDLEN 256

int acl_usr_add(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/pfctl -a %s/%u -t acl-%u-usr -T add %s",
		PFCTL_SBIN_PATH, ANCHORNAME, index, index, addr);

	if (system(cmd)) {
		journal_notice("pfctl]> %s %s to acl-%u-usr :: %s:%u\n",
			"unable to add", addr, index, __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int acl_usr_del(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/pfctl -a %s/%u -t acl-%u-usr -T delete %s",
		PFCTL_SBIN_PATH, ANCHORNAME, index, index, addr);

	if (system(cmd)) {
		journal_notice("pfctl]> %s %s from acl-%u-usr :: %s:%u\n",
			"unable to del", addr, index, __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int acl_res_add(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/pfctl -a %s/%u -t acl-%u-res -T add %s",
		PFCTL_SBIN_PATH, ANCHORNAME, index, index, addr);

	if (system(cmd)) {
		journal_notice("pfctl]> %s %s to acl-%u-res :: %s:%u\n",
			"unable to add", addr, index, __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int acl_res_del(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/pfctl -a %s/%u -t acl-%u-res -T delete %s",
		PFCTL_SBIN_PATH, ANCHORNAME, index, index, addr);

	if (system(cmd)) {
		journal_notice("pfctl]> %s %s from acl-%u-res :: %s:%u\n",
			"unable to del", addr, index, __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int acl_set_create(const unsigned int index)
{
	char cmd[MAXCMDLEN];

	snprintf(cmd, MAXCMDLEN,
		"%s/pfctl -q -a %s/%u "
		"-D usr_list=acl-%u-usr -D res_list=acl-%u-res "
		"-f %s", PFCTL_SBIN_PATH, ANCHORNAME, index, index, index,
		PFCTL_RULESET_FILE);

	if (system(cmd)) {
		journal_notice("pfctl]> %s :: %s:%u\n",
			"pfctl failed loading rules", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int acl_set_destroy(const unsigned int index)
{
	char cmd[MAXCMDLEN];

	snprintf(cmd, MAXCMDLEN,
		"%s/pfctl -a \"%s/%u\" -F all",
		PFCTL_SBIN_PATH, ANCHORNAME, index);

	if (system(cmd)) {
		journal_notice("pfctl]> %s :: %s:%u\n",
			"removing set verdict failed", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int acl_set_init()
{
	// nothing to do.
	return 0;
}

void acl_set_fini()
{
	char cmd[MAXCMDLEN];

	snprintf(cmd, MAXCMDLEN,
		"%s/pfctl -a \"%s\" -F all",
		PFCTL_SBIN_PATH, ANCHORNAME);

	if (system(cmd)) {
		journal_notice("pfctl]> %s :: %s:%u\n",
			"flushing anchor failed", __FILE__, __LINE__);
	}
}

int hookin()
{
	return HOOKLET_ACL;
}

