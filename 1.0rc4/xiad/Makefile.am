#
# xiad/Makefile.am
#

INCLUDES = -I../include
SUBDIRS = hooklets tools

sbin_PROGRAMS = xiad
xiad_SOURCES = acl.c bridge.c dbal.c discovery.c main.c muxia.c session.c
xiad_SOURCES += acl.h bridge.h dbal.h discovery.h muxia.h session.h
xiad_LDADD = ../lib/libcore.a
xiad_LDFLAGS = -ldl -rdynamic
xiad_CFLAGS = -fno-common -DCONFIG_FILE=\"@sysconfdir@/xiad.conf\"

subst_config = sed -e 's,[@]hookletsdir[@],$(hookletsdir),g'
xiad.conf:
	$(subst_config) < xiad.conf.in > xiad.conf

dist_sysconf_DATA = xiad.conf
