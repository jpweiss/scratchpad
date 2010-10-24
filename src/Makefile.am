##                                               -*- Makefile -*-
## Process this file with automake to produce Makefile.in
## 
## (Note:  Use double-comments with autoconf files.)
## 
## RCS $Id: Makefile.am 0 2010-02-19 00:30:13Z candide $
#############


## [jpw]  Define targets and sources:
##
bin_PROGRAMS = msnek4k_driverd
msnek4k_driverd_SOURCES = Trace.cc Daemonizer.cc \
	ProgramOptions_Base.cc msnek4k_driverd.cc \
	Trace.h Daemonizer.h ProgramOptions_Base.h
msnek4k_driverd_LDADD = $(BOOST_PROGRAM_OPTIONS_LIB)


## [jpw]  Custom rules
##
