#
# xiad/Makefile.am
#

INCLUDES = -I../include
SUBDIRS = hooklets tools
EXTRA_DIST = databases/core.sql databases/generate_db.sh databases/populate_db.sql

sbin_PROGRAMS = xiad
xiad_SOURCES = acl.c bridge.c dbal.c discovery.c main.c muxia.c session.c
xiad_SOURCES += acl.h bridge.h dbal.h discovery.h muxia.h session.h
xiad_LDADD = ../lib/libcore.la
xiad_LDFLAGS = -rdynamic
xiad_CFLAGS = -fno-common -DCONFIG_FILE=\"@sysconfdir@/xiad.conf\"

subst_config = sed -e 's,[@]hookletsdir[@],$(hookletsdir),g'
xiad.conf:
	$(subst_config) < xiad.conf.in > xiad.conf

dist_sysconf_DATA = xiad.conf pfctl.rules
