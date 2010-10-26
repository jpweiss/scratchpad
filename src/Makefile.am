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


## [jpw]  Custom vars
##

COMPILE_TYPE =

DEBUG = -g3 -ggdb3 -DDEBUG
DISABLE_INLINE =- fno-inline -fno-default-inline

REGRESSION = -O0 $(DISABLE_INLINE)

OPTIMIZE = -O3 -mieee-fp -malign-double \
	-fexpensive-optimizations -fprefetch-loop-arrays \
	-funroll-loops -fpeel-loops -funswitch-loops -frerun-loop-opt \
	-fno-caller-saves

GPROF_GCC = -pg
GCOV_GCC = -fprofile-arcs -ftest-coverage
PROFILE = $(GPROF_GCC) $(GCOV_GCC)

WARNFLAGS_COMMON = -Wall -W -Wformat-security -Wshadow # -Winline
WARNFLAGS_CXX = -felide-constructors -Wreorder \
	-Wold-style-cast -Woverloaded-virtual


## [jpw] How to compile?  Debug mode?  Optimized?  With or without profiling?
## 

#DEBUG_FLAGS = -DDEBUG
#LDFLAGS += $(PROFILE)
#LDFLAGS += -lefence
#LDFLAGS += -ldmalloc -ldmallocxx 
#COMPILE_TYPE += $(DEBUG) #$(PROFILE) #$(DEBUG_FLAGS)
#COMPILE_TYPE += $(DEBUG) $(DISABLE_INLINE) #$(DEBUG_FLAGS)
COMPILE_TYPE += $(OPTIMIZE)
##WARNING## Use this for the regression tests:
#COMPILE_TYPE += $(REGRESSION)


## [jpw]  The Automake-specific versions of several standard Makefile
##        variables.
##

AM_CFLAGS = $(WARNFLAGS_COMMON) $(COMPILE_TYPE)
AM_CXXFLAGS = $(WARNFLAGS_COMMON) $(WARNFLAGS_CXX) $(COMPILE_TYPE)