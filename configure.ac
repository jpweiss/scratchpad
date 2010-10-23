##                                               -*- Autoconf -*-
## Process this file with autoconf to produce a configure script.
##
## (Note:  Use double-comments with autoconf files.)
## 
## RCS $Id: script.sh 1904 2010-02-19 00:30:13Z candide $
#############

AC_PREREQ([2.65])
AC_INIT([msnek4k_driverd],
        [0.5],
        [jpwcandide@sourceforge.net],
        [msnek4kDriverDaemon],
        [http://sourceforge.net/projects/msnek4kdriverd/])
AC_CONFIG_HEADERS([config.h])

# [jpw]  My licensing and SVN revision variable.
AC_COPYRIGHT([Artistic License])
AC_REVISION([$Id: configure.ac 0 $])

# [jpw]  Automake
AM_INIT_AUTOMAKE([-Wall color-tests])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([Xtst],
             [XTestDiscard],
             [AC_MSG_ERROR([Failed to find the XTest extension library.
                            Cannot continue.]) ])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h stdint.h sys/time.h unistd.h \
                  X11/extensions/XTest.h X11/Xlib.h  ])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 strerror])

## [jpw] Common checks
AC_CXX_HEADER_STDCXX_TR1
AC_COMPILE_WARNINGS
AX_BOOST_BASE
AX_BOOST_PROGRAM_OPTIONS

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
