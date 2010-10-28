## A set of custom Autoconf macros that I commonly use.
##
## Copyright (C) 2010 by John P. Weiss
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the Artistic License, included as the file
## "LICENSE" in the source code archive.
##
## This package is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
##
## You should have received a copy of the file "LICENSE", containing
## the License John Weiss originally placed this program under.
##
## $Id$
#############


## AX_JPW_REQUIRE([<feature_name>],
##                [<what-failed>],
##                [<cache-variable-prefix>])
##
## Abort with an error if a specific feature isn't defined.
##
## "<feature-name>" will be either the name of a feature, or the part of a
## cache variable name following the "_cv_" infix.
##
## Specifically, if a test defines a flag in "config.h" named 
## 'HAVING_FOO_THING', then the "<feature-name>" will be "FOO_THING".  Most
## library- and function-checking macros are named "AC_<feature-name>"
## or "AC_<feature-name>", which also makes it easy to identify the
## "<feature-name>".
##
## {Optional}
## "<what-failed>" is a description of the feature.  It should be a noun,
## since it will be included in a standard error message.
##
## Omitting this argument, or passing the empty string, is equivalent to using
## [feature "<feature-name"] as the second argument.
##
## {Optional}
## "<cache-variable-prefix>" is, as the name implies, the prefix on the cache
## variable that is actually being checked.  Usually either "ac" or "ax", 
## which also happens to be the prefix of the library- or function-checking 
## macro for "<feature-name>".
##
## Omitting this argument, or passing the empty string, is equivalent to using
## [ac] as the third argument.
##
AC_DEFUN([AX_JPW_REQUIRE],
[
    jpw__feature_lc=m4_tolower([$1])
    jpw__cachevar_descr="$2"
    jpw__cachevar_prefix=m4_tolower([$3])
    if test "x$jpw__cachevar_prefix" = "x" ; then
        jpw__cachevar_prefix=ac
    fi

    if test "x$jpw__cachevar_descr" = "x" ; then
        jpw__cachevar_descr="feature \"$1\""
    fi

    jpw__cachevar_name="${jpw__cachevar_prefix}_cv_${jpw__feature_lc}"
    AS_VAR_COPY([jpw__cachevar_set], [$jpw__cachevar_name])

	if test "$jpw__cachevar_set" != "yes" ; then
        AC_MSG_ERROR([Error:  Failed to find $jpw__cachevar_descr.
                      Cannot continue.])
    fi
])


## AX_JPW_CHECK_CXX_HEADERS([<header-file-list>],
##                          [<action-if-found>],
##                          [<action-if-not-found>],
##                          [<includes>])
##
## Equivalent of the standard 'AC_CHECK_HEADERS' Autoconf macro, but using the
## C++ compiler instead of the C compiler.  See the Autoconf documentation of
## 'AC_CHECK_HEADERS' for details.
##
AC_DEFUN([AX_JPW_CHECK_CXX_HEADERS],
[
	AC_LANG_PUSH(C++)
    AC_CHECK_HEADERS([$1], [$2], [$3], [$4])
	AC_LANG_POP([C++])    
])


## A canned error message for missing headers.
##
## Typically used as follows:
##
##     AC_CHECK_HEADERS([myheader.h foo.h], [], [AX_JPW_HEADER_ERROR])
##
AC_DEFUN([AX_JPW_HEADER_ERROR],
[
    AC_MSG_ERROR([Error:  Failed to find one or more 
                  required headers.  Cannot continue.])
])


## AX_JPW_REQUIRE_CXX_HEADERS([<header-file-list>],
##                            [<includes>])
##
## Syntactic sugar around 'AX_JPW_CHECK_CXX_HEADERS'.  Equivalent to:
## 
##     AX_JPW_CHECK_CXX_HEADERS([<header-file-list>], [],
##                              [AX_JPW_HEADER_ERROR], [<includes>])
##
AC_DEFUN([AX_JPW_REQUIRE_CXX_HEADERS],
[
    AX_JPW_CHECK_CXX_HEADERS([$1], [], [AX_JPW_HEADER_ERROR], [$2])
])


## A canned error message for missing headers.
##
## Typically used as follows:
##
##     AC_CHECK_FUNCS([strerror fopen], [], [AX_JPW_FUNC_ERROR])
##
AC_DEFUN([AX_JPW_FUNC_ERROR],
[
    AC_MSG_ERROR([Error:  Failed to find one or more 
                  required library functions.  Cannot continue.])
])


## Run this macro if "/usr/etc" doesn't exist on your system.
##
## On Linux systems, software whose binaries, data, and documents are
## installed under "/usr" install their configuration files under "/etc",
## not "/usr/etc".  Using this macro will eliminate the need to pass
## "--sysconfdir=/etc" when using "--prefix=/usr", and omit it at all other
## times.
##
AC_DEFUN([AX_JPW_NO_USR_ETC],
[
    AC_ARG_ENABLE(
        [usr-etc],
        [AS_HELP_STRING([--enable-usr-etc],
                        [The standard version of "configure" uses 'PREFIX/etc'
                         as the default value of the "--sysconfdir" 
                         option.  This version of "configure" is somewhat 
                         different.  It converts any "--sysconfdir=/usr/etc"
                         (including the default value) to 
                         "--sysconfdir=/etc".  Use this option to disable this
                         custom behavior.
                        ])
        ],
        [],
        [enable_usr_etc=no])

    AC_CONFIG_COMMANDS_PRE(
    [
        if test "$prefix" = "/usr" ; then
           if test "$sysconfdir" = "${prefix}/etc" ; then
               if test "$enable_usr_etc" = "no" ; then
                   sysconfdir="/etc"
               fi
           fi
        fi
    ])
])


##################
## Local Variables:
## mode: autoconf
## End:
