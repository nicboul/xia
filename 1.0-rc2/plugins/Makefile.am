#
# xiad/tools/Makefile.am
#

# XXX: hardcoded openvpn include path
INCLUDES = -I../include -I$HOME/openvpn

lib_LTLIBRARIES = libopenvpn_xiad.la

libopenvpn_xiad_la_SOURCES = openvpn.c
libopenvpn_xiad_la_LIBADD = ../lib/libcore.a
