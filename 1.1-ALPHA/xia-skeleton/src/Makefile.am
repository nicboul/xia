#
# src/Makefile.am
#

sbin_PROGRAMS = xia-skeleton
xia_skeleton_SOURCES = main.c
xia_skeleton_SOURCES +=
xia_skeleton_LDFLAGS = -rdynamic @XIALIBS_LIBS@
xia_skeleton_CFLAGS =  -DCONFIG_FILE=\"@sysconfdir@/xia-skeleton.conf\"
xia_skeleton_CFLAGS += @XIALIBS_CFLAGS@
