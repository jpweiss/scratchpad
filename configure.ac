##                                               -*- Autoconf -*-
## Process this file with autoconf to produce a configure script.
##
## (Note:  Use double-comments with autoconf files.)
## 
## $Id: script.sh 2029 2010-02-19 00:30:13Z candide $
#############

AC_PREREQ([2.65])
AC_INIT([msnek4k_driverd],
        [0.5],
        [jpwcandide@sourceforge.net],
        [msnek4kDriverDaemon],
        [http://sourceforge.net/projects/msnek4kdriverd/])
##AC_CONFIG_HEADERS([config.h]) ## [jpw]  Not needed in this project.

# [jpw]  My licensing and SVN revision variable.
AC_COPYRIGHT([Artistic License])
AC_REVISION([RCS $Id: configure.ac 2029 2010-02-19 00:30:13Z candide $])


## [jpw] Create macros to check for missing headers & defines.
# 
AC_DEFUN([AX_JPW_REQUIRE],
[
    jpw__cachevar_lc=m4_tolower([$1])
    jpw__cachevar_descr="$2"
    jpw__cachevar_prefix="$3"
    if test "x$jpw__cachevar_prefix" = "x" ; then
        jpw__cachevar_prefix=ac
    fi

    if test "x$jpw__cachevar_descr" = "x" ; then
        jpw__cachevar_descr="feature \"$1\""
    fi

    AS_VAR_COPY([jpw__cachevar_set],
                [${jpw__cachevar_prefix}_cv_${jpw__cachevar_lc}])

	if test "$jpw__cachevar_set" != "yes" ; then
        AC_MSG_ERROR([Error:  Failed to find $jpw__cachevar_descr.
                      Cannot continue.])
    fi
])

AC_DEFUN([AX_JPW_HEADER_ERROR],
[
    AC_MSG_ERROR([Error:  Failed to find one or more 
                  required headers.  Cannot continue.])
])

AC_DEFUN([AX_JPW_FUNC_ERROR],
[
    AC_MSG_ERROR([Error:  Failed to find one or more 
                  required library functions.  Cannot continue.])
])


# [jpw]  Automake
AM_INIT_AUTOMAKE([-Wall color-tests])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([Xtst], [XTestDiscard], [],
             [AC_MSG_ERROR([Failed to find the XTest extension library.
                            Cannot continue.]) ])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([ fcntl.h stdint.h sys/time.h unistd.h \
                   X11/extensions/XTest.h X11/Xlib.h \
                   boost/algorithm/string/case_conv.hpp \
                   boost/algorithm/string/classification.hpp \
                   boost/algorithm/string/predicate.hpp ],
                   [], [AX_JPW_HEADER_ERROR])


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
AX_JPW_REQUIRE([FUNC_FORK], [the fork() function])
AC_CHECK_FUNCS([dup2 strerror], [], [AX_JPW_FUNC_ERROR])

## [jpw] Common checks
AC_CXX_HEADER_STDCXX_TR1
AX_JPW_REQUIRE([CXX_HEADER_STDCXX_TR1], [the C++ TR1 headers])
AC_COMPILE_WARNINGS
AX_BOOST_BASE
AX_BOOST_PROGRAM_OPTIONS
AX_JPW_REQUIRE([BOOST_PROGRAM_OPTIONS], [boost::program_options], [ax])


AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
