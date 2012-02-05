AC_INIT([xia-libs], [1.1-ALPHA])

AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(foreign)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.

# dlopen
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

AC_CHECK_FUNC(kqueue, [AC_SUBST(ION_FLAVOR, ion_kqueue.lo)])
AC_CHECK_FUNC(epoll_ctl, [AC_SUBST(ION_FLAVOR, ion_epoll.lo)])

AC_CHECK_HEADER(linux/if_tun.h, [AC_SUBST(TUN_FLAVOR, tun_ifreq.lo)])
AC_CHECK_HEADER(net/if_tun.h, [AC_SUBST(TUN_FLAVOR, tun_tuninfo.lo)])

AC_CONFIG_FILES([Makefile
		src/Makefile
		config/xia-libs.pc])
AC_OUTPUT
