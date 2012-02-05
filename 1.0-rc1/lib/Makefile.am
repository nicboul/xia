#
# src/Makefile.am
#

INCLUDES = -I../include

noinst_LIBRARIES = libcore.a
libcore_a_SOURCES = event.c hash.c hooklet.c journal.c netbus.c options.c sched.c utils.c xiap.c
libcore_a_SOURCES += event.h hash.h hooklet.h ion.h journal.h netbus.h options.h sched.h utils.h xiap.h

EXTRA_libcore_a_SOURCES = ion_epoll.c ion_kqueue.c
libcore_a_LIBADD = @ION_FLAVOR@
libcore_a_DEPENDENCIES = @ION_FLAVOR@
