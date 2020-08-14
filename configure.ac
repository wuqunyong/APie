#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([apie], [1.0.0], [xxx], [apie])
AM_INIT_AUTOMAKE([foreign])
AC_PREFIX_DEFAULT([/usr/local/apie/])
AC_CONFIG_SRCDIR([LibAPie/network/ctx.cpp])
AC_CONFIG_HEADER([config.h])

# CXXFLAGS=${CXXFLAGS--g}

# Checks for programs.
# AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([clock_gettime gettimeofday memset socket strchr strstr strtol])

AC_CONFIG_FILES([Makefile
		bin/Makefile
		])

AC_OUTPUT
