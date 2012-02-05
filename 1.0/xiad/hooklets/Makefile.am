#
# xiad/hooklets/Makefile.am
#

INCLUDES = -I../../include

AM_LDFLAGS = -module -avoid-version

# This only defines what are the targets here
hooklets_LTLIBRARIES = ipset.la mysql.la pfctl.la

# Now we need per-module definition

ipset_la_SOURCES = ipset.c
ipset_la_LIBADD = ../../lib/libcore.la

mysql_la_SOURCES = mysql.c
mysql_la_LDFLAGS = @MYSQL_LIBS@
mysql_la_LIBADD = ../../lib/libcore.la

pfctl_la_SOURCES = pfctl.c
pfctl_la_LIBADD = ../../lib/libcore.la
