#
# src/Makefile.am
#

SUBDIRS = hooklets 

sbin_PROGRAMS = xia-dnds
xia_dnds_SOURCES = dbal.c main.c dnds.c mgmt.c switch.c
xia_dnds_SOURCES += dbal.h dnds.h mgmt.h switch.h
xia_dnds_LDFLAGS = -rdynamic @XIALIBS_LIBS@
xia_dnds_CFLAGS = -fno-common -DCONFIG_FILE=\"@sysconfdir@/xia-dnds.conf\"
xia_dnds_CFLAGS += @XIALIBS_CFLAGS@
