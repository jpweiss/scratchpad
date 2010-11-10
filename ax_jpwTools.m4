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


## AX_JPW_SET_VAR_DEFAULT([<varname>],
##                        [<default-value>])
##
## Run this macro to provide a 'configure' variable with a default value.
##
## "<varname>" is the name of a custom variable that users can pass to
## 'configure' on the commandline.
##
## "<default-value>" is the value that "<varname>" should have if not
## explicitly passed to 'configure' on the commandline.
##
AC_DEFUN([AX_JPW_SET_VAR_DEFAULT],
[
     AC_CONFIG_COMMANDS_PRE(
     [
         jpw__varname="$1"
         jpw__defaultval="$2"

         AS_VAR_COPY([jpw__varval], [$jpw__varname])
         if test "x$jpw__varval" = "x"; then
           AS_VAR_COPY([$jpw__varname], [jpw__defaultval])
         fi
     ])
])


## AX_JPW_ARG_MAKEFILE_VAR([<varname>],
##                         [<default-value>],
##                         [<var-doc>])
##
## Invokes "AC_ARG_VAR([<varname>], [<var-doc>])" followed by
## "AX_JPW_SET_VAR_DEFAULT([<varname>], [<default-value>])".  See their
## respective documentation for more info.
##
## You might also consider using the AC_SUBST macro.
##
AC_DEFUN([AX_JPW_ARG_MAKEFILE_VAR],
[
     AC_ARG_VAR([$1], [$3])
     AX_JPW_SET_VAR_DEFAULT([$1], [$2])
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


## Run this macro if building solely on Linux systems.
##
## On Linux systems, software whose binaries, data, and documents are
## installed under "/usr" install their configuration files under "/etc" and
## their state under "/var", not "/usr/etc" or "/usr/var".  Using this macro
## will eliminate the need to set the "--sysconfdir", "--sharedstatedir", and
## "--localstatedir" options when running 'configure'.
##
AC_DEFUN([AX_JPW_USE_FHS_DEFAULTS],
[
     AC_CONFIG_COMMANDS_PRE(
     [
         if test "$prefix" = /usr; then
           test "$sysconfdir" = '${prefix}/etc' && sysconfdir=/etc
           test "$sharedstatedir" = '${prefix}/com' && sharedstatedir=/var
           test "$localstatedir" = '${prefix}/var' && localstatedir=/var
         fi
     ])
])


##################
## Local Variables:
## mode: autoconf
## End:
