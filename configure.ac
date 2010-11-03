## Process this file with autoconf to produce a configure script.
##
## (Note:  Use double-comments with autoconf files.)
##
## $Id$
#############

AC_PREREQ([2.65])
AC_INIT([msnek4k_driverd],
        [0.6],
        [jpwcandide@sourceforge.net],
        [],
        [http://sourceforge.net/projects/msnek4kdriverd/])
AC_CONFIG_HEADERS([config.h])

# [jpw]  My licensing and SVN revision variable.
AC_COPYRIGHT([Artistic License])
AC_REVISION([ $Revision$ ])

# [jpw] Pull in my custom macros.
# 
m4_include([ax_jpwTools.m4])

# [jpw] We're building on Linux only.  Follow the FHS layout.
AX_JPW_USE_FHS_DEFAULTS


# [jpw]  Automake
AM_INIT_AUTOMAKE([-Wall color-tests])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_CHECK_LIB([Xtst], [XTestDiscard], [],
             [AC_MSG_ERROR([Failed to find the XTest extension library.
                            Cannot continue.]) ])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([ fcntl.h stdint.h sys/time.h unistd.h \
                   X11/extensions/XTest.h X11/Xlib.h ],
                   [], [AX_JPW_HEADER_ERROR])

# [jpw] Check for required Boost headers that are part of header-only
#       libraries.
#
AX_JPW_REQUIRE_CXX_HEADERS([ boost/algorithm/string/case_conv.hpp \
                             boost/algorithm/string/classification.hpp \
                             boost/algorithm/string/predicate.hpp ])


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


# [jpw] Common checks
AC_CXX_HEADER_STDCXX_TR1
AX_JPW_REQUIRE([CXX_STDCXX_TR1], [the C++ TR1 headers])
AX_BOOST_BASE
AX_BOOST_PROGRAM_OPTIONS
AX_JPW_REQUIRE([BOOST_PROGRAM_OPTIONS], [boost::program_options], [ax])


AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT


##################
## Local Variables:
## mode: autoconf
## End:
