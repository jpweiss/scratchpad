##                                               -*- Makefile -*-
## Process this file with automake to produce Makefile.in
## 
## (Note:  Use double-comments with autoconf files.)
## 
## $Id$
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
AM_CFLAGS = -Wall -W -Wformat-security -Wshadow -Winline
AM_CXXFLAGS = $(AM_CFLAGS) -felide-constructors \
	-Wreorder -Wold-style-cast -Woverloaded-virtual