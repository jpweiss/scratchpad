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
	LibTrace.h Trace.h Daemonizer.h ProgramOptions_Base.h
msnek4k_driverd_LDADD = $(BOOST_PROGRAM_OPTIONS_LIB)


## [jpw] Can't use dist_bin_SCRIPTS here, since that would install things to
## $(bindir), which is not what we want.  So, define a custom target...
xsessionddir=$(XSESSIOND_PATH)

xsessiond_DATA = 90x11-msnek4k_driverd
	$(top_srcdir)/examples/msnek4k_driverd.xmodmap
EXTRA_DIST = 90x11-msnek4k_driverd.in
CLEANFILES = $(xsessiond_DATA)


## [jpw]  Old; To be removed.
#install-exec-local: 90x11-msnek4k_driverd
#	test -d $(DESTDIR)/$(XSESSIOND_PATH) || \
#	    mkdir -p $(DESTDIR)/$(XSESSIOND_PATH)
#	$(INSTALL_DATA) $(srcdir)/90x11-msnek4k_driverd \
#		$(DESTDIR)/$(XSESSIOND_PATH)


## [jpw]  System-Specific Makefile Vars and Generic Build-Customizations
##
include $(top_srcdir)/make.syscfg.mk

