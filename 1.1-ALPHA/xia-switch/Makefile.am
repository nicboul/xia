#
# Makefile.am
#

SUBDIRS = src tools

subst_config = sed -e 's,[@]hookletsdir[@],$(hookletsdir),g'
xia-switch.conf:
		$(subst_config) < xia-switch.conf.in > xia-switch.conf

dist_sysconf_DATA = xia-switch.conf
