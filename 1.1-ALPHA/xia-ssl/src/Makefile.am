#
# src/Makefile.am
#

sbin_PROGRAMS = xia-ssl
xia_ssl_SOURCES = main.c client.c server.c ssl.c
xia_ssl_SOURCES += client.h server.h ssl.h
xia_ssl_LDFLAGS = -rdynamic @XIALIBS_LIBS@ -lssl -lcrypto
xia_ssl_CFLAGS =  -DCONFIG_FILE=\"@sysconfdir@/xia-ssl.conf\"
xia_ssl_CFLAGS += @XIALIBS_CFLAGS@
