##                                               -*- Makefile -*-
## Process this file with automake to produce Makefile.in
## 
## (Note:  Use double-comments with autoconf files.)
## 
## $Id$
#############


## [jpw]  Define targets and sources:
##
msnek4k_mansrc = msnek4k_driverd.sgml
msnek4k_mantarg = msnek4k_driverd.1
dist_man_MANS = $(msnek4k_mantarg)
EXTRA_DIST = $(msnek4k_mansrc) msnek4k_driverd.lyx


##
## [jpw] Rules for building the manpage
##


$(msnek4k_mantarg):  $(msnek4k_mansrc)
	docbook2man $@

