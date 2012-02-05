#
# Makefile.am
#

SUBDIRS = src tools
EXTRA_DIST = databases/core.sql databases/generate_db.sh databases/populate_db.sql

subst_config = sed -e 's,[@]hookletsdir[@],$(hookletsdir),g'
xia-dnds.conf:
	$(subst_config) < xia-dnds.conf.in > xia-dnds.conf

dist_sysconf_DATA = xia-dnds.conf pfctl.rules
