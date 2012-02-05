#
# src/Makefile.am
#


xia_includedir=$(includedir)/xia
xia_include_HEADERS = bridge.h dnds.h event.h hash.h hooklet.h ion.h journal.h mbuf.h netbus.h options.h sched.h utils.h tun.h xsm.h xtp.h

lib_LTLIBRARIES = libxia.la
libxia_la_SOURCES = bridge_brconfig.c dnds.c event.c hash.c hooklet.c journal.c mbuf.c netbus.c options.c sched.c utils.c xsm.c xtp.c

EXTRA_libxia_la_SOURCES = ion_epoll.c ion_kqueue.c tun_ifreq.c tun_tuninfo.c
libxia_la_LIBADD = @ION_FLAVOR@ @TUN_FLAVOR@ $(LIBS)
libxia_la_DEPENDENCIES = @ION_FLAVOR@ @TUN_FLAVOR@

# XXX need some meaning...
# Read:http://www.openismus.com/documents/linux/building_libraries/building_libraries.shtml
#
LIBXIA_VERSION = 1:0:0
libxia_la_LDFLAGS = -version-info $(LIBXIA_VERSION)
