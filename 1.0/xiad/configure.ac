AC_INIT([XIA Server], [1.0])

AC_CONFIG_AUX_DIR(../config)
AC_CONFIG_HEADER(config.h)
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(foreign)

# Platform-specific tweaks
case $host in
*freebsd*)
    AM_CPPFLAGS="$AM_CPPFLAGS -I/usr/local/include";;
*openbsd*)
    AM_CPPFLAGS="$AM_CPPFLAGS -I/usr/local/include";;
esac

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_ARG_WITH(hookletsdir,
    AS_HELP_STRING([--with-hooklets-dir=PATH],
    [Path where we install the xiad hooklets [LIBDIR/xia/xiad]]),
    [hookletsdir="$withval"], [hookletsdir="${libdir}/xia/xiad"])

AC_SUBST(hookletsdir)

AC_ARG_WITH(mysql_config,
    AS_HELP_STRING(
        [--with-mysql=PROGRAM],
        [Path where mysql_config program can be found]),
    [mysql_config_path=$withval])

AC_PATH_PROG(MYSQL_CONFIG, mysql_config, $mysql_config_path)

# Retreive MySQL library paths
MYSQL_LIBS=$($MYSQL_CONFIG --libs)

if test "x$MYSQL_LIBS" = "x"; then
    AC_MSG_ERROR([MySQL library path is empty.  Use --with-mysql=<path/to/mysql_config>])
fi

# Check for dlopen
AC_SEARCH_LIBS(dlopen, dl)

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

# Propagate MySQL library path
AC_SUBST(MYSQL_LIBS, $MYSQL_LIBS)

# Also propagate the AM_CPPFLAGS
AC_SUBST(AM_CPPFLAGS)

AC_CONFIG_FILES([Makefile
		hooklets/Makefile
		tools/Makefile])
AC_OUTPUT
