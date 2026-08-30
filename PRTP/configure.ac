AC_INIT([iot], [0.7], [hhamalai@iki.fi, hasan.islam@aalto.fi, arseny.kurnikov@aalto.fi])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AM_SILENT_RULES([yes])

AM_PROG_AR

LT_INIT

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_CONFIG_HEADERS([config.h])

# Checks for libraries
# AC_CHECK_LIB([m], [ceil])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memset socket strtol mkdir strchr strdup])

AC_CONFIG_FILES([
  Makefile
  tests/Makefile
  application/Makefile
])
AC_OUTPUT
