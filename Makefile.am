##                                               -*- Makefile -*-
## Process this file with automake to produce Makefile.in
## 
## (Note:  Use double-comments with autoconf files.)
## 
## $Id$
#############


SUBDIRS = src

sysconf_DATA = examples/msnek4k_driverd.conf
nobase_doc_DATA = LICENSE WARRANTY INSTALL examples/*

## [jpw]  I don't get why I need to specify the stuff in $(nobase_doc_DATA)
## here.  I thought that *_DATA macros added their files to the
## dist-manifest...
EXTRA_DIST = $(nobase_doc_DATA) debian/*