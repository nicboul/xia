#
# src/hooklets/Makefile.am
#

hooklets_LTLIBRARIES = ipset.la mysql.la pfctl.la

ipset_la_SOURCES = ipset.c
ipset_la_LDFLAGS = @XIALIBS_LIBS@ -module -avoid-version

mysql_la_SOURCES = mysql.c
mysql_la_LDFLAGS = @MYSQL_LIBS@ @XIALIBS_LIBS@ -module -avoid-version

pfctl_la_SOURCES = pfctl.c
pfctl_la_LDFLAGS = @XIALIBS_LIBS@ -module -avoid-version
