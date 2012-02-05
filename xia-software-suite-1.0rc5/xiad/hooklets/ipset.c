
/*
 * See COPYRIGHTS file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../lib/hooklet.h"
#include "../../lib/journal.h"

#define IPSET_SBIN_PATH		"/usr/local/sbin"
#define IPTABLES_SBIN_PATH	"/sbin"

#define MAXCMDLEN 256

int acl_usr_add(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --add acl-%u-usr %s", IPSET_SBIN_PATH, index, addr);

	if (system(cmd)) {
		journal_notice("%s %s to acl-%u-usr :: %s:%u\n",
			"unable to add", addr, index, __FILE__, __LINE__);
		return 1;
	}

	return 0;
}

int acl_usr_del(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --del acl-%u-usr %s", IPSET_SBIN_PATH, index, addr);

	if (system(cmd)) {
		journal_notice("ipset]> %s %s from acl-%u-usr :: %s:%u\n",
			"unable to del", addr, index, __FILE__, __LINE__);
		return 1;
	}

	return 0;
}

int acl_res_add(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --add acl-%u-res %s", IPSET_SBIN_PATH, index, addr);

	if (system(cmd)) {
		journal_notice("ipset]> %s %s to acl-%u-res :: %s:%u\n",
			"unable to add", addr, index, __FILE__, __LINE__);
		return 1;
	}

	return 0;
}

int acl_res_del(const char *addr, const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --del acl-%u-res %s", IPSET_SBIN_PATH, index, addr);

	if (system(cmd)) {
		journal_notice("ipset]> %s %s from acl-%u-res :: %s:%u\n",
			"unable to del", addr, index, __FILE__, __LINE__);
		return 1;
	}

	return 0;
}

int acl_set_create(const unsigned int index)
{
	char cmd[MAXCMDLEN];
		
	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --create acl-%u-%s iphash --resize 0",
		IPSET_SBIN_PATH, index, "usr");

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to create ACL set", __FILE__, __LINE__);
		return 1;
	}

	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --create acl-%u-%s iphash --resize 0",
		IPSET_SBIN_PATH, index, "res");

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to create ACL set", __FILE__, __LINE__);
		return 1;
	}
	
	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --bind acl-%u-usr :default: -b acl-%u-res",
		IPSET_SBIN_PATH, index, index);
	
	if (system(cmd)) {
		journal_notice("ipset> %s :: %s:%u\n",
			"unable to bind sets", __FILE__, __LINE__);
		return 1;
	}

	snprintf(cmd, MAXCMDLEN,
		"%s/iptables -I XIAD_ACL "
		"-m set --set acl-%u-usr src,dst -j ACCEPT",
		IPTABLES_SBIN_PATH, index);

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"applying set verdict failed", __FILE__, __LINE__);
		return 1;
	}

	snprintf(cmd, MAXCMDLEN,
		"%s/iptables -I XIAD_ACL "
		"-m set --set acl-%u-usr dst,src -j ACCEPT",
		IPTABLES_SBIN_PATH, index);

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"applying set verdict failed", __FILE__, __LINE__);
		return 1;
	}

	return 0;
}

int acl_set_destroy(const unsigned int index)
{
	char cmd[MAXCMDLEN];
	int err = 0;

	snprintf(cmd, MAXCMDLEN,
		"%s/iptables -D XIAD_ACL "
		"-m set --set acl-%u-usr src,dst -j ACCEPT",
		IPTABLES_SBIN_PATH, index);

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"removing set verdict failed", __FILE__, __LINE__);
		err = 1;
	}

	snprintf(cmd, MAXCMDLEN,
		"%s/iptables -D XIAD_ACL "
		"-m set --set acl-%u-usr dst,src -j ACCEPT",
		IPTABLES_SBIN_PATH, index);

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"removing set verdict failed", __FILE__, __LINE__);
		err = 1;
	}

	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --unbind acl-%u-usr :all:", IPSET_SBIN_PATH, index);

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to unbind sets", __FILE__, __LINE__);
		err = 1;
	}

	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --destroy acl-%u-usr", IPSET_SBIN_PATH, index);

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to destroy ACL set", __FILE__, __LINE__);
		err = 1;
	}

	snprintf(cmd, MAXCMDLEN,
		"%s/ipset --destroy acl-%u-res", IPSET_SBIN_PATH, index);

	if (system(cmd)) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to destroy ACL set", __FILE__, __LINE__);
		err = 1;
	}

	return err;
}

int acl_set_init()
{
	if (system("/sbin/iptables --new XIAD_ACL")) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to create XIAD_ACL chain",
			__FILE__, __LINE__);
		return 1;
	}

	if (system("/sbin/iptables -I FORWARD -j XIAD_ACL")) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to link XIAD_ACL from FORWARD chain",
			__FILE__, __LINE__);
		return 1;
	}

	if (system("/usr/local/sbin/ipset --create acl-0-usr iphash --resize 0")) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to create ACL set", __FILE__, __LINE__);
		return 1;
	}

	if (system("/sbin/iptables -I INPUT "
		   "-m set --set acl-0-usr src -j ACCEPT")) {
		journal_notice("ipset]> %s :: %s:%u\n",
			"applying set verdict failed", __FILE__, __LINE__);
		return 1;
	}

	return 0;
}

void acl_set_fini()
{
	if (system("/sbin/iptables -D INPUT "
		   "-m set --set acl-0-usr src -j ACCEPT"))
		journal_notice("ipset]> %s :: %s:%u\n",
			"removing set verdict failed", __FILE__, __LINE__);

	if (system("/usr/local/sbin/ipset --destroy acl-0-usr"))
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to destroy ACL set", __FILE__, __LINE__);

	if (system("/sbin/iptables -D FORWARD -j XIAD_ACL"))
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to remove XIAD_ACL link from FORWARD chain",
			__FILE__, __LINE__);

	if (system("/sbin/iptables --delete-chain XIAD_ACL"))
		journal_notice("ipset]> %s :: %s:%u\n",
			"unable to delete XIAD_ACL chain",
			__FILE__, __LINE__);
}

int hookin()
{
	return HOOKLET_ACL;
}

