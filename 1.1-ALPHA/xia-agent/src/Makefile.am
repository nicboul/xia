#
# src/Makefile.am
#

sbin_PROGRAMS = xia-agent
xia_agent_SOURCES = main.c acl.c bridge.c discovery.c sniffer.c dhcp.c pingring.c reslist.c kalive.c
xia_agent_SOURCES += acl.h sniffer.h dhcp.h discovery.h pingring.h reslist.h kalive.h
xia_agent_LDFLAGS = @XIALIBS_LIBS@
xia_agent_CFLAGS = -DCONFIG_FILE=\"@sysconfdir@/xia-agent.conf\"
xia_agent_CFLAGS += @XIALIBS_CFLAGS@
