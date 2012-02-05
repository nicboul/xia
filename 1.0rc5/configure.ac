AC_INIT([XIA Software Suite], [1.0rc5])

AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(foreign)

AC_ARG_ENABLE(xia-agent,
	AS_HELP_STRING([--disable-xia-agent],
	[Do not compile xia-agent]),
	[want_agent="$enableval"],
	[want_agent="yes"])

if test x"$want_agent" = xyes; then
	AC_CONFIG_SUBDIRS(xia-agent)
	WANT_SUBDIRS="$WANT_SUBDIRS xia-agent"
fi

AC_ARG_ENABLE(xiad,
	AS_HELP_STRING([--disable-xiad],
	[Do not compile xiad]),
	[want_xiad="$enableval"],
	[want_xiad="yes"])

if test x"$want_xiad" = xyes; then
	AC_CONFIG_SUBDIRS(xiad)
	WANT_SUBDIRS="$WANT_SUBDIRS xiad"
fi

AC_SUBST(WANT_SUBDIRS)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_CHECK_FUNC(kqueue, [AC_SUBST(ION_FLAVOR, ion_kqueue.lo)])
AC_CHECK_FUNC(epoll_ctl, [AC_SUBST(ION_FLAVOR, ion_epoll.lo)])

AC_CONFIG_FILES([Makefile
		lib/Makefile
		plugins/Makefile])
AC_OUTPUT
