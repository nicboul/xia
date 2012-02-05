#
# xia-agent/Makefile.am
#

INCLUDES = -I../include

sbin_PROGRAMS = xia-agent
xia_agent_SOURCES = main.c acl.c bridge.c discovery.c sniffer.c dhcp.c pingring.c reslist.c kalive.c
xia_agent_SOURCES += acl.h sniffer.h dhcp.h discovery.h pingring.h reslist.h kalive.h
xia_agent_LDADD = ../lib/libcore.la
xia_agent_CFLAGS = -DCONFIG_FILE=\"@sysconfdir@/xia-agent.conf\"

dist_sysconf_DATA = xia-agent.conf
