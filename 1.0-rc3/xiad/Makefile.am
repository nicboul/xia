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
xiad_CFLAGS = -fno-common
