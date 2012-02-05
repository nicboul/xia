#
# src/Makefile.am
#

INCLUDES = -I../include

noinst_LTLIBRARIES = libcore.la
libcore_la_SOURCES = event.c hash.c hooklet.c journal.c netbus.c options.c sched.c utils.c xiap.c
libcore_la_SOURCES += event.h hash.h hooklet.h ion.h journal.h netbus.h options.h sched.h utils.h xiap.h

EXTRA_libcore_la_SOURCES = ion_epoll.c ion_kqueue.c
libcore_la_LIBADD = @ION_FLAVOR@
libcore_la_DEPENDENCIES = @ION_FLAVOR@
