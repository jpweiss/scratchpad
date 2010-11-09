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
msnek4k_driverd_SOURCES = X11Display.h \
	LinuxInputEvent.h LinuxInputEvent.cc \
	LinuxInputDevice.h LinuxInputDevice.cc \
	Trace.cc Daemonizer.cc \
	ProgramOptions_Base.cc msnek4k_driverd.cc \
	Trace.h Daemonizer.h ProgramOptions_Base.h
msnek4k_driverd_LDADD = $(BOOST_PROGRAM_OPTIONS_LIB)


## [jpw]  System-Specific Makefile Vars and Generic Build-Customizations
##
include $(top_srcdir)/make.syscfg.mk

