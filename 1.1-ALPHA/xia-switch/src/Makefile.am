#
# src/Makefile.am
#

SUBDIRS = hooklets

sbin_PROGRAMS = xia-switch
xia_switch_SOURCES = main.c context.c dbal.c fib.c inet.c ippool.c switch.c 
xia_switch_SOURCES += context.h dbal.h fib.h inet.h ippool.h protocols.h switch.h
xia_switch_LDFLAGS = -rdynamic @XIALIBS_LIBS@
xia_switch_CFLAGS =  -DCONFIG_FILE=\"@sysconfdir@/xia-switch.conf\"
xia_switch_CFLAGS += @XIALIBS_CFLAGS@
