#
# xiad/hooklets/Makefile.am
#

INCLUDES = -I../../include

# This only defines what are the targets here
hooklets_LTLIBRARIES = ipset.la mysql.la

# Now we need per-module definition

ipset_la_SOURCES = ipset.c
ipset_la_LDFLAGS = -module

mysql_la_SOURCES = mysql.c
mysql_la_LDFLAGS = -module -lmysqlclient
