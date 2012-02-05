AC_INIT([XIA Agent], [1.0rc5])

AC_CONFIG_AUX_DIR(../config)
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(foreign)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([pcap], [pcap_open_live], ,
    AC_MSG_ERROR([cannot find pcap library]))

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

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
