AC_INIT([xia-ssl], [1.0])

AC_CONFIG_SRCDIR(ChangeLog)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADER(config.h)
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(foreign)

# Platform-specific tweaks
case $host in
*freebsd*)
    AM_CPPFLAGS="$AM_CPPFLAGS -I/usr/local/include"
    AM_LDFLAGS="$AM_LDFLAGS -L/usr/local/lib"
    ;;
*openbsd*)
    AM_CPPFLAGS="$AM_CPPFLAGS -I/usr/local/include"
    AM_LDFLAGS="$AM_LDFLAGS -L/usr/local/lib"
    ;;
esac

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(XIALIBS, xia-libs >= 1.0)
AC_SUBST(XIALIBS_CFLAGS)
AC_SUBST(XIALIBS_LIBS)

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

# Have sysconfdir append a xia subfolder inconditionaly
sysconfdir=${sysconfdir}/xia

# Also propagate the AM_CPPFLAGS, AM_LDFLAGS compiler arguments
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)

AC_CONFIG_FILES([Makefile
		src/Makefile])
AC_OUTPUT
